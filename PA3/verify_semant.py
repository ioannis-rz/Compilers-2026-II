#!/usr/bin/env python3
"""
verify_semant.py — Validador local para el Analizador Semántico de COOL (PA3/Lab03)
Universidad Nacional de Colombia · Compiladores 2026-II

Propósito:
  Verificar de forma offline (sin la VM del curso) que:
  1. good.cl contiene las construcciones COOL sintácticamente válidas que el
     analizador semántico debe aceptar.
  2. bad.cl contiene intencionalmente los errores semánticos que el analizador
     debe detectar (uno por cada categoría del Lab03).

USO:
  python verify_semant.py

Nota: Este script NO ejecuta el compilador. Realiza chequeos léxicos/sintácticos
ligeros sobre los archivos .cl para confirmar que las estructuras están presentes.
Para la evaluación real se requiere la VM del curso y `./mysemant`.
"""

import re
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GOOD_CL = os.path.join(SCRIPT_DIR, "good.cl")
BAD_CL  = os.path.join(SCRIPT_DIR, "bad.cl")

# ─── Colores ANSI ────────────────────────────────────────────────────────────
GREEN  = "\033[92m"
RED    = "\033[91m"
YELLOW = "\033[93m"
CYAN   = "\033[96m"
BOLD   = "\033[1m"
RESET  = "\033[0m"

PASS = f"{GREEN}[PASS]{RESET}"
FAIL = f"{RED}[FAIL]{RESET}"
INFO = f"{CYAN}[INFO]{RESET}"

passed = 0
failed = 0

def check(description, condition, hint=""):
    global passed, failed
    if condition:
        print(f"  {PASS} {description}")
        passed += 1
    else:
        print(f"  {FAIL} {description}")
        if hint:
            print(f"       {YELLOW}Hint: {hint}{RESET}")
        failed += 1

def section(title):
    print(f"\n{BOLD}{CYAN}{'='*60}{RESET}")
    print(f"{BOLD}{CYAN}  {title}{RESET}")
    print(f"{BOLD}{CYAN}{'='*60}{RESET}")

def load(path):
    if not os.path.exists(path):
        print(f"{RED}ERROR: Archivo no encontrado: {path}{RESET}")
        sys.exit(1)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()

# ─────────────────────────────────────────────────────────────────────────────
#  GOOD.CL  CHECKS
# ─────────────────────────────────────────────────────────────────────────────

