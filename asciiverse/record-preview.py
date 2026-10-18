"""Records Star Lanes' catalog preview (hub/RECORDING.md): 100x30, about 50 s.

Runs the game under `asciinema rec` in a 100x30 pseudo-terminal, reads the
screen through pyte and plays a two-player hot-seat game: each turn it
takes the offered move next to a company, a new-company marker or a star
when there is one, buys as much stock as it can afford in one company,
and reads each announcement before going on. At the end it quits, which
shows the final standings. Usage, from the repo root after
asciiverse/build.sh (the binary in out/), with pyte installed:

    python3 asciiverse/record-preview.py

The game runs by name from out/ on PATH, so the cast's header holds no local
path. The cast goes to asciiverse/starlanes.cast.
"""
import fcntl, os, pty, random, re, select, shutil, struct, sys, tempfile, termios, time

import pyte

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COLS, ROWS = 100, 30
SCREEN_W, SCREEN_H = 80, 24      # the game's screen, centred (ui.c)
MAPX, MAPY = 12, 10
LENGTH = 48.0                    # seconds of play before quitting
NAMES = ["Vega", "Rigel"]
COMPANY = (r"(?:Altair Starways|Beetlejuice, Ltd\.|Capella Freight Co\.|Denebola Shippers"
           r"|Eridani Expediters) +\$(\d+)")

random.seed(7)

# ncurses repeats a character with REP (CSI n b) and scrolls part of the
# screen with SU and SD (CSI n S, CSI n T), which pyte does not know
_draw = pyte.Screen.draw


def _draw_remembering(self, data):
    if data:
        self.last_char = data[-1]
    _draw(self, data)


def _repeat_last(self, n=1, *rest, **kw):
    if getattr(self, "last_char", None):
        _draw(self, self.last_char * max(1, n))


def _scroll_up(self, n=1, *rest, **kw):
    """SU (CSI n S), which ncurses uses inside a scroll region."""
    top, bottom = self.margins or pyte.screens.Margins(0, self.lines - 1)
    for _ in range(max(1, n)):
        for y in range(top, bottom):
            self.buffer[y] = self.buffer[y + 1]
        self.buffer.pop(bottom, None)
    self.dirty.update(range(self.lines))


def _scroll_down(self, n=1, *rest, **kw):
    """SD (CSI n T)."""
    top, bottom = self.margins or pyte.screens.Margins(0, self.lines - 1)
    for _ in range(max(1, n)):
        for y in range(bottom, top, -1):
            self.buffer[y] = self.buffer[y - 1]
        self.buffer.pop(top, None)
    self.dirty.update(range(self.lines))


pyte.Screen.draw = _draw_remembering
pyte.Screen.repeat_last = _repeat_last
pyte.Screen.scroll_up = _scroll_up
pyte.Screen.scroll_down = _scroll_down
pyte.Stream.csi = dict(pyte.Stream.csi, b="repeat_last", S="scroll_up", T="scroll_down")


