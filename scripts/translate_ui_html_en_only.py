#!/usr/bin/env python3
# Translate Chinese spans in resources/ui.html to English only (no Vietnamese).
# Requires: pip install deep-translator
import json
import re
import sys
import time
from pathlib import Path

try:
    from deep_translator import GoogleTranslator
except ImportError:
    print("Install: pip install deep-translator", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "resources" / "ui.html"
CACHE = ROOT / "scripts" / ".ui_html_zh_to_en.json"

HAN = re.compile(r"[\u4e00-\u9fff·]+")


def load_cache():
    if CACHE.exists():
        try:
            return json.loads(CACHE.read_text(encoding="utf-8"))
        except Exception:
            return {}
    return {}


def save_cache(d):
    CACHE.write_text(json.dumps(d, ensure_ascii=False, indent=0), encoding="utf-8")


def translate_en(text: str, cache: dict) -> str:
    if text in cache:
        return cache[text]
    t_en = GoogleTranslator(source="zh-CN", target="en")
    for attempt in range(4):
        try:
            en = t_en.translate(text)
            time.sleep(0.12)
            cache[text] = en
            return en
        except Exception as e:
            wait = 1.5 * (attempt + 1)
            print(f"retry {attempt+1} after error: {e}", file=sys.stderr)
            time.sleep(wait)
    raise RuntimeError(f"failed: {text[:40]}")


def main():
    s = UI.read_text(encoding="utf-8")
    found = sorted(set(HAN.findall(s)), key=len, reverse=True)
    cache = load_cache()
    missing = [x for x in found if x not in cache]
    print(
        f"unique Han spans: {len(found)}, cached: {len(found)-len(missing)}, to translate: {len(missing)}",
        file=sys.stderr,
    )
    for i, text in enumerate(missing):
        translate_en(text, cache)
        if (i + 1) % 50 == 0:
            save_cache(cache)
            print(f"... {i+1}/{len(missing)}", file=sys.stderr)
    save_cache(cache)

    out = s
    for text in found:
        rep = cache[text]
        out = out.replace(text, rep)

    if HAN.search(out):
        left = len(HAN.findall(out))
        print(f"WARNING: {left} Han spans remain", file=sys.stderr)

    UI.write_text(out, encoding="utf-8")
    print(f"Wrote {UI}", file=sys.stderr)


if __name__ == "__main__":
    main()