def check_good(src):
    section("good.cl — Verificaciones de programa semánticamente correcto")

    # Clases presentes
    check("Clase 'Animal' definida",        bool(re.search(r'\bclass\s+Animal\b', src)))
    check("Clase 'Mammal' definida",         bool(re.search(r'\bclass\s+Mammal\b', src)))
    check("Clase 'Dog' definida",            bool(re.search(r'\bclass\s+Dog\b', src)))
    check("Clase 'Cat' definida",            bool(re.search(r'\bclass\s+Cat\b', src)))
    check("Clase 'Main' definida",           bool(re.search(r'\bclass\s+Main\b', src)))

    # Herencia
    check("Animal hereda de IO",             bool(re.search(r'class\s+Animal\s+inherits\s+IO', src)))
    check("Mammal hereda de Animal",         bool(re.search(r'class\s+Mammal\s+inherits\s+Animal', src)))
    check("Dog hereda de Mammal",            bool(re.search(r'class\s+Dog\s+inherits\s+Mammal', src)))
    check("Cat hereda de Mammal",            bool(re.search(r'class\s+Cat\s+inherits\s+Mammal', src)))

    # SELF_TYPE legal: retorno de método
    check("SELF_TYPE como tipo de retorno de método (init)",
          bool(re.search(r'init\s*\(.*?\)\s*:\s*SELF_TYPE', src, re.DOTALL)))

    # SELF_TYPE legal: tipo de atributo
    check("SELF_TYPE como tipo de atributo (best_friend)",
          bool(re.search(r'best_friend\s*:\s*SELF_TYPE', src)))

    # SELF_TYPE legal: tipo de variable let (puede estar en binding de let multi-línea)
    check("SELF_TYPE como tipo de variable let",
          bool(re.search(r'let\s+\w+\s*:\s*SELF_TYPE', src)) or
          bool(re.search(r'\w+\s*:\s*SELF_TYPE\s*<-', src)))

    # SELF_TYPE legal: new SELF_TYPE
    check("'new SELF_TYPE' presente",
          bool(re.search(r'\bnew\s+SELF_TYPE\b', src)))

    # Despacho estático
    check("Despacho estático '@' presente",
          bool(re.search(r'@\w+\.', src)))

    # Expresión case
    check("Expresión 'case ... esac' presente",
          bool(re.search(r'\bcase\b.*?\besac\b', src, re.DOTALL)))

    # Bucle while
    check("Bucle 'while ... pool' presente",
          bool(re.search(r'\bwhile\b.*?\bpool\b', src, re.DOTALL)))

    # Let con múltiples variables (LUB)
    check("Expresión 'let' con múltiples bindings",
          bool(re.search(r'\blet\b.*?,', src, re.DOTALL)))

    # Método main sin parámetros
    check("Método 'main()' sin parámetros en Main",
          bool(re.search(r'\bmain\s*\(\s*\)\s*:', src)))

    # Expresiones aritméticas y comparaciones
    check("Operación aritmética '+' presente",
          bool(re.search(r'\d\s*\+\s*\d|\w\s*\+\s*\d', src)))
    check("Comparación '<' presente",
          bool(re.search(r'[^<]<[^<-]', src)))

    # isvoid
    check("Expresión 'isvoid' presente",
          bool(re.search(r'\bisvoid\b', src)))

    # Métodos de Object: copy, type_name
    check("Llamada a '.copy()' presente",
          bool(re.search(r'\.copy\(\)', src)))
    check("Llamada a '.type_name()' presente",
          bool(re.search(r'\.type_name\(\)', src)))

    # Método definido más de una vez en subclase (override válido)
    check("Override válido de 'speak()' en Dog",
          src.count('speak()') >= 3,
          "speak() debería estar en Animal, Mammal y Dog")


# ─────────────────────────────────────────────────────────────────────────────
#  BAD.CL  CHECKS
# ─────────────────────────────────────────────────────────────────────────────

