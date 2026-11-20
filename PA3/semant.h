#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// Metadatos de firma para cada método definido o heredado en una clase COOL
struct MethodInfo {
    Symbol name;
    std::vector<Symbol> formals_types;
    std::vector<Symbol> formals_names;
    Symbol return_type;
    Class_ defining_class;

    MethodInfo() : name(NULL), return_type(NULL), defining_class(NULL) {}
    MethodInfo(Symbol n, const std::vector<Symbol>& f_types, const std::vector<Symbol>& f_names, Symbol ret, Class_ def)
        : name(n), formals_types(f_types), formals_names(f_names), return_type(ret), defining_class(def) {}
};

// Metadatos para cada atributo definido en una clase COOL
struct AttrInfo {
    Symbol name;
    Symbol type_decl;
    Expression init;
    Class_ defining_class;

    AttrInfo() : name(NULL), type_decl(NULL), init(NULL), defining_class(NULL) {}
    AttrInfo(Symbol n, Symbol t, Expression i, Class_ def)
        : name(n), type_decl(t), init(i), defining_class(def) {}
};

// Tabla de Clases y Grafo de Herencia (ClassTable)
// Responsable de construir la jerarquía de herencia, verificar consistencia estructural
// y orquestar las pasadas de análisis semántico y comprobación estática de tipos.
class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  ostream& error_stream;

  // Registro de clases y mapa de ancestros
  std::map<Symbol, Class_> name_to_class;
  std::map<Symbol, Symbol> parent_map;
  std::vector<Class_> user_classes;
  std::vector<Class_> basic_classes_list;

  // Tablas de símbolos de métodos y atributos por clase
  std::map<Symbol, std::map<Symbol, MethodInfo> > class_methods;
  std::map<Symbol, std::map<Symbol, AttrInfo> > class_attributes;

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);

  // Fases del analizador semántico
  void install_classes(Classes classes);
  void check_inheritance();
  void check_main();
  void build_feature_tables();
  void check_overrides();
  void check_types();

  // Sistema de tipos y reglas de conformidad
  bool conforms(Symbol t1, Symbol t2, Class_ current_class);
  Symbol lub(Symbol t1, Symbol t2, Class_ current_class);
  bool is_valid_type(Symbol type, bool allow_self_type = false);
  MethodInfo* lookup_method(Symbol class_name, Symbol method_name);
  AttrInfo* lookup_attr(Symbol class_name, Symbol attr_name);
  std::vector<Symbol> get_ancestors(Symbol class_name);
  
  Class_ get_class(Symbol name) {
      if (name_to_class.count(name)) return name_to_class[name];
      return NULL;
  }
};

#endif
