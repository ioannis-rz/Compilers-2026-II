#!/usr/bin/env python3
"""
Simulador y Validador del Analizador Léxico de COOL
Permite verificar localmente el comportamiento de los tokens y manejo de errores
de acuerdo con la especificación del Manual de COOL (CS143).
"""

import sys
import re

KEYWORDS = {
    'class', 'else', 'fi', 'if', 'in', 'inherits', 'isvoid', 'let',
    'loop', 'pool', 'then', 'while', 'case', 'esac', 'new', 'of', 'not'
}

SPECIAL_SYMBOLS = {
    '=>': 'DARROW',
    '<-': 'ASSIGN',
    '<=': 'LE',
    '+': '+', '-': '-', '*': '*', '/': '/', '~': '~',
    '<': '<', '=': '=', '.': '.', ';': ';', ':': ':',
    ',': ',', '(': '(', ')': ')', '{': '{', '}': '}', '@': '@'
}

def tokenize_cool(text):
    tokens = []
    i = 0
    n = len(text)
    lineno = 1

    while i < n:
        c = text[i]

        # 1. Newline
        if c == '\n':
            lineno += 1
            i += 1
            continue

        # 2. Whitespace
        if c in ' \t\r\f\v':
            i += 1
            continue

        # 3. Single-line comment (-- ... \n)
        if text[i:i+2] == '--':
            i += 2
            while i < n and text[i] != '\n':
                i += 1
            continue

        # 4. Multi-line nested comment (* ... *)
        if text[i:i+2] == '(*':
            start_line = lineno
            depth = 1
            i += 2
            while i < n and depth > 0:
                if text[i:i+2] == '(*':
                    depth += 1
                    i += 2
                elif text[i:i+2] == '*)':
                    depth -= 1
                    i += 2
                elif text[i] == '\n':
                    lineno += 1
                    i += 1
                else:
                    i += 1
            if depth > 0:
                tokens.append((lineno, 'ERROR', 'EOF in comment'))
            continue

        # 5. Unmatched *)
        if text[i:i+2] == '*)':
            tokens.append((lineno, 'ERROR', 'Unmatched *)'))
            i += 2
            continue

        # 6. Strings
        if c == '"':
            i += 1
            buf = []
            has_error = False
            err_msg = ""
            while i < n:
                ch = text[i]
                if ch == '"':
                    i += 1
                    break
                elif ch == '\0':
                    has_error = True
                    err_msg = "String contains null character"
                    i += 1
                    # consume till end of string
                    while i < n and text[i] != '"' and text[i] != '\n':
                        if text[i:i+2] == '\\\n':
                            lineno += 1
                            i += 2
                        elif text[i] == '\\' and i + 1 < n:
                            i += 2
                        else:
                            i += 1
                    if i < n and text[i] == '\n':
                        lineno += 1
                        i += 1
                    elif i < n and text[i] == '"':
                        i += 1
                    break
                elif ch == '\n':
                    lineno += 1
                    i += 1
                    has_error = True
                    err_msg = "Unterminated string constant"
                    break
                elif ch == '\\':
                    if i + 1 >= n:
                        has_error = True
                        err_msg = "EOF in string constant"
                        i += 1
                        break
                    next_ch = text[i+1]
                    if next_ch == 'b':
                        buf.append('\b')
                        i += 2
                    elif next_ch == 't':
                        buf.append('\t')
                        i += 2
                    elif next_ch == 'n':
                        buf.append('\n')
                        i += 2
                    elif next_ch == 'f':
                        buf.append('\f')
                        i += 2
                    elif next_ch == '0':
                        buf.append('0')
                        i += 2
                    elif next_ch == '\n':
                        lineno += 1
                        buf.append('\n')
                        i += 2
                    elif next_ch == '\0':
                        has_error = True
                        err_msg = "String contains null character"
                        i += 2
                        while i < n and text[i] != '"' and text[i] != '\n':
                            if text[i:i+2] == '\\\n':
                                lineno += 1
                                i += 2
                            elif text[i] == '\\' and i + 1 < n:
                                i += 2
                            else:
                                i += 1
                        if i < n and text[i] == '\n':
                            lineno += 1
                            i += 1
                        elif i < n and text[i] == '"':
                            i += 1
                        break
                    else:
                        buf.append(next_ch)
                        i += 2
                else:
                    buf.append(ch)
                    i += 1
                if len(buf) > 1024 and not has_error:
                    has_error = True
                    err_msg = "String constant too long"
                    while i < n and text[i] != '"' and text[i] != '\n':
                        if text[i:i+2] == '\\\n':
                            lineno += 1
                            i += 2
                        elif text[i] == '\\' and i + 1 < n:
                            i += 2
                        else:
                            i += 1
                    if i < n and text[i] == '\n':
                        lineno += 1
                        i += 1
                    elif i < n and text[i] == '"':
                        i += 1
                    break
            else:
                if not has_error:
                    has_error = True
                    err_msg = "EOF in string constant"

            if has_error:
                tokens.append((lineno, 'ERROR', err_msg))
            else:
                tokens.append((lineno, 'STR_CONST', "".join(buf)))
            continue

        # 7. Two-character operators
        two_char = text[i:i+2]
        if two_char in ('=>', '<-', '<='):
            tokens.append((lineno, SPECIAL_SYMBOLS[two_char], two_char))
            i += 2
            continue

        # 8. Single-character operators
        if c in SPECIAL_SYMBOLS:
            tokens.append((lineno, SPECIAL_SYMBOLS[c], c))
            i += 1
            continue

        # 9. Integer literals
        if c.isdigit():
            m = re.match(r'^[0-9]+', text[i:])
            val = m.group(0)
            tokens.append((lineno, 'INT_CONST', val))
            i += len(val)
            continue

        # 10. Identifiers / Keywords / Booleans
        if ('a' <= c <= 'z') or ('A' <= c <= 'Z'):
            m = re.match(r'^[a-zA-Z0-9_]+', text[i:])
            ident = m.group(0)
            lower = ident.lower()

            if lower == 'true' and ident[0] == 't':
                tokens.append((lineno, 'BOOL_CONST', 'true'))
            elif lower == 'false' and ident[0] == 'f':
                tokens.append((lineno, 'BOOL_CONST', 'false'))
            elif lower in KEYWORDS:
                tokens.append((lineno, lower.upper(), ident))
            elif ident[0].isupper():
                tokens.append((lineno, 'TYPEID', ident))
            elif ident[0].islower():
                tokens.append((lineno, 'OBJECTID', ident))
            else:
                tokens.append((lineno, 'ERROR', ident[0]))
                i += 1
                continue
            i += len(ident)
            continue

        # 11. Invalid characters
        tokens.append((lineno, 'ERROR', c))
        i += 1

    return tokens

def main():
    if len(sys.argv) < 2:
        print("Uso: python verify_lexer.py <archivo.cl>")
        sys.exit(1)
    
    filename = sys.argv[1]
    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    tokens = tokenize_cool(content)
    print(f"--- Tokens reconocidos en '{filename}' ({len(tokens)} tokens) ---")
    for lineno, tok_type, val in tokens:
        if tok_type == 'ERROR':
            print(f"#{lineno} ERROR \"{val}\"")
        elif tok_type in ('INT_CONST', 'TYPEID', 'OBJECTID', 'STR_CONST'):
            # escapar caracteres especiales para visualización
            esc_val = repr(val)[1:-1] if tok_type == 'STR_CONST' else val
            print(f"#{lineno} {tok_type} = {esc_val}")
        elif tok_type == 'BOOL_CONST':
            print(f"#{lineno} BOOL_CONST = {val}")
        else:
            print(f"#{lineno} {tok_type}")

if __name__ == '__main__':
    main()