class Rec:
    def __init__(self):
        self.home = tempfile.mkdtemp(prefix="starlanes-rec-")
        env = {
            "PATH": f"{os.path.join(ROOT, 'out')}:{os.environ['PATH']}",
            "HOME": self.home,
            "XDG_DATA_HOME": os.path.join(self.home, "data"),
            "USER": "player",
            "LOGNAME": "player",
            "TERM": "xterm-256color",
            "COLORTERM": "truecolor",
            "LANG": "C.UTF-8",
        }
        self.cast = os.path.join(ROOT, "asciiverse", "starlanes.cast")
        self.screen = pyte.Screen(COLS, ROWS)
        self.stream = pyte.ByteStream(self.screen)
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(self.home)
            os.execvpe("asciinema", ["asciinema", "rec", "--overwrite", "-q", "--idle-time-limit", "2",
                "--window-size", f"{COLS}x{ROWS}", "--capture-env", "TERM,COLORTERM,LANG",
                "--title", "Star Lanes", "--command", "starlanes", self.cast], env)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, COLS, 0, 0))
        self.start = time.time()

    def pump(self, t):
        end = time.time() + t
        while True:
            left = end - time.time()
            r, _, _ = select.select([self.fd], [], [], max(0.0, min(0.01, left)))
            if r:
                try:
                    self.stream.feed(os.read(self.fd, 65536))
                except OSError:
                    return False
            if left <= 0:
                return True

    def key(self, k, wait=0.0):
        os.write(self.fd, k.encode() if isinstance(k, str) else k)
        self.pump(wait)

    def type(self, text, gap=0.12):
        for ch in text:
            self.key(ch, gap)

    def text(self):
        return "\n".join(self.screen.display)

    def finish(self):
        end = time.time() + 5
        while time.time() < end:
            if not self.pump(0.1):
                break
            done, _ = os.waitpid(self.pid, os.WNOHANG)
            if done:
                break
        else:
            os.kill(self.pid, 9)    # SIGKILL: nothing more is written to the cast
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass
        shutil.rmtree(self.home, ignore_errors=True)
        print(f"{self.cast}: {os.path.getsize(self.cast) // 1024} KB, {time.time() - self.start:.0f} s",
              file=sys.stderr)


def board(rec):
    """The map as MAPY strings of MAPX cells, cut out where the game centred it."""
    oy, ox = (ROWS - SCREEN_H) // 2, (COLS - SCREEN_W) // 2
    rows = []
    for i in range(MAPY):
        line = rec.screen.display[oy + 2 + i]
        rows.append("".join(line[ox + 3 + j * 3] for j in range(MAPX)))
    return rows


def pick_move(rows):
    """The offered move (1 to 5) with the most interesting neighbours."""
    best, score = None, -1
    for i in range(MAPY):
        for j in range(MAPX):
            c = rows[i][j]
            if c not in "12345":
                continue
            s = random.random()
            for di, dj in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                ni, nj = i + di, j + dj
                if 0 <= ni < MAPY and 0 <= nj < MAPX:
                    n = rows[ni][nj]
                    if n.isalpha() and n.isupper():
                        s += 3           # grows a company
                    elif n == "+":
                        s += 2           # may found one
                    elif n == "*":
                        s += 1.5
                    elif n == "@":
                        s -= 4           # black hole
            if s > score:
                best, score = c, s
    return best or "1"


def main():
    rec = Rec()
    rec.pump(2.0)
    rec.key("2", 0.8)
    for name in NAMES:
        rec.type(name)
        rec.key("\r", 0.6)
    end = rec.start + LENGTH
    while True:
        rec.pump(0.15)
        t = rec.text()
        if "Press any key to continue" in t:
            rec.pump(1.8)                # let it be read
            rec.key(" ", 0.4)
        elif "enter your move" in t:
            if time.time() > end:
                break
            rec.pump(0.7)
            rec.key(pick_move(board(rec)), 0.6)
        elif "Arrow keys to select a company" in t:
            # the trade list holds the active companies in the order the
            # company table shows them; buy the most of one we can afford
            prices = [int(p) for p in re.findall(COMPANY, t)]
            m = re.search(r"\(Cash: \$(-?\d+)\)", t)
            cash = int(m.group(1)) if m else 0
            can = [i for i, p in enumerate(prices) if 0 < p <= cash]
            if can:
                pick = random.choice(can)
                for _ in range(pick):
                    rec.key("j", 0.35)
                rec.key("\r", 0.5)
                rec.key("m", 0.4)
                rec.key("\r", 0.9)
            rec.key("\x1b", 1.3)         # escape ends trading (curses waits for more first)
        elif "Amount (" in t:
            rec.key("\r", 0.5)
    rec.pump(0.6)
    rec.key("q", 1.0)                    # quit: the y/n box, then the final standings
    rec.key("y", 3.5)
    rec.key(" ", 0.5)
    rec.finish()


if __name__ == "__main__":
    main()
