# PA3 — Analizador Semántico para COOL (Laboratorio 03)

> **Compiladores 2026-II · Universidad Nacional de Colombia**  
> Profesor: Arles Rodríguez · Fecha de entrega: Sustentación 26 Nov 2026

---

## 1. Archivos entregables

| Archivo | Descripción |
|---|---|
| `semant.h` | Declaración de `ClassTable`, `MethodInfo`, `AttrInfo` |
| `semant.cc` | Implementación completa del analizador semántico |
| `cool-tree.h` | Árbol de sintaxis abstracta (AST) con extensiones |
| `cool-tree.handcode.h` | Macros de extensión inyectadas en nodos del AST |
| `good.cl` | Programa COOL semánticamente correcto (caso positivo) |
| `bad.cl` | Programa COOL con errores semánticos intencionales (caso negativo) |
| `EXPLICACION_DETALLADA.md` | Guía técnica completa para la sustentación |
| `verify_semant.py` | Validador local offline (sin VM) |

---

## 2. Verificación local (sin VM)

```bash
python verify_semant.py
```

Verifica la presencia y estructura de todos los entregables, y comprueba
léxicamente que `good.cl` y `bad.cl` contienen las construcciones esperadas.

---

## 3. Compilación y ejecución en la VM del curso

### 3.1 Preparar el directorio de trabajo

```bash
# En la VM del curso (usuario stanford o similar)
cd ~
mkdir -p lab03
cd lab03
make -f /usr/class/cs143/assignments/PA4/Makefile
```

### 3.2 Copiar los entregables

```bash
# Desde el directorio de entrega (ajustar ruta según transferencia)
cp semant.cc semant.h cool-tree.h cool-tree.handcode.h ~/lab03/
```

### 3.3 Compilar el analizador semántico

```bash
cd ~/lab03
make semant
```

> Si la compilación falla, revisar que `cool-tree.h` y `cool-tree.handcode.h`
> coincidan con los esperados por el Makefile de CS143 PA4.

### 3.4 Ejecutar con los programas de prueba

```bash
# Programa correcto — no debe producir errores
./mysemant good.cl
echo "Exit code good.cl: $?"

# Programa con errores — debe listar los errores detectados
./mysemant bad.cl
echo "Exit code bad.cl: $?"
```

**Salida esperada para `good.cl`:** sin mensajes de error, código de salida `0`.  
**Salida esperada para `bad.cl`:** mensajes de error en `stderr` para cada error
intencional, código de salida distinto de `0`.

### 3.5 Ejecutar el script de calificación

```bash
cd ~/lab03
./pa3-grading.pl
```

La nota máxima evaluada es **74/74 puntos**.

---

## 4. Programa de prueba adicional (lexer + parser + semant pipeline)

```bash
# Correr el pipeline completo del compilador
./mycoolc good.cl
./mycoolc bad.cl
```

---

## 5. Depuración

Para activar mensajes de depuración del analizador semántico:

```bash
./mysemant --semant-debug good.cl 2>&1 | head -50
```

---

## 6. Referencia rápida de errores cubiertos en `bad.cl`

| Categoría | Errores cubiertos |
|---|---|
| **A** — Clases | Duplicado, redef. básica, `SELF_TYPE` como nombre, herencia de indefinida, herencia sellada, ciclo, sin `Main`, `main()` con args |
| **B** — Atributos | `self`, duplicado, herencia redefinida, tipo indefinido, init no conforme |
| **C** — Métodos | Duplicado, params repetidos, `self` param, `SELF_TYPE` param, tipos indef, body no conforme, override inválido |
| **D** — Identificadores | Var no declarada, asign a `self`, `self` en let, tipo de let indefinido |
| **E** — Expresiones | Asign no conforme, dispatch a método indef., `new` tipo indef., `if`/`while` cond no Bool, aritmética tipo incorrecto, negación, igualdad ilegal, case duplicado/`SELF_TYPE`/tipo indef. |
