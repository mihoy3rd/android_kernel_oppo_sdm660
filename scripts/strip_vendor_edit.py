#!/usr/bin/env python3
"""
Strip all VENDOR_EDIT guards from kernel source files.

Handles these comment/guard styles:
  C source:
    #ifdef VENDOR_EDIT       #ifndef VENDOR_EDIT
    #endif /* VENDOR_EDIT */ #endif //VENDOR_EDIT
  DTS files:
    //#ifdef VENDOR_EDIT         /*#ifdef VENDOR_EDIT*/
    //#endif /*VENDOR_EDIT*/     /*#endif VENDOR_EDIT */
    #ifdef VENDOR_EDIT (defconfig)

Logic:
  #ifdef VENDOR_EDIT ... #endif  -> keep content, drop guards
  #ifndef VENDOR_EDIT ... #else ... #endif -> keep else branch, drop everything else
  #ifndef VENDOR_EDIT ... #endif (no #else) -> keep content, drop guards
"""

import re
import sys
import os


def is_vendor_ifdef(line):
    """Line starts a #ifdef VENDOR_EDIT or #if defined(VENDOR_EDIT) block (any comment style)."""
    s = line.strip()
    return bool(re.match(r'^\s*(?://|/\*)?\s*#ifdef\s+VENDOR_EDIT\b', s)) or \
           bool(re.search(r'#if\s+defined\s*\(\s*VENDOR_EDIT\s*\)', s))

def is_vendor_ifndef(line):
    """Line starts a #ifndef VENDOR_EDIT block (any comment style)."""
    return bool(re.match(
        r'^\s*(?://|/\*)?\s*#ifndef\s+VENDOR_EDIT\b',
        line.strip()))

def is_vendor_endif(line):
    """Line ends a VENDOR_EDIT block: #endif ... VENDOR_EDIT (any comment style)."""
    s = line.strip()
    return bool(re.search(r'#endif\b', s)) and 'VENDOR_EDIT' in s

def is_plain_endif(line):
    """A bare #endif (no VENDOR_EDIT)."""
    s = line.strip()
    return bool(re.match(r'^\s*(?://|/\*)?\s*#endif\b', s)) and 'VENDOR_EDIT' not in s

def is_preprocessor_if(line):
    """#if, #ifdef, #ifndef (not VENDOR_EDIT)."""
    s = line.strip()
    return bool(re.match(r'^\s*(?://|/\*)?\s*#if(?:n?def)?\s', s)) and 'VENDOR_EDIT' not in s

def is_else(line):
    """#else directive."""
    return bool(re.match(r'^\s*(?://|/\*)?\s*#else\b', line.strip()))


def strip_vendor_edit(content, filepath):
    lines = content.split('\n')
    result = []
    i = 0

    while i < len(lines):
        line = lines[i]

        if is_vendor_ifdef(line) or is_vendor_ifndef(line):
            # Start of a VENDOR_EDIT block
            vendor_type = 'ifdef' if is_vendor_ifdef(line) else 'ifndef'
            depth = 1
            vendor_lines = []
            has_else = False
            else_lines = []
            after_else = False
            i += 1

            while i < len(lines) and depth > 0:
                l = lines[i]

                if is_vendor_ifdef(l) or is_vendor_ifndef(l):
                    depth += 1
                    if after_else:
                        else_lines.append(l)
                    else:
                        vendor_lines.append(l)
                    i += 1
                    continue

                if is_preprocessor_if(l):
                    depth += 1
                    if after_else:
                        else_lines.append(l)
                    else:
                        vendor_lines.append(l)
                    i += 1
                    continue

                if is_vendor_endif(l):
                    depth -= 1
                    if depth == 0:
                        break
                    if after_else:
                        else_lines.append(l)
                    else:
                        vendor_lines.append(l)
                    i += 1
                    continue

                if is_plain_endif(l):
                    depth -= 1
                    if depth == 0:
                        # This shouldn't happen in well-formed code,
                        # but handle it: treat as end of vendor block
                        break
                    if after_else:
                        else_lines.append(l)
                    else:
                        vendor_lines.append(l)
                    i += 1
                    continue

                if is_else(l) and depth == 1:
                    has_else = True
                    after_else = True
                    i += 1
                    continue

                if after_else:
                    else_lines.append(l)
                else:
                    vendor_lines.append(l)
                i += 1

            # Block ended. Decide what to keep.
            if vendor_type == 'ifdef':
                result.extend(vendor_lines)
                if has_else:
                    result.extend(else_lines)
            else:  # ifndef
                if has_else:
                    result.extend(else_lines)
                else:
                    result.extend(vendor_lines)

            i += 1
            continue

        result.append(line)
        i += 1

    return '\n'.join(result)


def process_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            original = f.read()
    except Exception as e:
        print(f"  SKIP {filepath}: {e}")
        return False

    if 'VENDOR_EDIT' not in original:
        return False

    new_content = strip_vendor_edit(original, filepath)

    if new_content != original:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False


def main():
    files = sys.argv[1:] if len(sys.argv) > 1 else \
        [line.strip() for line in sys.stdin if line.strip()]

    changed = 0
    for f in files:
        if os.path.isfile(f):
            if process_file(f):
                changed += 1
                print(f"  STRIPPED: {f}")
        else:
            print(f"  SKIP (not found): {f}")

    print(f"\nTotal: {changed} files modified")


if __name__ == '__main__':
    main()
