# Explicación Detallada — Laboratorio 03: Analizador Semántico para COOL

> **Compiladores 2026-II · Universidad Nacional de Colombia**  
> Profesor: Arles Rodríguez · Sustentación grupal: **26 de noviembre de 2026**

---

## Tabla de Contenido

1. [Visión general de la arquitectura](#1-visión-general-de-la-arquitectura)
2. [Archivo `semant.h` — Cabecera del analizador](#2-archivo-semanth)
3. [Archivo `cool-tree.handcode.h` — Extensiones del AST](#3-archivo-cool-treehandcodeh)
4. [Archivo `cool-tree.h` — Árbol de Sintaxis Abstracta](#4-archivo-cool-treeh)
5. [Archivo `semant.cc` — Implementación completa](#5-archivo-semantcc)
   - 5.1 Símbolos globales e `initialize_constants`
   - 5.2 `install_basic_classes` — clases predefinidas de COOL
   - 5.3 `install_classes` — clases del usuario
   - 5.4 `check_inheritance` — validación del grafo de herencia
   - 5.5 `check_main` — verificación de la clase Main
   - 5.6 `build_feature_tables` — Pasada 1: recolección de firmas
   - 5.7 `check_overrides` — reglas de sobreescritura
   - 5.8 `check_types` — Pasada 2: comprobación de tipos
   - 5.9 `conforms` — relación de conformidad (subtipado)
   - 5.10 `lub` — cómputo del Least Upper Bound (Join)
   - 5.11 `type_check` — reglas de tipado por expresión (24 constructores)
6. [Archivos de prueba: `good.cl` y `bad.cl`](#6-archivos-de-prueba)
7. [Reglas del sistema de tipos de COOL (notación formal)](#7-reglas-del-sistema-de-tipos-de-cool)
8. [Banco de preguntas y respuestas para la sustentación](#8-banco-de-preguntas-y-respuestas)

---

## 1. Visión General de la Arquitectura

El analizador semántico de COOL recibe el Árbol de Sintaxis Abstracta (AST) producido
por el parser y lo anota con información de tipos. Si detecta errores estáticos, los
reporta todos sin abortar prematuramente, aplicando *error recovery* (tipo `Object`
como tipo de recuperación).

### Flujo de ejecución

```
Entrada: AST (program_class)
         │
         ▼
program_class::semant()
  │
  ├─► initialize_constants()           ← Inicializa símbolos globales (idtable)
  │
  └─► new ClassTable(classes)
        │
        ├─► install_basic_classes()    ← Object, IO, Int, Bool, String
        ├─► install_classes()          ← Clases del usuario
        ├─► check_inheritance()        ← Grafo de herencia válido y acíclico
        ├─► check_main()               ← Clase Main con main() sin parámetros
        ├─► build_feature_tables()     ← Pasada 1: colectar métodos y atributos
        ├─► check_overrides()          ← Reglas de sobreescritura
        └─► check_types()              ← Pasada 2: type_check en cada expresión
              │
              └─► Expression::type_check() — recursivo sobre el AST
```

> [!IMPORTANT]
> Si `check_inheritance` detecta errores, el constructor aborta antes de las siguientes
> fases porque sin un grafo válido las demás verificaciones producirían falsos errores
> en cascada.

### Componentes principales

| Componente | Responsabilidad |
|---|---|
| `ClassTable` | Grafo de herencia, tablas de métodos/atributos, orquestación de pasadas |
| `MethodInfo` | Firma completa de un método (nombre, tipos de parámetros, tipo de retorno, clase) |
| `AttrInfo` | Declaración de atributo (nombre, tipo, inicialización, clase) |
| `conforms(T1, T2, C)` | Verifica si T1 ≤ T2 en el orden parcial del árbol de herencia |
| `lub(T1, T2, C)` | Mínimo ancestro común (join) de dos tipos en el árbol de herencia |
| `type_check(ct, cur, O)` | Regla de tipado de cada nodo del AST; anota el nodo con su tipo |

---

## 2. Archivo `semant.h`

**Propósito:** Declarar la interfaz pública del analizador semántico, incluyendo las
estructuras de datos que usa `semant.cc` y que el compilador incluye para vincular.

### `MethodInfo`

```cpp
struct MethodInfo {
    Symbol name;                        // Nombre del método
    std::vector<Symbol> formals_types;  // Tipo de cada parámetro formal
    std::vector<Symbol> formals_names;  // Nombre de cada parámetro formal
    Symbol return_type;                 // Tipo de retorno declarado
    Class_ defining_class;             // Clase donde fue declarado
};
```

Almacena la firma completa de un método para ser consultada durante el despacho
y la verificación de overrides.

### `AttrInfo`

```cpp
struct AttrInfo {
    Symbol name;            // Nombre del atributo
    Symbol type_decl;       // Tipo declarado
    Expression init;        // Expresión de inicialización (puede ser no_expr)
    Class_ defining_class;  // Clase donde fue declarado
};
```

### `ClassTable`

Clase central del analizador. Mantiene:

- `name_to_class`: mapeo `Symbol → Class_` para todas las clases (básicas + usuario).
- `parent_map`: mapeo `Symbol → Symbol` para recuperar el padre de cualquier clase.
- `user_classes`: lista ordenada de las clases definidas por el usuario.
- `class_methods`: tabla bidimensional `clase → método_name → MethodInfo`.
- `class_attributes`: tabla bidimensional `clase → atrib_name → AttrInfo`.

---

## 3. Archivo `cool-tree.handcode.h`

**Propósito:** Inyectar métodos y campos adicionales en los nodos del AST mediante
macros de C++. El sistema de generación de código de CS143 (`cool-tree.cc` generado
desde `cool-tree.aps`) expande estas macros en cada clase del AST.

### Macros clave

| Macro | Dónde se expande | Qué inyecta |
|---|---|---|
| `Expression_SHARED_EXTRAS` | Clase abstracta `Expression_class` | Campo `type`, métodos `get_type()`, `set_type()`, declaración `virtual type_check(...)` |
| `Expression_EXTRAS` | Cada subclase concreta de expresión | Declaración `type_check(...)` concreta |
| `Program_EXTRAS` / `program_EXTRAS` | `Program_class` / `program_class` | Declaración/implementación de `semant()` |
| `Class__EXTRAS` / `class__EXTRAS` | `Class__class` / `class__class` | Accesores `get_name()`, `get_parent()`, `get_filename()`, `get_features()` |
| `Formal_EXTRAS` / `formal_EXTRAS` | Parámetros formales | Accesores `get_name()`, `get_type_decl()` |
| `Case_EXTRAS` / `branch_EXTRAS` | Ramas de case | Accesores `get_name()`, `get_type_decl()`, `get_expr()` |

### Firma de `type_check`

```cpp
Symbol type_check(ClassTable* ct, Class_ cur_class,
                  SymbolTable<Symbol, Entry>& o_table);
```

- **`ct`**: puntero a la `ClassTable` global (acceso a conformidad, LUB, tablas).
- **`cur_class`**: clase que actualmente se analiza (necesaria para resolver `SELF_TYPE`).
- **`o_table`**: tabla de símbolos de objetos (ambiente *O* de la semántica formal).
- **Retorno**: el `Symbol` que representa el tipo inferido de la expresión.

---

## 4. Archivo `cool-tree.h`

Contiene la definición de todas las clases del AST de COOL. Fue generado
(con ayuda del sistema `tree.cc` / `tree.aps` de CS143) y extendido con las
macros de `cool-tree.handcode.h`.

**Clases del AST relevantes para el analizador semántico:**

| Clase | Constructor COOL | Campos principales |
|---|---|---|
| `program_class` | — | `classes: Classes` |
| `class__class` | `class C inherits P { ... }` | `name, parent, features, filename` |
| `method_class` | `m(formals): T { expr }` | `name, formals, return_type, expr` |
| `attr_class` | `a: T <- expr` | `name, type_decl, init` |
| `assign_class` | `x <- e` | `name, expr` |
| `dispatch_class` | `e.m(args)` | `expr, name, actual` |
| `static_dispatch_class` | `e@T.m(args)` | `expr, type_name, name, actual` |
| `cond_class` | `if p then t else e fi` | `pred, then_exp, else_exp` |
| `loop_class` | `while p loop b pool` | `pred, body` |
| `typcase_class` | `case e of branches esac` | `expr, cases` |
| `block_class` | `{ e1; e2; ... }` | `body` |
| `let_class` | `let x:T <- e in body` | `identifier, type_decl, init, body` |
| `plus_class` | `e1 + e2` | `e1, e2` |
| `sub_class` | `e1 - e2` | `e1, e2` |
| `mul_class` | `e1 * e2` | `e1, e2` |
| `divide_class` | `e1 / e2` | `e1, e2` |
| `neg_class` | `~e` | `e1` |
| `lt_class` | `e1 < e2` | `e1, e2` |
| `leq_class` | `e1 <= e2` | `e1, e2` |
| `eq_class` | `e1 = e2` | `e1, e2` |
| `comp_class` | `not e` | `e1` |
| `int_const_class` | `42` | `token` |
| `bool_const_class` | `true / false` | `val` |
| `string_const_class` | `"hola"` | `token` |
| `new__class` | `new T` | `type_name` |
| `isvoid_class` | `isvoid e` | `e1` |
| `no_expr_class` | *(inicializador vacío)* | — |
| `object_class` | `x` (identificador) | `name` |

---

## 5. Archivo `semant.cc`

### 5.1 Símbolos globales e `initialize_constants`

```cpp
static Symbol arg, arg2, Bool, concat, cool_abort, copy, Int, ...
              Str, str_field, substr, type_name, val;

static void initialize_constants(void) {
    Bool = idtable.add_string("Bool");
    Int  = idtable.add_string("Int");
    Str  = idtable.add_string("String");   // ← el símbolo es "Str" pero el string es "String"
    ...
}
```

> [!NOTE]
> En CS143, `typedef Entry* Symbol`. La función `idtable.add_string(s)` devuelve un
> puntero `Entry*` al registro de la cadena `s` en la tabla de cadenas. Dos llamadas
> con la misma cadena devuelven **el mismo puntero**, por lo que comparar símbolos con
> `==` es equivalente a comparar cadenas.
>
> `Str` es el nombre interno C++ del símbolo de tipo `String`. En el código COOL
> del usuario se escribe `String`, pero en el C++ del analizador se usa el alias `Str`.

---

### 5.2 `install_basic_classes`

Instala las 5 clases predefinidas de COOL en `name_to_class` y `parent_map`:

| Clase COOL | Padre | Métodos/atributos instalados |
|---|---|---|
| `Object` | `_no_class` | `abort():Object`, `type_name():String`, `copy():SELF_TYPE` |
| `IO` | `Object` | `out_string(String):SELF_TYPE`, `out_int(Int):SELF_TYPE`, `in_string():String`, `in_int():Int` |
| `Int` | `Object` | Atributo interno `_val:_prim_slot` |
| `Bool` | `Object` | Atributo interno `_val:_prim_slot` |
| `String` | `Object` | `length():Int`, `concat(String):String`, `substr(Int,Int):String` |

Los cuerpos de los métodos básicos son `no_expr()` porque son nativos; sólo importan sus
firmas para el análisis semántico.

---

### 5.3 `install_classes`

Registra cada clase del usuario. Verifica antes de registrar:

1. **A.3** El nombre no puede ser `SELF_TYPE`.
2. **A.2** El nombre no puede coincidir con una clase básica (`Object`, `IO`, `Int`, `Bool`, `String`).
3. **A.1** El nombre no puede estar ya registrado (clase duplicada).

---

### 5.4 `check_inheritance`

Para cada clase de usuario verifica que su padre cumpla:

| Error | Condición detectada |
|---|---|
| **A.5** | El padre es `Int`, `String`, `Bool` o `SELF_TYPE` (clases selladas) |
| **A.4** | El padre no existe en `name_to_class` |
| **A.6** | Hay un ciclo: se recorre la cadena de padres marcando visitados; si se encuentra un nodo ya visitado, hay ciclo |

El algoritmo de detección de ciclos usa un `set<Symbol> visited` inicializado con la
clase actual. Recorre la cadena de herencia hasta `Object` o hasta encontrar un padre
desconocido; si en algún punto el padre ya está en `visited`, hay ciclo.

---

### 5.5 `check_main`

Verifica dos reglas:

- **A.7**: La clase `Main` debe existir.
- **A.8**: La clase `Main` debe definir directamente el método `main()` sin parámetros formales.

---

### 5.6 `build_feature_tables` — Pasada 1

Recorre todas las clases (primero básicas, luego usuario) y para cada método verifica:

- **C.1**: Método definido más de una vez en la misma clase.
- **C.3**: Nombre de parámetro formal no puede ser `self`.
- **C.2**: Parámetros formales con nombre repetido.
- **C.4**: Tipo de parámetro no puede ser `SELF_TYPE`.
- **C.5**: Tipo de parámetro o retorno debe ser una clase definida.

Para cada atributo verifica:

- **B.1**: Nombre del atributo no puede ser `self`.
- **B.2**: Atributo definido más de una vez en la misma clase.
- **B.4**: Tipo del atributo debe ser una clase definida o `SELF_TYPE`.

---

### 5.7 `check_overrides`

Recorre los métodos y atributos propios de cada clase de usuario y los compara con sus
ancestros usando `get_ancestors(c_name)`:

#### Atributos (regla B.3)

Si un atributo `a` de la clase `C` aparece también en algún ancestro de `C`, se reporta
error: en COOL los atributos **no pueden** sobreescribirse.

#### Métodos (regla C.7)

Si un método `m` existe en un ancestro, se verifica invarianza de firma:
- El número de parámetros formales debe ser idéntico.
- El tipo de cada parámetro formal debe ser idéntico (no covariante ni contravariante).
- El tipo de retorno debe ser idéntico.

> [!NOTE]
> COOL usa invarianza estricta en los métodos sobreescritos (a diferencia de Eiffel con
> covarianza o Java con tipos de retorno covariantes). Esto simplifica la verificación de
> tipo pero es más restrictivo.

---

### 5.8 `check_types` — Pasada 2

Para cada clase de usuario:

1. **Construye el ambiente O** (`object_env: SymbolTable<Symbol, Entry>`):
   - `self → SELF_TYPE` (siempre presente).
   - Atributos de todos los ancestros (de más lejano a más cercano, luego propios de `C`).

2. **Verifica inicializaciones de atributos** (regla B.5):
   - `init->type_check(...)` devuelve el tipo inferido de la inicialización.
   - Si no es `No_type` (es decir, si existe inicialización), verifica conformidad con el tipo declarado.

3. **Verifica cuerpos de métodos** (regla C.6):
   - Abre un nuevo scope con `enterscope()`.
   - Agrega los parámetros formales al ambiente.
   - Llama `body->type_check(...)`.
   - Si el retorno declarado es `SELF_TYPE`, el cuerpo debe inferirse exactamente como `SELF_TYPE`.
   - Si no, el tipo inferido debe conformar con el tipo de retorno declarado.

---

### 5.9 `conforms(T1, T2, cur_class)` — Conformidad

Implementa la relación de subtipado **T1 ≤ T2** de COOL:

```
conforms(No_type, T2, C)      = true       ← error recovery
conforms(T1, T1, C)           = true       ← reflexividad
conforms(T1, SELF_TYPE, C)    = false      ← salvo T1 == SELF_TYPE
conforms(SELF_TYPE, T2, C)    = conforms(C, T2, C)
conforms(T1, T2, C)           = T2 ∈ ancestors(T1)  ← caso general
```

La función recorre la cadena de herencia de T1 hacia arriba. Si encuentra T2, retorna
`true`. Si llega a `Object` sin encontrarlo, retorna `false`.

---

### 5.10 `lub(T1, T2, cur_class)` — Least Upper Bound (Join)

Calcula el mínimo ancestro común de T1 y T2:

```
lub(T, T, C)              = T
lub(SELF_TYPE, SELF_TYPE, C) = SELF_TYPE
lub(SELF_TYPE, T2, C)     = lub(C, T2, C)
lub(T1, SELF_TYPE, C)     = lub(T1, C, C)
lub(T1, T2, C) → primer ancestro común en ancestors(T1) ∩ ancestors(T2)
```

El algoritmo obtiene `get_ancestors(T1)` y `get_ancestors(T2)` (listas desde el nodo
hacia la raíz `Object`) y busca el primer elemento que aparezca en ambas listas.

**Ejemplo con la jerarquía `Dog → Mammal → Animal → IO → Object`:**

```
lub(Dog, Cat)
  ancestors(Dog) = [Dog, Mammal, Animal, IO, Object]
  ancestors(Cat) = [Cat, Mammal, Animal, IO, Object]
  Primer común   = Mammal  ✓
```

Esto coincide con el resultado esperado en `good.cl`:

```cool
let chosen : Mammal <- if cond then dog else cat fi
```

---

### 5.11 `type_check` — Reglas de Tipado por Expresión

#### `assign_class` — Regla [Assign]

```
O,M,C ⊢ x ← e : T'
  donde O(x) = T, O,M,C ⊢ e : T', T' ≤ T
```

Verificaciones: `x` no puede ser `self` (D.2), `x` debe estar declarado en O (D.1),
el tipo de `e` debe conformar con el tipo declarado de `x` (E.1).

#### `dispatch_class` — Regla [Dispatch]

```
O,M,C ⊢ e₀.f(e₁,...,eₙ) : T
  C' = tipo_de_despacho(T₀)  donde T₀ = tipo(e₀)
  M(C',f) = (T₁,...,Tₙ,Tᵣ)
  T = Tᵣ si Tᵣ ≠ SELF_TYPE, sino T = T₀
```

**Paso clave de SELF_TYPE en dispatch:** Si el tipo de retorno declarado en la firma
del método es `SELF_TYPE`, el tipo de retorno del dispatch es el tipo *estático* del
receptor `e₀` (no `SELF_TYPE` en sí), porque en tiempo de ejecución `SELF_TYPE` se
refiere a la clase del objeto, que estáticamente conocemos como el tipo de `e₀`.

```cpp
Symbol result_type = m->return_type;
if (result_type == SELF_TYPE) {
    result_type = t0;   // tipo estático del receptor
}
```

#### `static_dispatch_class` — Regla [StaticDispatch]

Igual que `dispatch`, pero la búsqueda del método se hace en la clase `T` especificada
explícitamente (`e@T.m(...)`), no en la clase dinámica de `e`. Restricciones adicionales:

- `T` no puede ser `SELF_TYPE` (C.4-estático).
- `T` debe ser una clase definida.
- El tipo de `e` debe conformar con `T` (E.4).

#### `cond_class` — Regla [If]

```
O,M,C ⊢ if p then t else e fi : lub(T1, T2)
  donde tipo(p) = Bool, tipo(t) = T1, tipo(e) = T2
```

La condición debe ser exactamente `Bool`. El tipo del `if` es el LUB de los tipos de
ambas ramas, lo que captura polimorfismo de forma segura.

#### `loop_class` — Regla [Loop]

```
O,M,C ⊢ while p loop b pool : Object
  donde tipo(p) = Bool
```

El tipo de un `while` siempre es `Object`, independientemente del tipo del cuerpo,
porque el bucle puede no ejecutarse nunca.

#### `typcase_class` — Regla [Case]

```
O,M,C ⊢ case e of x₁:T₁ ⟹ e₁ ... xₙ:Tₙ ⟹ eₙ esac : lub(S₁,...,Sₙ)
  donde Sᵢ = tipo(eᵢ) con O[xᵢ:Tᵢ],M,C
```

Restricciones:
- Ningún `xᵢ` puede ser `self` (D.3).
- Ningún `Tᵢ` puede ser `SELF_TYPE` (E.9).
- Los `Tᵢ` deben ser tipos definidos (E.9).
- Los `Tᵢ` deben ser todos distintos entre sí (E.9).
- El tipo del `case` es el LUB de los tipos de todas las ramas.

#### `block_class` — Regla [Block]

```
O,M,C ⊢ { e₁; e₂; ...; eₙ } : Tₙ
```

El tipo del bloque es el tipo de la **última** expresión del bloque.

#### `let_class` — Regla [Let]

```
O,M,C ⊢ let x:T ← e₀ in e₁ : T₁
  donde O[x:T],M,C ⊢ e₁ : T₁
        tipo(e₀) ≤ T (si e₀ existe)
```

Restricciones: `x` no puede ser `self` (D.3), `T` debe ser una clase definida o
`SELF_TYPE` (D.4). La inicialización `e₀` se evalúa en el ambiente **sin** `x`
(no es visible en su propia inicialización). El cuerpo `e₁` se evalúa con `x:T` en O.

#### Operaciones aritméticas (`plus`, `sub`, `mul`, `divide`) — Regla [Arith]

```
O,M,C ⊢ e₁ ⊕ e₂ : Int
  donde tipo(e₁) = Int, tipo(e₂) = Int
```

Ambos operandos deben ser exactamente `Int`. El resultado es siempre `Int`.

#### Negación (`neg_class`) — Regla [Neg]

```
O,M,C ⊢ ~e : Int   donde tipo(e) = Int
```

#### Comparaciones (`lt_class`, `leq_class`) — Reglas [Lt, Leq]

```
O,M,C ⊢ e₁ < e₂ : Bool   donde tipo(e₁) = Int, tipo(e₂) = Int
```

#### Igualdad (`eq_class`) — Regla [Equal]

```
O,M,C ⊢ e₁ = e₂ : Bool
```

Regla especial: si alguno de los dos operandos es de tipo primitivo (`Int`, `Bool`,
`String`), ambos deben ser del **mismo** tipo primitivo. Tipos no primitivos pueden
compararse con `=` sin restricción (comparación de identidad de objeto).

```
Permitido:   5 = 5           (Int = Int)
Permitido:   obj1 = obj2     (Object = Object → identidad)
Error:       5 = "cinco"     (Int = String)
Error:       true = 1        (Bool = Int)
```

#### Complemento lógico (`comp_class`) — Regla [Comp]

```
O,M,C ⊢ not e : Bool   donde tipo(e) = Bool
```

#### Literales

```
O,M,C ⊢ n : Int       (entero)
O,M,C ⊢ s : String    (cadena)
O,M,C ⊢ true : Bool
O,M,C ⊢ false : Bool
```

#### `new__class` — Regla [New]

```
O,M,C ⊢ new T : T     (T ≠ SELF_TYPE)
O,M,C ⊢ new SELF_TYPE : SELF_TYPE
```

Si T no existe, se reporta error y se recupera con tipo `Object`.

#### `isvoid_class` — Regla [IsVoid]

```
O,M,C ⊢ isvoid e : Bool
```

El tipo del operando no importa; `isvoid` siempre retorna `Bool`.

#### `no_expr_class` — Regla [No_expr]

```
O,M,C ⊢ <no_expr> : No_type
```

`No_type` es un centinela que significa "sin tipo" (no confundirlo con `Object`).
`conforms(No_type, T, C)` retorna `true` para cualquier `T`, lo que permite
atributos sin inicialización.

#### `object_class` — Regla [Object] (identificador)

```
O,M,C ⊢ self : SELF_TYPE
O,M,C ⊢ x : O(x)          (si x ≠ self y x ∈ dom(O))
```

Si `x` no está en O, se reporta error (D.1) y se recupera con tipo `Object`.

---

## 6. Archivos de Prueba

### `good.cl`

Programa semánticamente correcto que cubre exhaustivamente las construcciones del
lenguaje COOL. Fue diseñado para activar todas las reglas de tipado sin producir errores.

**Jerarquía de clases:**
```
Object
  └── IO
        └── Animal    (name: String, age: Int)
              └── Mammal  (has_fur: Bool)
                    ├── Dog   (breed: String, best_friend: SELF_TYPE)
                    └── Cat   (lives_left: Int)
        └── Main
```

**Construcciones cubiertas:**

| Construcción | Dónde en `good.cl` |
|---|---|
| `SELF_TYPE` como retorno | `init() : SELF_TYPE` en Animal |
| `SELF_TYPE` como atributo | `best_friend : SELF_TYPE` en Dog |
| `SELF_TYPE` en let | `let copy_of_main : SELF_TYPE <- new SELF_TYPE` |
| `new SELF_TYPE` | `clone_self() : SELF_TYPE { new SELF_TYPE }` |
| Override válido | `speak()` en Animal, Mammal y Dog |
| Despacho dinámico | `dog.speak()`, `chosen_animal.get_name()` |
| Despacho estático | `dog@Mammal.speak()` |
| `if` con LUB | `if cond then dog else cat fi : Mammal` |
| `while` | Resultado siempre `Object` |
| `case` con ramas disyuntas | 5 ramas: Dog, Cat, Mammal, Animal, Object |
| `let` con múltiples bindings | `let count: Int, greeting: String, ...` |
| `isvoid` | `isvoid dog` |
| Métodos de Object | `.copy()`, `.type_name()` |

### `bad.cl`

Programa con **errores semánticos intencionales** que el analizador debe detectar.
Cubre todas las categorías del enunciado:

| Categoría | Errores incluidos |
|---|---|
| A — Clases | A.1 (dup), A.2 (básica Int), A.3 (SELF_TYPE), A.4 (indef), A.5 (sellada), A.6 (ciclo), A.8 (main con args) |
| B — Atributos | B.1 (self), B.2 (dup), B.3 (herencia), B.4 (tipo indef), B.5 (init) |
| C — Métodos | C.1 (dup), C.2 (params dup), C.3 (self param), C.4 (SELF_TYPE param), C.5 (tipo indef), C.6 (cuerpo), C.7 (override) |
| D — Identificadores | D.1 (indef), D.2 (asign self), D.3 (self en let/case), D.4 (tipo let indef) |
| E — Expresiones | E.1 (asign), E.3 (dispatch indef), E.4 (new indef), E.5 (if/while cond), E.6 (arith), E.7 (neg/not), E.8 (igualdad), E.9 (case dup/SELF_TYPE/indef) |

---

## 7. Reglas del Sistema de Tipos de COOL (Notación Formal)

La notación `O, M, C ⊢ e : T` se lee:  
*"Bajo el ambiente de objetos O, el ambiente de métodos M, y en la clase actual C,
la expresión e tiene tipo T."*

### Tabla de Tipado (resumen)

| Expresión COOL | Regla de Tipado | Restricciones |
|---|---|---|
| `n` (entero) | `Int` | — |
| `"s"` (cadena) | `String` | — |
| `true` / `false` | `Bool` | — |
| `self` | `SELF_TYPE` | — |
| `x` | `O(x)` | x ∈ dom(O) |
| `x ← e` | `T'` | T' = tipo(e), T' ≤ O(x), x ≠ self |
| `e.m(aᵢ)` | `Tᵣ` o tipo(e) si Tᵣ=SELF_TYPE | m existe en tipo(e) |
| `e@T.m(aᵢ)` | Tᵣ | tipo(e) ≤ T, m existe en T |
| `if p then t else e fi` | `lub(tipo(t), tipo(e))` | tipo(p) = Bool |
| `while p loop b pool` | `Object` | tipo(p) = Bool |
| `{ e₁;…;eₙ }` | `tipo(eₙ)` | — |
| `let x:T ← e in b` | `tipo(b)` con O[x:T] | tipo(e) ≤ T, x ≠ self |
| `case e of xᵢ:Tᵢ ⟹ eᵢ esac` | `lub(tipo(eᵢ))` | Tᵢ distintos, Tᵢ definidos, Tᵢ ≠ SELF_TYPE |
| `new T` | `T` | T definida o SELF_TYPE |
| `isvoid e` | `Bool` | — |
| `e₁ ⊕ e₂` (+,-,*,/) | `Int` | ambos Int |
| `~e` | `Int` | tipo(e) = Int |
| `e₁ < e₂`, `e₁ ≤ e₂` | `Bool` | ambos Int |
| `e₁ = e₂` | `Bool` | si alguno es básico, deben ser iguales |
| `not e` | `Bool` | tipo(e) = Bool |

### Reglas de SELF_TYPE

| Posición | ¿SELF_TYPE permitido? | Efecto semántico |
|---|---|---|
| Tipo de retorno de método | ✅ Sí | Polimorfismo de retorno |
| Tipo de atributo | ✅ Sí | Atributo del mismo tipo que `self` |
| Tipo de variable en `let` | ✅ Sí | Variable del mismo tipo que `self` |
| Expresión `new SELF_TYPE` | ✅ Sí | Crea objeto del mismo tipo dinámico |
| Nombre de clase | ❌ No | Error A.3 |
| Tipo de parámetro formal | ❌ No | Error C.4 |
| Tipo en despacho estático `@T` | ❌ No | Error E.4 |
| Tipo de rama en `case` | ❌ No | Error E.9 |

---

## 8. Banco de Preguntas y Respuestas para la Sustentación

> **Formato de penalización:** −1/5 por cada pregunta incorrecta; 0/5 si ≥ 3 incorrectas.

---

### Arquitectura General

**P1. ¿Cuántas pasadas realiza el analizador semántico implementado y qué hace cada una?**

> **R:** El analizador realiza **dos pasadas principales** sobre las clases:
> - **Pasada 0 (pre-análisis):** `install_basic_classes` + `install_classes` + `check_inheritance` + `check_main`. Establece el grafo de herencia válido.
> - **Pasada 1:** `build_feature_tables` + `check_overrides`. Recolecta todas las firmas de métodos y atributos para cada clase (incluyendo heredados), y verifica reglas de sobreescritura.
> - **Pasada 2:** `check_types`. Recorre el AST de cada expresión en métodos y atributos, invocando `type_check` recursivamente para anotar cada nodo con su tipo estático.
>
> Son necesarias al menos dos pasadas porque los métodos de una clase pueden referirse a métodos de otras clases que aún no se han analizado en detalle (uso mutuo).

---

**P2. ¿Por qué se necesita `initialize_constants()` y qué hace exactamente?**

> **R:** En CS143, los identificadores de tipos (como `"Bool"`, `"Int"`, `"String"`) son punteros `Symbol` a registros de la tabla de cadenas (`idtable`). La función `initialize_constants` llama a `idtable.add_string(...)` una vez para cada identificador y guarda el puntero resultante en variables globales (`Bool`, `Int`, `Str`, `SELF_TYPE`, `self`, etc.). Esto es necesario porque la comparación de símbolos se hace con `==` (comparación de punteros), y para que dos usos de `"Bool"` en partes distintas del código comparen como iguales, deben apuntar al mismo registro de la tabla de cadenas.

---

**P3. ¿Qué es `SymbolTable<Symbol, Entry>` y cómo modela el ambiente *O* de la semántica?**

> **R:** `SymbolTable<Symbol, Entry>` es una tabla de símbolos con pila de scopes implementada en CS143. `Symbol` es el tipo de la clave (nombre del identificador) y `Entry` es el tipo del valor almacenado (aquí, el tipo declarado). Cada `enterscope()` crea un nuevo scope (capa), y `exitscope()` lo descarta. `addid(x, T)` agrega `x` con valor `T` en el scope actual. `lookup(x)` busca desde el scope más interno hacia el más externo (implementando *static scoping*). En COOL, el ambiente *O* asigna a cada variable visible su tipo declarado; la tabla de símbolos implementa directamente esto con la pila de scopes.

---

**P4. ¿Qué ocurre si el analizador detecta un error de tipo en una expresión? ¿Cómo evita una cascada de errores?**

> **R:** Se aplica *error recovery*: cuando `type_check` detecta un error, reporta el mensaje con `semant_error(...)`, incrementa el contador de errores, pero **continúa el análisis** asignando el tipo `Object` a la expresión problemática. Esto es seguro porque `Object` es el supertipo de todos los tipos en COOL, por lo que `conforms(Object, T, C)` puede fallar pero `conforms(T, Object, C)` siempre es verdadero. Así se evita que un único error propague falsos errores a todas las expresiones que dependen de él.

---

### Grafo de Herencia y Clases

**P5. ¿Qué clases de COOL no pueden heredarse y por qué?**

> **R:** Las clases `Int`, `Bool` y `String` son **selladas** (*sealed*) en COOL. No pueden heredarse porque tienen representaciones internas especiales en el runtime (enteros máquina, booleanos máquina, cadenas con longitud). Permitir subclasificación rompería la semántica de los operadores primitivos. `SELF_TYPE` tampoco puede usarse como clase padre porque no es un tipo de clase real sino un metanombre.

---

**P6. ¿Cómo detecta el analizador ciclos en el grafo de herencia?**

> **R:** Para cada clase `C`, se inicia un conjunto `visited = {C}` y se recorre la cadena de herencia (`C → padre(C) → padre(padre(C)) → ...`) hasta llegar a `Object` o a un padre desconocido. En cada paso se verifica si el padre actual ya está en `visited`. Si lo está, hay un ciclo y se reporta el error. Este algoritmo es O(n) por clase en el peor caso, pero correcto porque el grafo de herencia es un DAG (dirigido acíclico) si no hay ciclos.

---

**P7. ¿En qué orden se instalan los atributos en el ambiente O durante `check_types`?**

> **R:** Se obtiene la lista de ancestros de la clase actual (desde la clase hasta `Object`). Luego se recorre **de más lejano a más cercano** (es decir, de `Object` hacia la clase actual). Esto garantiza que los atributos del padre se instalen antes que los del hijo, de modo que si un hijo intenta redefinir un atributo (lo cual es un error), el atributo del padre ya está en la tabla y se detectará la redefinición en `check_overrides`.

---

### Sistema de Tipos

**P8. Explique la diferencia entre `conforms` y `lub`. ¿Cuándo se usa cada uno?**

> **R:**
> - `conforms(T1, T2, C)`: pregunta *"¿puede usarse un T1 donde se espera un T2?"* Implementa T1 ≤ T2 en el orden parcial del árbol de herencia. Se usa al verificar que el tipo de una expresión cumple el tipo esperado (en asignaciones, despachos, retornos de método, inicializaciones de let/atributo).
> - `lub(T1, T2, C)`: calcula el **mínimo ancestro común** de T1 y T2 en el árbol. Se usa cuando el contexto puede producir dos tipos distintos y necesitamos el tipo más específico que cubre ambos: en `if` (branches), en `case` (branches) y en cadenas de `lub` para múltiples ramas.

---

**P9. ¿Qué es el tipo `No_type` y para qué sirve?**

> **R:** `No_type` (símbolo `"_no_type"`) es un centinela que representa la ausencia de tipo en `no_expr_class` (el nodo del AST para "sin expresión de inicialización"). `conforms(No_type, T, C)` siempre retorna `true`, lo que permite que un atributo sin inicialización (`x : Int` sin `<-`) sea válido: su inicialización es `no_expr()` con tipo `No_type`, y `No_type ≤ Int` es verdadero por convenio. Esto es distinto de `Object`: `Object` es el supertipo de todos los tipos reales, mientras que `No_type` es un tipo ficticio sólo para el caso de ausencia de inicialización.

---

**P10. ¿Qué significa que `SELF_TYPE` aparezca como tipo de retorno de un método? ¿Cómo se resuelve en un despacho?**

> **R:** Cuando un método declara `SELF_TYPE` como tipo de retorno (ej: `init() : SELF_TYPE`), significa que el método devuelve un objeto del **mismo tipo dinámico** que `self`. Estáticamente, en un despacho `e.init()` donde `tipo(e) = Dog`, el tipo del dispatch es `Dog` (el tipo estático de `e`). Esto es el *principio de especialización*: una subclase que hereda `init()` y lo llama obtendrá un resultado del tipo de la subclase. En el código: si `m->return_type == SELF_TYPE`, el resultado del dispatch se fija como `t0` (el tipo estático del receptor `e₀`), no como `SELF_TYPE`.

---

**P11. ¿Por qué SELF_TYPE no puede ser el tipo de un parámetro formal?**

> **R:** Un parámetro formal `x : SELF_TYPE` crearía una ambigüedad: desde la perspectiva del llamador, ¿qué tipo concreto se pasa? Si el método `m(x: SELF_TYPE)` está en `Animal` y se llama sobre un `Dog`, `SELF_TYPE` significaría `Dog`, por lo que el argumento debería ser un `Dog`. Pero si el mismo método se llama sobre un `Cat`, el argumento debería ser un `Cat`. Esto hace imposible una verificación estática unificada de llamadas al método, especialmente en presencia de polimorfismo. Para evitar esta complejidad, el Manual de COOL prohíbe `SELF_TYPE` en parámetros formales.

---

**P12. ¿Por qué SELF_TYPE no puede usarse como tipo de rama en un `case`?**

> **R:** En un `case e of x:T ⟹ expr esac`, el tipo `T` de cada rama se usa para el **despacho dinámico de selección**: en tiempo de ejecución se elige la rama cuyo tipo `T` es el más específico ancestro del tipo dinámico de `e`. Si `T = SELF_TYPE`, el sistema de tipos no puede determinar estáticamente qué clase concreta representa `SELF_TYPE` en ese punto (podría ser cualquier subclase), haciendo imposible la selección de rama. Por eso el Manual de COOL lo prohíbe explícitamente.

---

### Reglas de Tipado Específicas

**P13. ¿Cuál es el tipo de un `while ... loop ... pool`? ¿Por qué?**

> **R:** El tipo de `while p loop b pool` es **siempre `Object`**, independientemente del tipo del cuerpo `b`. Esto se debe a que el bucle puede no ejecutarse ninguna vez (si la condición es falsa desde el principio), en cuyo caso "no hay valor de retorno". En COOL todo debe tener un tipo, y el supertipo de cualquier posible tipo de `b` es `Object`. Formalmente: el resultado del `while` es `Object`.

---

**P14. ¿Cuál es el tipo de un bloque `{ e1; e2; e3 }`? ¿Importa el tipo de las expresiones anteriores?**

> **R:** El tipo del bloque es el **tipo de la última expresión** (aquí `tipo(e3)`). Las expresiones anteriores se evalúan por sus efectos secundarios (asignaciones, despachos con efectos). Sus tipos se verifican igualmente (para asegurar que son expresiones válidas), pero no afectan el tipo del bloque.

---

**P15. En la regla de `let`, ¿cuándo se agrega la variable al ambiente y cuándo no?**

> **R:** La inicialización `e₀` se evalúa en el ambiente **sin** la nueva variable `x` (scope externo). Solo el cuerpo `e₁` se evalúa con `x` en el ambiente. Esto evita la auto-referencia en la inicialización (`let x:Int <- x + 1 in ...` sería un error porque `x` no existe aún al evaluar `x + 1`). En el código:
> ```cpp
> Symbol init_t = init->type_check(ct, cur_class, o_table);  // sin x
> o_table.enterscope();
> o_table.addid(identifier, type_decl);                       // ahora x existe
> Symbol body_t = body->type_check(ct, cur_class, o_table);  // con x
> o_table.exitscope();
> ```

---

**P16. ¿Cómo funciona la regla de igualdad (`=`) para tipos básicos?**

> **R:** En COOL, `=` sobre tipos de referencia compara identidad de objeto (si son el mismo objeto en heap). Sin embargo, para los tipos primitivos (`Int`, `Bool`, `String`) la igualdad tiene semántica de valor. Para evitar comparaciones ambiguas, el Manual de COOL establece que si **alguno** de los dos operandos es de tipo básico, entonces **ambos** deben ser del mismo tipo básico. Si no, es un error estático. Tipos no básicos pueden compararse libremente entre sí con `=`.

---

**P17. ¿Qué es `lub(Dog, Cat)` en la jerarquía `Dog → Mammal → Animal → IO → Object` y `Cat → Mammal`?**

> **R:**
> ```
> ancestors(Dog) = [Dog, Mammal, Animal, IO, Object]
> ancestors(Cat) = [Cat, Mammal, Animal, IO, Object]
> ```
> El primer elemento en común es `Mammal`. Por tanto `lub(Dog, Cat) = Mammal`.  
> Esto permite que `if cond then new Dog else new Cat fi` tenga tipo `Mammal`.

---

**P18. ¿Qué hace `get_ancestors(X)` y cuál es el orden de la lista resultante?**

> **R:** Retorna la lista de ancestros de `X` **desde la clase misma hasta `Object`** (inclusive). Ejemplo: `get_ancestors(Dog) = [Dog, Mammal, Animal, IO, Object]`. Este orden es importante para `lub`: al buscar el primer elemento común en las dos listas de ancestros, se encuentra **el ancestro común más específico**, que es exactamente el LUB.

---

### Overrides y Herencia de Atributos

**P19. ¿Qué ocurre si una subclase redefine un atributo heredado?**

> **R:** Es un **error semántico** (regla B.3). En COOL, los atributos se heredan pero **no se pueden sobreescribir**. Si la subclase `S` hereda el atributo `x` de `P` y declara su propio `x`, se reporta: `"Attribute x is an attribute of an inherited class."` La razón es que permitir redefinición de atributos rompería el modelo de memoria del runtime (el layout del objeto).

---

**P20. ¿Qué invariantes deben cumplirse al sobreescribir un método?**

> **R:** En COOL, el override de un método en una subclase debe preservar **invarianza estricta** de la firma:
> 1. El número de parámetros formales debe ser idéntico.
> 2. El tipo de cada parámetro formal (en la misma posición) debe ser idéntico.
> 3. El tipo de retorno debe ser idéntico.
>
> Esto es más restrictivo que Java (que permite tipos de retorno covariantes) y que Eiffel (que permite tipos de parámetros covariantes). La invarianza garantiza que cualquier llamada válida sobre un objeto de tipo padre sigue siendo válida sobre un objeto de tipo hijo.

---

**P21. ¿Por qué `check_overrides` busca sólo en el ancestro más cercano que define el método?**

> **R:** Porque la verificación de overrides es **transitiva**: si `C` sobreescribe correctamente el método `m` de `P`, y `P` sobreescribió correctamente `m` de `P'`, entonces la firma de `m` en `C` necesariamente coincide con la de `P'` (porque coincide con la de `P` que coincide con la de `P'`). Por tanto, basta verificar contra el primer ancestro que define el método. Verificar contra todos los ancestros produciría mensajes de error duplicados.

---

### COOL y el Compilador

**P22. ¿Cuál es el orden en que el compilador de COOL procesa un archivo `.cl`?**

> **R:**
> 1. **Análisis léxico** (`lexer`): convierte el texto en tokens.
> 2. **Análisis sintáctico** (`parser`): construye el AST.
> 3. **Análisis semántico** (`semant`): verifica las reglas estáticas y anota el AST con tipos. ← *Este laboratorio.*
> 4. **Generación de código intermedio** (opcional, dependiendo del compilador).
> 5. **Generación de código** (`cgen`): produce código MIPS o SPIM para el runtime de COOL.

---

**P23. ¿Por qué se necesitan al menos 2 pasadas para el análisis semántico de COOL?**

> **R:** Porque COOL permite **uso mutuo de clases**: la clase `A` puede usar un método de la clase `B` antes de que `B` haya sido completamente analizada. Con una sola pasada, si `A` aparece antes de `B` en el archivo, al analizar `A` aún no se conocería la firma de los métodos de `B`. Con dos pasadas: en la primera se recolectan todas las firmas (Pasada 1), y en la segunda se verifican los cuerpos de los métodos usando las firmas ya conocidas de todas las clases (Pasada 2).

---

**P24. ¿Qué es `_no_class` y cuándo aparece en el grafo de herencia?**

> **R:** `_no_class` es el "padre ficticio" de la clase raíz `Object`. El árbol de herencia de COOL tiene una raíz única (`Object`) cuyo padre es `_no_class` (un símbolo centinela, nunca una clase real). Las funciones `get_ancestors` y `conforms` recorren la cadena de herencia y se detienen cuando encuentran `_no_class` (o cuando llegan a un nodo desconocido en `name_to_class`). `_no_class` nunca aparece en código COOL del usuario.

---

**P25. Si en `bad.cl` la clase `Main` define `main(arg: Int): Object`, ¿cuántos errores produce y cuáles?**

> **R:** Exactamente **un error** de la categoría A.8: `"'main' method in class Main should have no arguments."` La clase `Main` existe y tiene un método llamado `main`, pero ese método tiene un parámetro formal, lo cual viola la regla. No se genera un error A.7 (que sería si `Main` no existiera o no tuviera ningún método `main`).

---

**P26. ¿Cómo se representa internamente `String` en COOL? ¿Por qué el símbolo C++ se llama `Str` y no `String`?**

> **R:** En COOL el tipo se escribe `String` en el código fuente. Sin embargo, en el runtime de CS143 el símbolo interno en la tabla de cadenas es la cadena `"String"`. En el código C++ del analizador semántico, la variable global que apunta a ese símbolo se llama `Str` (no `String`) para evitar conflictos con `std::string` de C++ o con el nombre `String` que podría usarse en otros contextos. La línea crítica es:
> ```cpp
> Str = idtable.add_string("String");
> ```
> Es decir: `Str` (C++) apunta al símbolo `"String"` (COOL). En comparaciones se usa el puntero `Str`, no la cadena literal.

---

**P27. ¿Qué ocurre con `isvoid` en COOL? ¿Puede `isvoid` sobre un Int dar `true`?**

> **R:** `isvoid` sobre un `Int`, `Bool` o `String` **nunca da `true`** en tiempo de ejecución, porque los valores primitivos no son punteros y no pueden ser `void`. Sin embargo, el analizador semántico **no reporta error** al usar `isvoid` sobre un Int; simplemente calcula el tipo de la expresión como `Bool` sin importar el tipo del operando. Es una advertencia semántica que el estándar de COOL decide no reportar como error.

---

**P28. ¿Qué ocurre si no existe la clase `Main` en el programa?**

> **R:** El analizador reporta el error A.7: `"Class Main is not defined."` y **continúa** el análisis (si es posible) para detectar otros errores. Al final, si hay errores, el compilador imprime `"Compilation halted due to static semantic errors."` y termina con código de salida distinto de 0. No se genera código objeto.

---

*Fin del documento de explicación detallada.*

---

> [!TIP]
> Para la sustentación, practica explicar en voz alta la función `lub` con un ejemplo de jerarquía propia, y traza manualmente la ejecución de `conforms(SELF_TYPE, Animal, Dog)`. Esas son las preguntas más frecuentes en sustentaciones de este laboratorio.
