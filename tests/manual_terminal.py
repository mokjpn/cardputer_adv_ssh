"""Run on the SSH SERVER in a UTF-8 locale, never on the Cardputer MCU."""
import shutil
cols, rows = shutil.get_terminal_size()
print(f"PTY: {cols} x {rows}")
print("ASCII | 日本語 | ひらがな | カタカナ | ｶﾀｶﾅ ﾊﾟﾋﾟﾌﾟﾍﾟﾎﾟ")
print("漢字 全角幅。、「」〜￥")
print("X" * (cols - 1) + "日" + " <- wide wrap")
for i in range(rows * 3):
    print(f"\033[{31+i%7}m{i:03d}: 日本語のスクロール ABC 123\033[0m")
print("Use Esc to inspect scrollback; Enter returns to live terminal.")