def check_bad(src):
    section("bad.cl — Verificaciones de errores semánticos intencionales")

    # ── Categoría A: Clases y grafo de herencia ──────────────────────────────
    print(f"\n  {BOLD}Categoría A: Clases y Grafo de Herencia{RESET}")

    check("A.1 — Clase duplicada (DuplicateClass × 2)",
          src.count("class DuplicateClass") >= 2,
          "Debe haber dos definiciones de DuplicateClass")

    check("A.2 — Redefinición de clase básica Int",
          bool(re.search(r'class\s+Int\b', src)),
          "Debe redefinir la clase Int")

    check("A.3 — Clase llamada SELF_TYPE",
          bool(re.search(r'class\s+SELF_TYPE\b', src)),
          "Debe definir una clase llamada SELF_TYPE")

    check("A.4 — Herencia de clase indefinida (Inexistente)",
          bool(re.search(r'inherits\s+Inexistente\b', src)),
          "Debe heredar de una clase no definida")

    check("A.5 — Herencia de clase sellada (Bool o String)",
          bool(re.search(r'inherits\s+(Bool|String)\b', src)),
          "Debe heredar de Bool o String")

    check("A.6 — Ciclo en herencia (CycleA ↔ CycleB)",
          bool(re.search(r'class\s+CycleA', src)) and bool(re.search(r'class\s+CycleB', src)),
          "Debe haber ciclo CycleA <-> CycleB")

    check("A.7/A.8 — Main con main() con parámetros",
          bool(re.search(r'main\s*\(\s*\w+\s*:\s*\w+', src)),
          "main() no debe tener parámetros")

    # ── Categoría B: Atributos ───────────────────────────────────────────────
    print(f"\n  {BOLD}Categoría B: Atributos{RESET}")

    check("B.1 — Atributo llamado 'self'",
          bool(re.search(r'\bself\s*:\s*\w+\s*;', src)),
          "Debe declarar un atributo llamado self")

    check("B.2 — Atributo duplicado en la misma clase (dup_attr)",
          src.count("dup_attr") >= 2,
          "dup_attr debe aparecer al menos 2 veces")

    check("B.3 — Atributo heredado redefinido (inherited_attr)",
          bool(re.search(r'inherited_attr\s*:\s*Int', src)),
          "inherited_attr debe redefinirse en la subclase")

    check("B.4 — Atributo de tipo indefinido (TipoDesconocido)",
          bool(re.search(r':\s*TipoDesconocido\b', src)),
          "Debe usar un tipo no definido")

    check("B.5 — Inicialización de atributo Int con String literal",
          bool(re.search(r':\s*Int\s*<-\s*\"', src)),
          "Debe inicializar un Int con un String")

    # ── Categoría C: Métodos ─────────────────────────────────────────────────
    print(f"\n  {BOLD}Categoría C: Métodos{RESET}")

    check("C.1 — Método duplicado en la misma clase (unique_meth × 2)",
          src.count("unique_meth") >= 2,
          "unique_meth debe definirse dos veces en la misma clase")

    check("C.2 — Parámetros formales con nombre repetido (dup_params)",
          bool(re.search(r'dup_params\s*\(', src)),
          "dup_params(p: Int, p: String) debe existir")

    check("C.3 — Parámetro formal llamado 'self'",
          bool(re.search(r'\(\s*self\s*:\s*\w+', src)),
          "Debe pasar 'self' como nombre de parámetro formal")

    check("C.4 — Parámetro formal de tipo SELF_TYPE",
          bool(re.search(r':\s*SELF_TYPE\s*\)', src)),
          "Debe usar SELF_TYPE como tipo de parámetro formal")

    check("C.5 — Tipo de retorno/parámetro indefinido (RetornoRaro / TipoRaro)",
          bool(re.search(r'(RetornoRaro|TipoRaro)', src)),
          "Debe usar tipos de retorno/parámetro no definidos")

    check("C.6 — Cuerpo de método cuyo tipo no conforma con el retorno (Int ← String)",
          bool(re.search(r'mismatch_body\s*\(\s*\)\s*:\s*Int\s*\{', src)),
          "mismatch_body():Int con cuerpo String debe existir")

    check("C.7 — Override con firma incompatible (foo redefinido con 1 arg en vez de 2)",
          src.count("foo(") >= 2,
          "foo() debe redefinirse con firma diferente")

    # ── Categoría D: Identificadores y alcance ───────────────────────────────
    print(f"\n  {BOLD}Categoría D: Identificadores y Alcance{RESET}")

    check("D.1 — Identificador no declarado (variable_fantasma)",
          bool(re.search(r'variable_fantasma', src)),
          "Debe usar un identificador no declarado")

    check("D.2 — Asignación a 'self'",
          bool(re.search(r'\bself\s*<-\s*', src)),
          "Debe asignar a self")

    check("D.3 — 'self' como variable en let",
          bool(re.search(r'\blet\s+self\s*:', src)),
          "Debe usar self como variable de let")

    check("D.4 — Tipo de let indefinido (TipoInexistente)",
          bool(re.search(r'let\s+\w+\s*:\s*TipoInexistente', src)),
          "Debe declarar variable let con tipo no definido")

    # ── Categoría E: Expresiones ─────────────────────────────────────────────
    print(f"\n  {BOLD}Categoría E: Expresiones{RESET}")

    check("E.1 — Asignación cuyo tipo no conforma (Int ← String)",
          bool(re.search(r'x\s*<-\s*\"cadena\"', src)),
          "Debe asignar String a un Int")

    check("E.4 — 'new' con tipo indefinido (ClaseInexistente)",
          bool(re.search(r'new\s+ClaseInexistente\b', src)),
          "Debe usar new con tipo no definido")

    check("E.5 — Condición de if no booleana (Int)",
          bool(re.search(r'if\s+\d+\s*\+\s*\d+', src)),
          "Debe usar expresión Int como condición de if")

    check("E.5 — Condición de while no booleana (String literal)",
          bool(re.search(r'while\s+\"', src)),
          "Debe usar String literal como condición de while")

    check("E.6 — Operando aritmético no Int (String + String o Bool)",
          bool(re.search(r'10\s*\+\s*\"', src)) or bool(re.search(r'true\s*<\s*5', src)),
          "Debe usar tipos no-Int en operación aritmética")

    check("E.7 — Negación de no-Int (~\"cadena\")",
          bool(re.search(r'~\s*\"', src)),
          "Debe negar un String")

    check("E.7 — 'not' de no-Bool (not 42)",
          bool(re.search(r'\bnot\s+\d', src)),
          "Debe aplicar 'not' a un Int")

    check("E.8 — Igualdad ilegal entre tipos básicos (5 = \"cinco\")",
          bool(re.search(r'\d\s*=\s*\"', src)),
          "Debe comparar Int con String con '='")

    check("E.9 — Case con rama de tipo duplicado (Int × 2)",
          bool(re.search(r':\s*Int\s*=>', src)) and src.count(": Int =>") >= 2,
          "Debe haber dos ramas de tipo Int en case")

    check("E.9 — Case con SELF_TYPE como tipo de rama",
          bool(re.search(r':\s*SELF_TYPE\s*=>', src)),
          "Debe usar SELF_TYPE como tipo de rama en case")

    check("E.3 — Dispatch a método inexistente",
          bool(re.search(r'metodo_inexistente', src)),
          "Debe llamar a un método que no existe")


