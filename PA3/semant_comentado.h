#ifndef SEMANT_H_
#define SEMANT_H_

// ==============================================================================
// ARCHIVO: semant_comentado.h
// PROYECTO: Laboratorio 03 - Analizador Semántico para COOL (PA4)
// ASIGNATURA: Compiladores, Universidad Nacional de Colombia (2026-II)
// ==============================================================================
// PROPÓSITO:
// Declarar la interfaz pública de la tabla de clases (ClassTable) y las estructuras
// auxiliares de metadatos (MethodInfo, AttrInfo) requeridas para la comprobación
// estática de tipos en COOL.
//
// Este archivo está exhaustivamente documentado línea por línea para servir como
// material de estudio para la sustentación oral ante el profesor y monitores.
// ==============================================================================

#include <assert.h>       // assert(): para afirmaciones de depuración e invariantes internas.
#include <iostream>       // std::ostream, std::cerr: para flujos de salida de errores semánticos.
#include <map>            // std::map: para tablas asociativas (símbolo -> objeto, jerarquías de herencia).
#include <set>            // std::set: para conjuntos de búsqueda rápida (detección de ciclos, nombres duplicados).
#include <vector>         // std::vector: para listas ordenadas dinámicas (parámetros formales, ancestros).
#include <string>         // std::string: para manipulación de cadenas estándar en C++.
#include "cool-tree.h"    // Árbol de Sintaxis Abstracta (AST) de COOL con todas las clases de nodos.
#include "stringtab.h"    // Tablas de símbolos de CS143 (idtable, stringtable, inttable).
#include "symtab.h"       // SymbolTable<Symbol, Entry>: tabla de símbolos jerárquica con scopes.
#include "list.h"         // Plantillas de listas encadenadas de CS143 para el AST.

#define TRUE 1            // Constante booleana entera compatible con macros de CS143.
#define FALSE 0           // Constante booleana entera compatible con macros de CS143.

class ClassTable;         // Declaración adelantada (forward declaration) de ClassTable.
typedef ClassTable *ClassTableP; // Alias de puntero usado tradicionalmente en el código de CS143.

// ==============================================================================
// ESTRUCTURA: MethodInfo
// ==============================================================================
// Almacena la firma (signature) completa de un método definido en una clase COOL.
// Se utiliza en la Pasada 1 para registrar métodos y en la Pasada 2 para validar
// las llamadas de despacho (dispatch) estático y dinámico.
// ==============================================================================
struct MethodInfo {
    Symbol name;                        // Nombre identificador del método (ej. "out_string", "init").
    std::vector<Symbol> formals_types;  // Lista ordenada con el tipo declarado de cada parámetro formal.
    std::vector<Symbol> formals_names;  // Lista ordenada con el nombre de cada parámetro formal.
    Symbol return_type;                 // Tipo de retorno declarado en el método (puede ser SELF_TYPE).
    Class_ defining_class;             // Puntero al nodo AST de la clase donde este método fue declarado.

    // Constructor por defecto: inicializa los punteros a NULL para evitar basura en memoria.
    MethodInfo() : name(NULL), return_type(NULL), defining_class(NULL) {}

    // Constructor parametrizado: inicializa todos los campos con los metadatos recolectados.
    MethodInfo(Symbol n, const std::vector<Symbol>& f_types, const std::vector<Symbol>& f_names, Symbol ret, Class_ def)
        : name(n), formals_types(f_types), formals_names(f_names), return_type(ret), defining_class(def) {}
};

// ==============================================================================
// ESTRUCTURA: AttrInfo
// ==============================================================================
// Almacena los metadatos de un atributo perteneciente a una clase COOL.
// Se utiliza para verificar que no haya sobreescritura (override) ilegal de atributos
// en subclases, y para poblar el ambiente de objetos (O) antes de tipar expresiones.
// ==============================================================================
struct AttrInfo {
    Symbol name;            // Nombre del atributo (ej. "age", "name"). Prohibido que sea "self".
    Symbol type_decl;       // Tipo declarado del atributo (ej. "Int", "String", "SELF_TYPE").
    Expression init;        // Expresión AST de inicialización (puede ser de tipo no_expr si no se inicializa).
    Class_ defining_class;  // Puntero al nodo AST de la clase que introdujo este atributo.

    // Constructor por defecto: inicializa punteros a NULL.
    AttrInfo() : name(NULL), type_decl(NULL), init(NULL), defining_class(NULL) {}

    // Constructor parametrizado: asigna los metadatos del atributo.
    AttrInfo(Symbol n, Symbol t, Expression i, Class_ def)
        : name(n), type_decl(t), init(i), defining_class(def) {}
};

