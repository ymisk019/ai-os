#!/usr/bin/env python3
"""
Yazan OS 3.0 (Fire Edition) - Visual Renderer & UI Preview Simulator
Uses the real font8x8 bitmap font to render pixel-perfect preview images:
1. Boot Splash Screen
2. Desktop with Multi-Window Environment (Terminal, Calculator, Settings)
3. Start Menu & Taskbar
4. Red Screen of Death (Insufficient RAM Panic)
5. Red Screen of Death (CPU Exception / Crash Panic)
Outputs PPM & PNG images.
"""

import os
import re

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(ROOT_DIR, "build")

WIDTH = 1024
HEIGHT = 768

# Color Palette (0xRRGGBB)
WALLPAPER_TOP = (7, 11, 25)
WALLPAPER_MID = (23, 21, 59)
WALLPAPER_BOTTOM = (14, 37, 74)
TOPBAR_BG = (10, 15, 29)
TASKBAR_BG = (13, 19, 34)
START_BLUE = (37, 99, 235)
SKY_BLUE = (56, 189, 248)
TEXT_WHITE = (248, 250, 252)
TEXT_MUTED = (148, 163, 184)
TEXT_DIM = (100, 116, 139)
PANIC_RED_TOP = (127, 29, 29)
PANIC_RED_BOTTOM = (69, 10, 10)

def load_font():
    font_path = os.path.join(os.path.dirname(__file__), "../kernel/font8x8.h")
    with open(font_path) as f:
        text = f.read()
    matches = re.findall(r'\{([0-9a-fA-Fx,\s]+)\}', text)
    glyphs = []
    for m in matches:
        nums = [int(x.strip(), 16) for x in m.split(',') if x.strip()]
        if len(nums) == 8:
            glyphs.append(nums)
    return glyphs

FONT = load_font()