# ─────────────────────────────────────────────────────────────────────────────
#  VERIFICACIONES ESTRUCTURALES DE LOS ENTREGABLES C++
# ─────────────────────────────────────────────────────────────────────────────

def check_deliverables():
    section("Entregables C++ — Verificación de presencia y estructura")

    files = {
        "semant.h":              os.path.join(SCRIPT_DIR, "semant.h"),
        "semant.cc":             os.path.join(SCRIPT_DIR, "semant.cc"),
        "cool-tree.h":           os.path.join(SCRIPT_DIR, "cool-tree.h"),
        "cool-tree.handcode.h":  os.path.join(SCRIPT_DIR, "cool-tree.handcode.h"),
    }

    for name, path in files.items():
        check(f"Archivo '{name}' existe", os.path.exists(path))

    if not os.path.exists(files["semant.h"]):
        return

    # semant.h
    sh = load(files["semant.h"])
    check("semant.h — Clase 'ClassTable' declarada",
          bool(re.search(r'\bclass\s+ClassTable\b', sh)))
    check("semant.h — Struct 'MethodInfo' declarado",
          bool(re.search(r'\bstruct\s+MethodInfo\b', sh)))
    check("semant.h — Struct 'AttrInfo' declarado",
          bool(re.search(r'\bstruct\s+AttrInfo\b', sh)))
    check("semant.h — Método 'conforms' declarado",
          bool(re.search(r'\bconforms\b', sh)))
    check("semant.h — Método 'lub' declarado",
          bool(re.search(r'\blub\b', sh)))
    check("semant.h — Método 'lookup_method' declarado",
          bool(re.search(r'\blookup_method\b', sh)))
    check("semant.h — Método 'check_inheritance' declarado",
          bool(re.search(r'\bcheck_inheritance\b', sh)))

    # semant.cc
    sc = load(files["semant.cc"])
    check("semant.cc — 'initialize_constants' implementada",
          bool(re.search(r'\binitialize_constants\b', sc)))
    check("semant.cc — 'install_basic_classes' implementada",
          bool(re.search(r'\binstall_basic_classes\b', sc)))
    check("semant.cc — 'check_inheritance' implementada",
          bool(re.search(r'\bcheck_inheritance\b', sc)))
    check("semant.cc — 'build_feature_tables' implementada",
          bool(re.search(r'\bbuild_feature_tables\b', sc)))
    check("semant.cc — 'check_overrides' implementada",
          bool(re.search(r'\bcheck_overrides\b', sc)))
    check("semant.cc — 'check_types' implementada",
          bool(re.search(r'\bcheck_types\b', sc)))
    check("semant.cc — 'conforms' implementada",
          bool(re.search(r'\bClassTable::conforms\b', sc)))
    check("semant.cc — 'lub' implementada",
          bool(re.search(r'\bClassTable::lub\b', sc)))
    check("semant.cc — type_check para 'assign_class' implementado",
          bool(re.search(r'assign_class.*type_check|type_check.*assign', sc, re.DOTALL)))
    check("semant.cc — type_check para 'dispatch_class' implementado",
          bool(re.search(r'dispatch_class.*type_check|type_check.*dispatch', sc, re.DOTALL)))
    check("semant.cc — type_check para 'cond_class' implementado",
          bool(re.search(r'cond_class.*type_check|type_check.*cond', sc, re.DOTALL)))
    check("semant.cc — type_check para 'typcase_class' implementado",
          bool(re.search(r'typcase_class.*type_check|type_check.*typcase', sc, re.DOTALL)))
    check("semant.cc — type_check para 'let_class' implementado",
          bool(re.search(r'let_class.*type_check|type_check.*let_class', sc, re.DOTALL)))
    check("semant.cc — 'program_class::semant' implementado",
          bool(re.search(r'program_class::semant\b', sc)))

    # cool-tree.handcode.h
    hc = load(files["cool-tree.handcode.h"])
    check("cool-tree.handcode.h — 'Expression_SHARED_EXTRAS' con 'type_check'",
          bool(re.search(r'Expression_SHARED_EXTRAS', hc)) and bool(re.search(r'type_check', hc)))
    check("cool-tree.handcode.h — 'set_type' y 'get_type' definidos",
          bool(re.search(r'set_type', hc)) and bool(re.search(r'get_type', hc)))