// ==============================================================================
// CLASE: ClassTable
// ==============================================================================
// Orquestador central del análisis semántico. Administra:
// 1. El grafo de herencia entre clases y su validación acíclica.
// 2. Las tablas de métodos y atributos de cada clase.
// 3. El sistema de relaciones de tipos: conformidad (<=) y Least Upper Bound (lub).
// 4. El registro y emisión de errores semánticos formateados según el estándar CS143.
// ==============================================================================
class ClassTable {
private:
  int semant_errors;             // Contador acumulativo de errores semánticos detectados.
  void install_basic_classes();  // Registra en el grafo las 5 clases base de COOL: Object, IO, Int, Bool, String.
  ostream& error_stream;         // Flujo de salida hacia el cual se emiten los mensajes (generalmente cerr).

  // ---------------------------------------------------------------------------
  // Tablas de registro de clases y relaciones de herencia
  // ---------------------------------------------------------------------------
  std::map<Symbol, Class_> name_to_class;   // Mapea: NombreDeClase -> Puntero al nodo AST de la clase.
  std::map<Symbol, Symbol> parent_map;     // Mapea: NombreDeClase -> NombreDeLaClasePadre (grafo de herencia).
  std::vector<Class_> user_classes;         // Vector ordenado con las clases escritas por el usuario en el código.
  std::vector<Class_> basic_classes_list;   // Vector con las 5 clases básicas predefinidas del compilador.

  // ---------------------------------------------------------------------------
  // Tablas de símbolos de dos niveles para métodos y atributos
  // ---------------------------------------------------------------------------
  // class_methods[NombreClase][NombreMetodo] -> MethodInfo
  std::map<Symbol, std::map<Symbol, MethodInfo> > class_methods;

  // class_attributes[NombreClase][NombreAtributo] -> AttrInfo
  std::map<Symbol, std::map<Symbol, AttrInfo> > class_attributes;

public:
  // Constructor: recibe la lista de clases del AST y ejecuta el flujo completo de análisis semántico.
  ClassTable(Classes);

  // Retorna el número total de errores semánticos acumulados. Si es > 0, el compilador debe detenerse.
  int errors() { return semant_errors; }

  // Métodos sobrecargados para reporte de errores semánticos con archivo y línea:
  ostream& semant_error();                          // Incrementa semant_errors y devuelve el error_stream.
  ostream& semant_error(Class_ c);                  // Reporta error asociando el nombre de archivo y línea de la clase.
  ostream& semant_error(Symbol filename, tree_node *t); // Reporta error asociando el archivo y nodo AST exacto.

  // ---------------------------------------------------------------------------
  // Fases principales del flujo del analizador semántico (Pasadas)
  // ---------------------------------------------------------------------------
  void install_classes(Classes classes); // Fase 1: Registra clases de usuario verificando nombres y duplicados.
  void check_inheritance();              // Fase 2: Valida el grafo de herencia (padres válidos y ausencia de ciclos).
  void check_main();                     // Fase 3: Valida que exista la clase Main con método main() sin parámetros.
  void build_feature_tables();           // Fase 4 (Pasada 1): Recolecta firmas de métodos y declaraciones de atributos.
  void check_overrides();                // Fase 5: Valida invarianza de métodos sobreescritos y prohíbe override de atributos.
  void check_types();                    // Fase 6 (Pasada 2): Recorre el AST comprobando tipos de expresiones y cuerpos.

  // ---------------------------------------------------------------------------
  // Sistema de Tipos de COOL (Subtipado y LUB)
  // ---------------------------------------------------------------------------

  // conforms(T1, T2, C): Retorna true si T1 <= T2 en el contexto de la clase actual C.
  // Regla fundamental de subtipado: T1 conforma con T2 si T1 es idéntico a T2,
  // o si T2 es un ancestro de T1 en el grafo de herencia. Maneja SELF_TYPE rigurosamente.
  bool conforms(Symbol t1, Symbol t2, Class_ current_class);

  // lub(T1, T2, C): Least Upper Bound (mínimo ancestro común) entre T1 y T2 en la jerarquía.
  // Utilizado en sentencias if-then-else y en case para unificar los tipos de múltiples ramas.
  Symbol lub(Symbol t1, Symbol t2, Class_ current_class);

  // is_valid_type: Verifica si un tipo dado corresponde a una clase existente en name_to_class.
  // El parámetro allow_self_type permite aceptar SELF_TYPE cuando el contexto sintáctico lo permite.
  bool is_valid_type(Symbol type, bool allow_self_type = false);

  // lookup_method: Busca un método por nombre subiendo por la cadena de herencia desde class_name hasta Object.
  MethodInfo* lookup_method(Symbol class_name, Symbol method_name);

  // lookup_attr: Busca un atributo por nombre subiendo por la cadena de herencia.
  AttrInfo* lookup_attr(Symbol class_name, Symbol attr_name);

  // get_ancestors: Devuelve un vector con la cadena completa de ancestros desde class_name hasta Object.
  std::vector<Symbol> get_ancestors(Symbol class_name);
  
  // get_class: Obtiene el puntero Class_ al AST de una clase dado su Symbol, o NULL si no existe.
  Class_ get_class(Symbol name) {
      if (name_to_class.count(name)) return name_to_class[name];
      return NULL;
  }
};

#endif // SEMANT_H_
