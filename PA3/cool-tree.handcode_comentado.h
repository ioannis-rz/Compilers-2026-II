#ifndef COOL_TREE_HANDCODE_H
#define COOL_TREE_HANDCODE_H

// ==============================================================================
// ARCHIVO: cool-tree.handcode_comentado.h
// PROYECTO: Laboratorio 03 - Analizador Semántico para COOL (PA4)
// ASIGNATURA: Compiladores, Universidad Nacional de Colombia (2026-II)
// ==============================================================================
// PROPÓSITO:
// Este archivo define macros de extensión manual que el generador de código de
// árboles de Stanford CS143 inyecta dentro de las clases de nodos del AST
// (declaradas en cool-tree.h y definidas en cool-tree.cc).
//
// ¿CÓMO FUNCIONA ESTE MECANISMO EN CS143?
// El sistema de AST de COOL se especifica en un archivo llamado "cool-tree.aps".
// La herramienta de compilación genera automáticamente las clases en C++, pero
// los estudiantes necesitan agregar campos (como el tipo inferido de cada expresión)
// y funciones virtuales (como type_check). En lugar de editar archivos generados,
// CS143 incluye cool-tree.handcode.h y expande estas macros dentro del cuerpo
// de cada clase concreta y abstracta del AST.
// ==============================================================================

#include <iostream>       // Para manipulación de flujos de salida (std::ostream).
#include "tree.h"         // Define las clases base del árbol (tree_node, list_node).
#include "cool.h"         // Tipos fundamentales de COOL (Boolean, etc.).
#include "stringtab.h"    // Tablas de símbolos de CS143 (Symbol, Entry).
#include "symtab.h"       // Tabla de símbolos con scopes: SymbolTable<Symbol, Entry>.

// Definición de utilidades de seguimiento de líneas:
#define yylineno_the_ yy_g_yylineno
extern int yylineno;      // Variable global del analizador sintáctico para números de línea.

// Funciones inline auxiliares para el manejo y volcado (dumping) de valores booleanos:
inline Boolean copy_Boolean(Boolean b) { return b; }
inline void assert_Boolean(Boolean) {}
inline void dump_Boolean(ostream& stream, int padding, Boolean b) {
    stream << pad(padding) << (b ? "true" : "false") << "\n";
}

// Prototipos de funciones globales de manipulación de símbolos para el AST:
void dump_Symbol(ostream& stream, int padding, Symbol b);
void assert_Symbol(Symbol b);
Symbol copy_Symbol(Symbol b);

// Declaración adelantada de la tabla de clases para que los punteros ClassTable* sean válidos:
class ClassTable;

// ==============================================================================
// EXTENSIONES PARA EL NODO RAÍZ: Program
// ==============================================================================
// Inyecta en la clase abstracta Program_class y en la clase concreta program_class
// el método semant() que sirve como punto de inicio de todo el análisis semántico.

#define Program_EXTRAS \
virtual void semant() = 0; \
virtual void dump_with_types(ostream&, int) = 0;

#define program_EXTRAS \
void semant(); \
void dump_with_types(ostream&, int);

// ==============================================================================
// EXTENSIONES PARA EL NODO: Class_ (Declaración de una clase)
// ==============================================================================
// Inyecta accesores para obtener el nombre de la clase, el nombre del padre,
// el archivo origen y la lista de características (features: métodos y atributos).

#define Class__EXTRAS \
virtual Symbol get_name() = 0; \
virtual Symbol get_parent() = 0; \
virtual Symbol get_filename() = 0; \
virtual Features get_features() = 0; \
virtual void dump_with_types(ostream&, int) = 0;

#define class__EXTRAS \
Symbol get_name() { return name; } \
Symbol get_parent() { return parent; } \
Symbol get_filename() { return filename; } \
Features get_features() { return features; } \
void dump_with_types(ostream&, int);

// ==============================================================================
// EXTENSIONES PARA EL NODO: Feature (Métodos o Atributos)
// ==============================================================================
// Métodos virtuales para consultar polimórficamente si un nodo Feature es un
// método (is_method) o un atributo (is_attr) sin requerir dynamic_cast inseguros.

#define Feature_EXTRAS \
virtual void dump_with_types(ostream&, int) = 0; \
virtual bool is_method() { return false; } \
virtual bool is_attr() { return false; }

#define Feature_SHARED_EXTRAS \
void dump_with_types(ostream&, int);

// ==============================================================================
// EXTENSIONES PARA EL NODO: Formal (Parámetro formal de un método)
// ==============================================================================
// Permite extraer el nombre y el tipo declarado de un parámetro (ej. "x : Int").

#define Formal_EXTRAS \
virtual void dump_with_types(ostream&, int) = 0; \
virtual Symbol get_name() = 0; \
virtual Symbol get_type_decl() = 0;

#define formal_EXTRAS \
void dump_with_types(ostream&, int); \
Symbol get_name() { return name; } \
Symbol get_type_decl() { return type_decl; }

// ==============================================================================
// EXTENSIONES PARA EL NODO: Case / Branch (Rama de una sentencia case)
// ==============================================================================
// Permite extraer la variable ligada, el tipo de la rama y la expresión del cuerpo
// (ej. "d : Dog => d.play_fetch()").

#define Case_EXTRAS \
virtual void dump_with_types(ostream&, int) = 0; \
virtual Symbol get_name() = 0; \
virtual Symbol get_type_decl() = 0; \
virtual Expression get_expr() = 0;

#define branch_EXTRAS \
void dump_with_types(ostream&, int); \
Symbol get_name() { return name; } \
Symbol get_type_decl() { return type_decl; } \
Expression get_expr() { return expr; }

// ==============================================================================
// EXTENSIONES PARA EL NODO: Expression (Expresiones de COOL)
// ==============================================================================
// Este es el componente MÁS IMPORTANTE inyectado en el AST:
// 1. Campo `type`: guarda el tipo inferido por el analizador semántico.
// 2. `get_type()` / `set_type()`: accesores estándar requeridos por el evaluador
//    de Stanford y por la fase de generación de código (cgen / PA5).
// 3. `type_check(...)`: método virtual puro en la base y concreto en cada una de
//    las 24 clases derivadas de Expression (assign, dispatch, cond, loop, etc.).
//
// Parámetros de type_check:
// - ct: puntero a la ClassTable para resolver herencia, conformidad (<=) y LUB.
// - cur_class: nodo AST de la clase que se está analizando (resuelve SELF_TYPE).
// - o_table: tabla de símbolos de objetos (el ambiente formal O).
// Retorna: el Symbol que representa el tipo estático inferido de la expresión.

#define Expression_EXTRAS \
Symbol type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table);

#define Expression_SHARED_EXTRAS \
void dump_with_types(ostream&, int); \
Symbol type; \
Symbol get_type() { return type; } \
Expression set_type(Symbol s) { type = s; return this; } \
virtual Symbol type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) = 0;

#endif // COOL_TREE_HANDCODE_H