# ─────────────────────────────────────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    print(f"\n{BOLD}╔══════════════════════════════════════════════════════════════╗")
    print(f"║  Verificador Local — Lab03: Analizador Semántico COOL        ║")
    print(f"║  Compiladores 2026-II · Universidad Nacional de Colombia      ║")
    print(f"╚══════════════════════════════════════════════════════════════╝{RESET}\n")

    good_src = load(GOOD_CL)
    bad_src  = load(BAD_CL)

    check_deliverables()
    check_good(good_src)
    check_bad(bad_src)

    section("Resumen Final")
    total = passed + failed
    pct   = int(100 * passed / total) if total > 0 else 0
    color = GREEN if failed == 0 else (YELLOW if pct >= 70 else RED)
    print(f"\n  Checks pasados: {color}{passed}/{total} ({pct}%){RESET}")
    if failed == 0:
        print(f"\n  {GREEN}{BOLD}✔ Todos los checks pasaron. Los entregables parecen completos.{RESET}")
        print(f"  {INFO} Recuerda compilar y ejecutar en la VM del curso para validación final.")
    else:
        print(f"\n  {RED}{BOLD}✘ {failed} check(s) fallaron — revisa los archivos indicados.{RESET}")

    print()
    sys.exit(0 if failed == 0 else 1)
