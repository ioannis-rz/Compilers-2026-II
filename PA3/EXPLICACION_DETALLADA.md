# Explicación Técnica Detallada — Analizador Sintáctico de COOL (`cool.y`)

Este documento complementa el archivo [README.md](README.md) y detalla cada elemento del código fuente de [`cool.y`](cool.y).

---

## 1. Declaraciones Globales y Enlace con el Runtime de Stanford

En la cabecera `%{%}` se incluyen:
- `cool-tree.h`: Define los tipos de punteros (`Program`, `Class_`, `Feature`, `Expression`, etc.) y las funciones constructoras del AST.
- `stringtab.h`: Provee las tablas de símbolos (`idtable`, `stringtable`, `inttable`) para realizar *interning* de identificadores y literales.
- `utilities.h`: Utilidades para impresión de tokens y manejo de mensajes.
- `ast_root`: Variable global asignada al reducir la producción raíz `program`. El punto de entrada `main()` en `parser-phase.cc` lee este puntero para llamar a `ast_root->dump_with_types(cout, 0)`.
- `parse_results`: Variable global de tipo `Classes` requerida por `parser-phase.cc` para acumular las clases del archivo.
- `omerrs`: Contador de errores léxicos y sintácticos.

---

## 2. Definición de la Unión Semántica (`%union`)

Bison asigna a cada símbolo un valor semántico a través de `YYSTYPE`, definido como una `union`:

```yacc
%union {
  Boolean         boolean;
  Symbol          symbol;
  Program         program;
  Class_          class_;
  Classes         classes;
  Feature         feature;
  Features        features;
  Formal          formal;
  Formals         formals;
  Case            case_;
  Cases           cases;
  Expression      expression;
  Expressions     expressions;
  char            *error_msg;
}
```

Cada no-terminal se asocia a su campo correspondiente:
- `%type <program> program`
- `%type <classes> class_list`
- `%type <class_> class`
- `%type <features> feature_list`
- `%type <feature> feature`
- `%type <formals> formal_list formal_list_ne`
- `%type <formal> formal`
- `%type <expression> expr let_expr`
- `%type <expressions> expr_list actual_list actual_list_ne`
- `%type <cases> case_list`
- `%type <case_> case_branch`

---

## 3. Códigos Numéricos de Tokens Terminales

Los tokens deben corresponder exactamente a los valores definidos en `tokens-lex.o`:
- Palabras clave: `CLASS (258)`, `ELSE (259)`, `FI (260)`, `IF (261)`, `IN (262)`, `INHERITS (263)`, `LET (264)`, `LOOP (265)`, `POOL (266)`, `THEN (267)`, `WHILE (268)`, `CASE (269)`, `ESAC (270)`, `OF (271)`, `DARROW (272)`, `NEW (273)`, `ISVOID (274)`.
- Valores con atributos:
  - `STR_CONST (275)` $\rightarrow$ `<symbol>`
  - `INT_CONST (276)` $\rightarrow$ `<symbol>`
  - `BOOL_CONST (277)` $\rightarrow$ `<boolean>`
  - `TYPEID (278)` $\rightarrow$ `<symbol>`
  - `OBJECTID (279)` $\rightarrow$ `<symbol>`
- Operadores y tokens especiales:
  - `ASSIGN (280)`
  - `NOT (281)`
  - `LE (282)`
  - `ERROR (283)`
  - `LET_STMT (285)`

---

## 4. Reglas Gramaticales y Constructores de Nodos

| Producción | Acción Semántica | Constructor AST CS143 |
| :--- | :--- | :--- |
| `program : class_list` | `ast_root = program($1);` | `Program program(Classes)` |
| `class : CLASS TYPEID '{' feature_list '}'` | Herencia de `Object` por defecto | `Class_ class_(Symbol, Symbol, Features, Symbol)` |
| `class : CLASS TYPEID INHERITS TYPEID '{' feature_list '}'` | Herencia explícita | `Class_ class_(Symbol, Symbol, Features, Symbol)` |
| `feature : OBJECTID '(' formal_list ')' ':' TYPEID '{' expr '}'` | Definición de método | `Feature method(Symbol, Formals, Symbol, Expression)` |
| `feature : OBJECTID ':' TYPEID ASSIGN expr` | Atributo con valor inicial | `Feature attr(Symbol, Symbol, Expression)` |
| `feature : OBJECTID ':' TYPEID` | Atributo sin valor inicial | `Feature attr(Symbol, Symbol, Expression)` con `no_expr()` |
| `expr : OBJECTID ASSIGN expr` | Asignación de variable | `Expression assign(Symbol, Expression)` |
| `expr : expr '.' OBJECTID '(' actual_list ')'` | Despacho dinámico | `Expression dispatch(Expression, Symbol, Expressions)` |
| `expr : expr '@' TYPEID '.' OBJECTID '(' actual_list ')'` | Despacho estático | `Expression static_dispatch(Expression, Symbol, Symbol, Expressions)` |
| `expr : OBJECTID '(' actual_list ')'` | Despacho sobre `self` | `dispatch(object("self"), $1, $3)` |
| `expr : IF expr THEN expr ELSE expr FI` | Expresión condicional | `Expression cond(Expression, Expression, Expression)` |
| `expr : WHILE expr LOOP expr POOL` | Bucle while | `Expression loop(Expression, Expression)` |
| `expr : '{' expr_list '}'` | Bloque secuencial | `Expression block(Expressions)` |
| `expr : CASE expr OF case_list ESAC` | Evaluación de casos | `Expression typcase(Expression, Cases)` |
| `expr : NEW TYPEID` | Instanciación | `Expression new_(Symbol)` |
| `expr : ISVOID expr` | Comprobación de vacío | `Expression isvoid(Expression)` |

---

## 5. Función de Reporte de Errores (`yyerror`)

```cpp
void yyerror(char *s) {
  extern int curr_lineno;

  cerr << "\"" << curr_filename << "\", line " << curr_lineno << ": "
       << s << " at or near ";
  print_cool_token(yychar);
  cerr << endl;
  omerrs++;

  if (omerrs > 50) {
    fprintf(stderr, "More than 50 errors\n");
    exit(1);
  }
}
```
Bison invoca automáticamente `yyerror()` cuando encuentra un token inesperado. Imprime el nombre del archivo, número de línea, descripción del error y el token actual (`print_cool_token(yychar)`), e incrementa el contador de errores. Si se superan 50 errores, aborta para evitar bucles infinitos en código severamente dañado.