class Canvas:
    def __init__(self, w, h, bg=(0, 0, 0)):
        self.w = w
        self.h = h
        self.pixels = [list(bg) for _ in range(w * h)]

    def pixel(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.pixels[y * self.w + x] = list(c)

    def fill_rect(self, x, y, w, h, c):
        x1 = max(0, x)
        y1 = max(0, y)
        x2 = min(self.w, x + w)
        y2 = min(self.h, y + h)
        for j in range(y1, y2):
            idx = j * self.w + x1
            for _ in range(x1, x2):
                self.pixels[idx] = list(c)
                idx += 1

    def vgradient(self, x, y, w, h, c1, c2):
        for j in range(h):
            t = j / max(1, h - 1)
            c = (
                int(c1[0] * (1 - t) + c2[0] * t),
                int(c1[1] * (1 - t) + c2[1] * t),
                int(c1[2] * (1 - t) + c2[2] * t)
            )
            self.fill_rect(x, y + j, w, 1, c)

    def fill_rrect(self, x, y, w, h, r, c):
        r2 = r * r
        for j in range(h):
            py = y + j
            if not (0 <= py < self.h):
                continue
            for i in range(w):
                px = x + i
                if not (0 <= px < self.w):
                    continue
                dx = 0
                dy = 0
                if i < r: dx = r - 1 - i
                elif i >= w - r: dx = i - (w - r)
                if j < r: dy = r - 1 - j
                elif j >= h - r: dy = j - (h - r)
                if dx * dx + dy * dy >= r2 and (dx or dy):
                    continue
                self.pixels[py * self.w + px] = list(c)

    def rect_outline(self, x, y, w, h, c):
        self.fill_rect(x, y, w, 1, c)
        self.fill_rect(x, y + h - 1, w, 1, c)
        self.fill_rect(x, y, 1, h, c)
        self.fill_rect(x + w - 1, y, 1, h, c)

    def blend_rect(self, x, y, w, h, c, alpha):
        a = alpha / 255.0
        x1 = max(0, x)
        y1 = max(0, y)
        x2 = min(self.w, x + w)
        y2 = min(self.h, y + h)
        for j in range(y1, y2):
            for i in range(x1, x2):
                idx = j * self.w + i
                bg = self.pixels[idx]
                mixed = [
                    int(bg[0] * (1 - a) + c[0] * a),
                    int(bg[1] * (1 - a) + c[1] * a),
                    int(bg[2] * (1 - a) + c[2] * a)
                ]
                self.pixels[idx] = mixed

    def draw_text(self, x, y, text, scale=1, c=(255, 255, 255)):
        for ch in text:
            code = ord(ch)
            if 0x20 <= code <= 0x7E:
                glyph = FONT[code - 0x20]
                for row in range(8):
                    for col in range(8):
                        if glyph[row] & (1 << col):
                            self.fill_rect(x + col * scale, y + row * scale, scale, scale, c)
            x += 8 * scale

    def draw_text_center(self, cx, y, text, scale=1, c=(255, 255, 255)):
        w = len(text) * 8 * scale
        self.draw_text(cx - w // 2, y, text, scale, c)

    def draw_cursor(self, cx, cy):
        mask = [
            0b100000000000,
            0b110000000000,
            0b111000000000,
            0b111100000000,
            0b111110000000,
            0b111111000000,
            0b111111100000,
            0b111111110000,
            0b111111111000,
            0b111111000000,
            0b110111100000,
            0b100011110000,
            0b000001111000,
            0b000000111000,
        ]
        # Shadow
        for r, row in enumerate(mask):
            for c in range(12):
                if (row >> (11 - c)) & 1:
                    self.pixel(cx + c + 2, cy + r + 2, (10, 15, 25))
        # Body & white fill
        for r, row in enumerate(mask):
            for c in range(12):
                if (row >> (11 - c)) & 1:
                    is_edge = (c == 0 or r == 0 or not ((row >> (12 - c)) & 1))
                    col = (15, 23, 42) if is_edge else (255, 255, 255)
                    self.pixel(cx + c, cy + r, col)

    def save_ppm(self, filename):
        with open(filename, "wb") as f:
            header = f"P6\n{self.w} {self.h}\n255\n".encode("ascii")
            f.write(header)
            raw = bytearray()
            for p in self.pixels:
                raw.extend(p)
            f.write(raw)
        print(f"[Preview] Saved image: {filename}")


def render_boot_splash():
    c = Canvas(WIDTH, HEIGHT)
    c.vgradient(0, 0, WIDTH, HEIGHT, (5, 8, 20), (10, 22, 51))

    cx, cy = WIDTH // 2, HEIGHT // 2 - 30

    # Badge Logo
    c.fill_rrect(cx - 48, cy - 48, 96, 96, 24, (30, 58, 138))
    c.fill_rrect(cx - 44, cy - 44, 88, 88, 20, (37, 99, 235))
    # Y shape
    for i in range(16):
        c.fill_rect(cx - 24 + i, cy - 24 + i, 8, 8, (255, 255, 255))
        c.fill_rect(cx + 16 - i, cy - 24 + i, 8, 8, (255, 255, 255))
    c.fill_rect(cx - 4, cy - 8, 8, 28, (255, 255, 255))

    # Text
    c.draw_text_center(cx + 2, cy + 72, "YAZAN OS", 3, (2, 6, 23))
    c.draw_text_center(cx, cy + 70, "YAZAN OS", 3, TEXT_WHITE)
    c.draw_text_center(cx, cy + 102, "Fire Edition 3.0  |  Hybrid UI", 1, SKY_BLUE)

    # Progress bar
    bar_w = 400
    bar_h = 10
    bar_x = (WIDTH - bar_w) // 2
    bar_y = cy + 130
    c.fill_rrect(bar_x, bar_y, bar_w, bar_h, 5, (30, 41, 59))
    c.rect_outline(bar_x, bar_y, bar_w, bar_h, (51, 65, 85))
    c.fill_rrect(bar_x + 2, bar_y + 2, 310, bar_h - 4, 3, SKY_BLUE)

    c.draw_text_center(cx, bar_y + 22, "Loading Desktop Environment & Applications...", 1, TEXT_MUTED)
    return c


def render_desktop(start_menu=False):
    c = Canvas(WIDTH, HEIGHT)
    # Wallpaper
    c.vgradient(0, 0, WIDTH, HEIGHT // 2, WALLPAPER_TOP, WALLPAPER_MID)
    c.vgradient(0, HEIGHT // 2, WIDTH, HEIGHT - HEIGHT // 2, WALLPAPER_MID, WALLPAPER_BOTTOM)

    # Ambient subtle grid
    for x in range(0, WIDTH, 64):
        c.pixel(x, HEIGHT // 3, (30, 41, 59))

    # Desktop Icons (left side)
    icons = [
        ("Terminal", (30, 41, 59)),
        ("Calculator", (37, 99, 235)),
        ("Files", (217, 119, 6)),
        ("Notepad", (13, 148, 136)),
        ("Settings", (75, 85, 99)),
        ("Task Mgr", (124, 58, 237)),
    ]
    for i, (name, col) in enumerate(icons):
        iy = 48 + i * 92
        c.blend_rect(30, iy, 56, 56, (30, 41, 59), 180)
        c.fill_rrect(38, iy + 8, 40, 40, 8, col)
        c.draw_text_center(58 + 1, iy + 62 + 1, name, 1, (5, 7, 15))
        c.draw_text_center(58, iy + 62, name, 1, TEXT_WHITE)

    # Window 1: Terminal
    tx, ty, tw, th = 110, 56, 540, 340
    c.blend_rect(tx + 6, ty + 6, tw, th, (5, 8, 15), 140)
    c.fill_rrect(tx, ty, tw, th, 8, (10, 14, 26))
    c.fill_rrect(tx, ty, tw, 28, 8, (30, 41, 59))
    c.fill_rect(tx, ty + 24, tw, 4, (30, 41, 59))
    c.rect_outline(tx, ty, tw, th, SKY_BLUE)
    c.draw_text(tx + 12, ty + 9, ">_ Terminal - yazan@os:~", 1, TEXT_WHITE)
    # Traffic lights
    c.fill_rrect(tx + tw - 20, ty + 8, 12, 12, 6, (239, 68, 68))
    c.fill_rrect(tx + tw - 38, ty + 8, 12, 12, 6, (16, 185, 129))
    c.fill_rrect(tx + tw - 56, ty + 8, 12, 12, 6, (245, 158, 11))
    # Terminal text
    c.draw_text(tx + 14, ty + 40, "Welcome to Yazan OS 3.0 Terminal Shell!", 1, SKY_BLUE)
    c.draw_text(tx + 14, ty + 56, "Type 'help' to view available system commands.", 1, TEXT_MUTED)
    c.draw_text(tx + 14, ty + 72, "Type 'info' or 'mem' for system diagnostics.", 1, TEXT_MUTED)
    c.draw_text(tx + 14, ty + 98, "yazan@os:~$ info", 1, TEXT_WHITE)
    c.draw_text(tx + 14, ty + 116, "OS       : Yazan OS 3.0 Fire Edition (Hybrid UI)", 1, (34, 197, 94))
    c.draw_text(tx + 14, ty + 132, "Kernel   : x86_64 Long Mode Monolithic Hybrid", 1, TEXT_WHITE)
    c.draw_text(tx + 14, ty + 148, "RAM Total: 256 MB | Free: 208 MB | Usable: 256 MB", 1, SKY_BLUE)
    c.draw_text(tx + 14, ty + 164, "Drivers  : PMM, Heap, IDT, PIT, PS/2 Mouse & Keyboard", 1, TEXT_MUTED)
    c.draw_text(tx + 14, ty + 180, "UI Style : Windows 11 + macOS + Android Glass Design", 1, (245, 158, 11))
    c.draw_text(tx + 14, ty + 208, "yazan@os:~$ _", 1, SKY_BLUE)

    # Window 2: Calculator
    cx, cy, cw, ch = 680, 90, 270, 340
    c.blend_rect(cx + 6, cy + 6, cw, ch, (5, 8, 15), 140)
    c.fill_rrect(cx, cy, cw, ch, 8, (17, 24, 39))
    c.fill_rrect(cx, cy, cw, 28, 8, (30, 41, 59))
    c.fill_rect(cx, cy + 24, cw, 4, (30, 41, 59))
    c.rect_outline(cx, cy, cw, ch, (51, 65, 85))
    c.draw_text(cx + 12, cy + 9, "+- Calculator", 1, TEXT_WHITE)
    # Traffic lights
    c.fill_rrect(cx + cw - 20, cy + 8, 12, 12, 6, (239, 68, 68))
    c.fill_rrect(cx + cw - 38, cy + 8, 12, 12, 6, (16, 185, 129))
    c.fill_rrect(cx + cw - 56, cy + 8, 12, 12, 6, (245, 158, 11))
    # LCD screen
    c.fill_rrect(cx + 12, cy + 38, cw - 24, 44, 6, (31, 41, 55))
    c.rect_outline(cx + 12, cy + 38, cw - 24, 44, (55, 65, 81))
    c.draw_text(cx + cw - 90, cy + 52, "2026", 2, SKY_BLUE)
    # Calculator buttons grid
    grid = [["7", "8", "9", "/"], ["4", "5", "6", "*"], ["1", "2", "3", "-"], ["C", "0", "=", "+"]]
    bw = (cw - 44) // 4
    bh = (ch - 100) // 4
    for r in range(4):
        for col in range(4):
            bx = cx + 12 + col * (bw + 6)
            by = cy + 92 + r * (bh + 4)
            bcol = (37, 99, 235) if col == 3 or (r == 3 and col == 2) else ((220, 38, 38) if (r == 3 and col == 0) else (31, 41, 55))
            c.fill_rrect(bx, by, bw, bh, 6, bcol)
            c.rect_outline(bx, by, bw, bh, (55, 65, 81))
            c.draw_text_center(bx + bw // 2, by + bh // 2 - 4, grid[r][col], 1, TEXT_WHITE)

    # Top Status Bar (macOS / Android style)
    c.fill_rect(0, 0, WIDTH, 28, TOPBAR_BG)
    c.fill_rect(0, 27, WIDTH, 1, (30, 41, 59))
    c.fill_rrect(8, 4, 20, 20, 5, START_BLUE)
    c.draw_text(14, 7, "Y", 1, TEXT_WHITE)
    c.draw_text(34, 9, "Yazan OS", 1, TEXT_WHITE)
    c.draw_text(110, 9, "|", 1, TEXT_DIM)
    c.draw_text(124, 9, "Terminal (Active)", 1, SKY_BLUE)
    c.draw_text_center(WIDTH // 2, 9, "12:45 PM", 1, TEXT_WHITE)
    c.draw_text(WIDTH - 150, 9, "RAM: 48M/256M", 1, SKY_BLUE)
    c.fill_rrect(WIDTH - 44, 9, 8, 8, 4, (34, 197, 94))
    c.draw_text(WIDTH - 30, 9, "OK", 1, (34, 197, 94))

    # Taskbar (Windows 11 style)
    ty = HEIGHT - 44
    c.blend_rect(0, ty, WIDTH, 44, TASKBAR_BG, 240)
    c.fill_rect(0, ty, WIDTH, 1, (30, 41, 59))
    # Start button
    c.fill_rrect(14, ty + 6, 110, 32, 6, (29, 78, 216) if start_menu else START_BLUE)
    c.draw_text(24, ty + 16, "Yazan Start", 1, TEXT_WHITE)
    # Tabs
    tabs = ["Terminal", "Calculator", "Files"]
    for i, t in enumerate(tabs):
        ix = 136 + i * 126
        c.fill_rrect(ix, ty + 6, 120, 32, 6, (37, 99, 235) if i == 0 else (30, 41, 59))
        c.draw_text(ix + 12, ty + 16, t, 1, TEXT_WHITE)
        if i == 0:
            c.fill_rect(ix + 10, ty + 34, 100, 2, SKY_BLUE)

    # Start Menu popup (if toggled)
    if start_menu:
        sm_w, sm_h = 340, 390
        sm_x, sm_y = 14, HEIGHT - 44 - sm_h - 8
        c.blend_rect(sm_x + 6, sm_y + 6, sm_w, sm_h, (2, 6, 23), 160)
        c.fill_rrect(sm_x, sm_y, sm_w, sm_h, 10, (15, 23, 42))
        c.rect_outline(sm_x, sm_y, sm_w, sm_h, (51, 65, 85))

        # Profile
        c.fill_rrect(sm_x + 12, sm_y + 12, sm_w - 24, 46, 6, (30, 41, 59))
        c.fill_rrect(sm_x + 20, sm_y + 19, 32, 32, 8, START_BLUE)
        c.draw_text(sm_x + 32, sm_y + 27, "Y", 1, TEXT_WHITE)
        c.draw_text(sm_x + 62, sm_y + 20, "Yazan Administrator", 1, TEXT_WHITE)
        c.draw_text(sm_x + 62, sm_y + 36, "yazan@os.local  |  Root", 1, TEXT_MUTED)

        # Search
        c.fill_rrect(sm_x + 12, sm_y + 66, sm_w - 24, 28, 4, (30, 41, 59))
        c.draw_text(sm_x + 24, sm_y + 74, "Search apps & files...", 1, TEXT_DIM)

        # App list
        c.draw_text(sm_x + 16, sm_y + 104, "ALL APPLICATIONS", 1, SKY_BLUE)
        app_names = ["Terminal", "Calculator", "File Explorer", "Notepad", "System Settings", "Task Manager"]
        for i, a in enumerate(app_names):
            iy = sm_y + 122 + i * 34
            c.fill_rrect(sm_x + 12, iy, sm_w - 24, 30, 4, (22, 32, 50))
            c.draw_text(sm_x + 40, iy + 10, a, 1, TEXT_WHITE)

        # Bottom buttons
        c.fill_rrect(sm_x + 12, sm_y + sm_h - 40, 130, 28, 4, (127, 29, 29))
        c.draw_text(sm_x + 20, sm_y + sm_h - 31, "Test Panic RSOD", 1, (252, 165, 165))
        c.fill_rrect(sm_x + sm_w - 110, sm_y + sm_h - 40, 98, 28, 4, (30, 41, 59))
        c.draw_text(sm_x + sm_w - 90, sm_y + sm_h - 31, "Restart", 1, TEXT_WHITE)

    # Mouse pointer
    c.draw_cursor(520, 280)
    return c


def render_ram_panic():
    c = Canvas(WIDTH, HEIGHT)
    c.vgradient(0, 0, WIDTH, HEIGHT, PANIC_RED_TOP, PANIC_RED_BOTTOM)

    x, y = 40, 35
    c.draw_text(x, y, ":(  YAZAN OS - SYSTEM HALTED", 2, TEXT_WHITE)
    c.draw_text(x, y + 32, "Your computer ran into a critical hardware memory error.", 1, (252, 165, 165))

    # Red diagnostic box
    c.fill_rrect(x, y + 54, WIDTH - 80, 230, 8, (56, 5, 5))
    c.rect_outline(x, y + 54, WIDTH - 80, 230, (220, 38, 38))

    by = y + 70
    c.draw_text(x + 20, by, "STOP CODE: INSUFFICIENT_SYSTEM_RAM (0x0000003E)", 1, (253, 224, 71))
    by += 22
    c.draw_text(x + 20, by, "Detected RAM : 32 MB", 1, TEXT_WHITE)
    by += 18
    c.draw_text(x + 20, by, "Required RAM : 64 MB Minimum", 1, (74, 222, 128))
    by += 24
    c.draw_text(x + 20, by, "Arabic Diagnostics:", 1, (253, 224, 71))
    by += 18
    c.draw_text(x + 20, by, "Khata'a: Al-Thakira al-Ashwa'iya (RAM) ghayr kafiya!", 1, TEXT_WHITE)
    by += 16
    c.draw_text(x + 20, by, "Yurja ziyadat RAM fi i'dadat QEMU aw VirtualBox ila 128MB+.", 1, (252, 165, 165))

    y2 = y + 310
    c.draw_text(x, y2, "How to fix this issue:", 1, (253, 224, 71))
    c.draw_text(x, y2 + 18, "1. In QEMU: Add '-m 256M' or higher to your launch command.", 1, TEXT_WHITE)
    c.draw_text(x, y2 + 34, "2. In VirtualBox: Open VM Settings -> System -> Motherboard -> Base Memory -> Set >= 256 MB.", 1, TEXT_WHITE)
    c.draw_text(x, y2 + 50, "3. Ensure your host machine has enough free memory available.", 1, (252, 165, 165))

    return c


def render_exception_panic():
    c = Canvas(WIDTH, HEIGHT)
    c.vgradient(0, 0, WIDTH, HEIGHT, PANIC_RED_TOP, PANIC_RED_BOTTOM)

    x, y = 32, 24
    c.draw_text(x, y, ":(  YAZAN OS - KERNEL PANIC", 2, TEXT_WHITE)
    c.draw_text(x, y + 30, "The system was halted to protect your hardware and data.", 1, (252, 165, 165))

    c.draw_text(x, y + 56, "Page Fault (#PF) - vector 14, error code 0x2", 1, (253, 224, 71))
    c.draw_text(x, y + 72, "Write of non-present page at 0x0000000180000000 (kernel mode)", 1, (253, 224, 71))

    # Registers
    regs = [
        "RAX=0000000000000001  RBX=00000000deadbeef",
        "RCX=0000000180000000  RDX=0000000000000000",
        "RSI=000000000010c000  RDI=000000000010cf00",
        "RBP=0000000000112fe0  RSP=0000000000112fd0",
        "RIP=000000000010204a  FLG=0000000000010006",
        "CR2=0000000180000000  CR3=0000000000104000"
    ]
    ry = y + 96
    for r in regs:
        c.draw_text(x, ry, r, 1, TEXT_WHITE)
        ry += 16

    c.draw_text(x, ry + 10, "Call Stack Backtrace:", 1, (253, 224, 71))
    c.draw_text(x, ry + 26, "  #0  0x000000000010204a <test_fault>", 1, TEXT_WHITE)
    c.draw_text(x, ry + 42, "  #1  0x00000000001018e2 <kmain>", 1, TEXT_WHITE)
    c.draw_text(x, ry + 64, "Debug info: addr2line -e build/yazan.elf 0x10204a", 1, TEXT_MUTED)

    return c


def main():
    os.makedirs(BUILD_DIR, exist_ok=True)

    print("Generating Yazan OS 3.0 Previews with real font...")
    boot = render_boot_splash()
    boot.save_ppm(BUILD_DIR + "/preview-boot.ppm")

    desk = render_desktop(start_menu=False)
    desk.save_ppm(BUILD_DIR + "/preview-desktop.ppm")

    start = render_desktop(start_menu=True)
    start.save_ppm(BUILD_DIR + "/preview-startmenu.ppm")

    ram_panic = render_ram_panic()
    ram_panic.save_ppm(BUILD_DIR + "/preview-panic-ram.ppm")

    exc_panic = render_exception_panic()
    exc_panic.save_ppm(BUILD_DIR + "/preview-panic-fault.ppm")

    # Convert to PNG using ffmpeg
    for img in ["preview-boot", "preview-desktop", "preview-startmenu", "preview-panic-ram", "preview-panic-fault"]:
        ppm = f"{BUILD_DIR}/{img}.ppm"
        png = f"{BUILD_DIR}/{img}.png"
        os.system(f"ffmpeg -y -i {ppm} {png} >/dev/null 2>&1")

    print("[Preview] All 5 previews converted to PNG successfully!")

if __name__ == "__main__":
    main()
