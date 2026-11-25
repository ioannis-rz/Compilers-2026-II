#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include "semant.h"
#include "utilities.h"

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// Símbolos globales predefinidos para representar tipos primitivos,
// métodos básicos y palabras clave requeridas por el entorno COOL.
//
//////////////////////////////////////////////////////////////////////
static Symbol arg, arg2, Bool, concat, cool_abort, copy, Int, in_int, 
              in_string, IO, length, Main, main_meth, No_class, No_type, 
              Object, out_int, out_string, prim_slot, self, SELF_TYPE, 
              Str, str_field, substr, type_name, val;

static void initialize_constants(void) {
  arg         = idtable.add_string("arg");
  arg2        = idtable.add_string("arg2");
  Bool        = idtable.add_string("Bool");
  concat      = idtable.add_string("concat");
  cool_abort  = idtable.add_string("abort");
  copy        = idtable.add_string("copy");
  Int         = idtable.add_string("Int");
  in_int      = idtable.add_string("in_int");
  in_string   = idtable.add_string("in_string");
  IO          = idtable.add_string("IO");
  length      = idtable.add_string("length");
  Main        = idtable.add_string("Main");
  main_meth   = idtable.add_string("main");
  No_class    = idtable.add_string("_no_class");
  No_type     = idtable.add_string("_no_type");
  Object      = idtable.add_string("Object");
  out_int     = idtable.add_string("out_int");
  out_string  = idtable.add_string("out_string");
  prim_slot   = idtable.add_string("_prim_slot");
  self        = idtable.add_string("self");
  SELF_TYPE   = idtable.add_string("SELF_TYPE");
  Str         = idtable.add_string("String");
  str_field   = idtable.add_string("_prim_slot");
  substr      = idtable.add_string("substr");
  type_name   = idtable.add_string("type_name");
  val         = idtable.add_string("_val");
}

//////////////////////////////////////////////////////////////////////
//
// ClassTable Constructor y Métodos de Inicialización
//
//////////////////////////////////////////////////////////////////////

ClassTable::ClassTable(Classes classes) : semant_errors(0), error_stream(cerr) {
    // 1. Instalar las 5 clases básicas estándar de COOL
    install_basic_classes();

    // 2. Registrar las clases definidas por el usuario
    install_classes(classes);

    // 3. Verificar la validez del grafo de herencia (ausencia de ciclos, padres válidos)
    check_inheritance();

    // Si el grafo de herencia tiene errores graves, detener inmediatamente
    if (errors() > 0) return;

    // 4. Verificar la presencia de la clase Main y su método main() sin parámetros
    check_main();

    // 5. Construir las tablas de firmas de métodos y atributos (Pasada 1)
    build_feature_tables();

    // 6. Verificar reglas de sobreescritura (override de métodos y no-override de atributos)
    check_overrides();

    // 7. Comprobación de tipos en cuerpos de métodos, inicializaciones y expresiones (Pasada 2)
    check_types();
}

void ClassTable::install_basic_classes() {
    // The tree package uses these globals to annotate the classes built below.
    // curr_lineno = 0;
    Symbol filename = stringtable.add_string("<basic class>");

    // Clases básicas de COOL: Object, IO, Int, Bool, String
    Class_ Object_class =
        class_(Object, 
               No_class,
               append_Features(
                   append_Features(
                       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
                       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
                   single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
               filename);

    Class_ IO_class = 
        class_(IO, 
               Object,
               append_Features(
                   append_Features(
                       append_Features(
                           single_Features(method(out_string, single_Formals(formal(arg, Str)), SELF_TYPE, no_expr())),
                           single_Features(method(out_int, single_Formals(formal(arg, Int)), SELF_TYPE, no_expr()))),
                       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
                   single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
               filename);

    Class_ Int_class =
        class_(Int, 
               Object,
               single_Features(attr(val, prim_slot, no_expr())),
               filename);

    Class_ Bool_class =
        class_(Bool, 
               Object,
               single_Features(attr(val, prim_slot, no_expr())),
               filename);

    Class_ String_class =
        class_(Str, 
               Object,
               append_Features(
                   append_Features(
                       single_Features(attr(val, Int, no_expr())),
                       single_Features(attr(str_field, prim_slot, no_expr()))),
                   append_Features(
                       append_Features(
                           single_Features(method(length, nil_Formals(), Int, no_expr())),
                           single_Features(method(concat, single_Formals(formal(arg, Str)), Str, no_expr()))),
                       single_Features(method(substr, 
                                              append_Formals(
                                                  single_Formals(formal(arg, Int)),
                                                  single_Formals(formal(arg2, Int))),
                                              Str, no_expr())))),
               filename);

    basic_classes_list.push_back(Object_class);
    basic_classes_list.push_back(IO_class);
    basic_classes_list.push_back(Int_class);
    basic_classes_list.push_back(Bool_class);
    basic_classes_list.push_back(String_class);

    for (size_t i = 0; i < basic_classes_list.size(); ++i) {
        Class_ c = basic_classes_list[i];
        name_to_class[c->get_name()] = c;
        parent_map[c->get_name()] = c->get_parent();
    }
}

void ClassTable::install_classes(Classes classes) {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ c = classes->nth(i);
        Symbol c_name = c->get_name();

        // Regla: No se puede llamar SELF_TYPE a una clase
        if (c_name == SELF_TYPE) {
            semant_error(c) << "Redefinition of basic class SELF_TYPE.\n";
            continue;
        }

        // Regla: No se pueden redefinir clases básicas
        if (c_name == Object || c_name == IO || c_name == Int || c_name == Bool || c_name == Str) {
            semant_error(c) << "Redefinition of basic class " << c_name << ".\n";
            continue;
        }

        // Regla: No se puede definir la misma clase más de una vez
        if (name_to_class.count(c_name) > 0) {
            semant_error(c) << "Class " << c_name << " was previously defined.\n";
            continue;
        }

        name_to_class[c_name] = c;
        parent_map[c_name] = c->get_parent();
        user_classes.push_back(c);
    }
}

void ClassTable::check_inheritance() {
    for (size_t i = 0; i < user_classes.size(); ++i) {
        Class_ c = user_classes[i];
        Symbol c_name = c->get_name();
        Symbol p_name = c->get_parent();

        // Regla: No se puede heredar de Int, String, Bool ni SELF_TYPE
        if (p_name == Int || p_name == Str || p_name == Bool || p_name == SELF_TYPE) {
            semant_error(c) << "Class " << c_name << " cannot inherit class " << p_name << ".\n";
            continue;
        }

        // Regla: Herencia de una clase no definida
        if (name_to_class.count(p_name) == 0) {
            semant_error(c) << "Class " << c_name << " inherits from an undefined class " << p_name << ".\n";
            continue;
        }

        // Regla: Ciclos en el grafo de herencia
        Symbol curr = p_name;
        std::set<Symbol> visited;
        visited.insert(c_name);

        while (curr != Object && curr != No_class && name_to_class.count(curr) > 0) {
            if (visited.count(curr) > 0) {
                semant_error(c) << "Class " << c_name << ", or an ancestor of " << c_name 
                                << ", is involved in an inheritance cycle.\n";
                break;
            }
            visited.insert(curr);
            curr = parent_map[curr];
        }
    }
}

void ClassTable::check_main() {
    // Regla: Debe existir la clase Main
    if (name_to_class.count(Main) == 0) {
        semant_error() << "Class Main is not defined.\n";
        return;
    }

    // Regla: La clase Main debe tener un método main() sin parámetros definido directamente en ella
    Class_ main_class = name_to_class[Main];
    Features f = main_class->get_features();
    bool found_main_meth = false;

    for (int i = f->first(); f->more(i); i = f->next(i)) {
        Feature feat = f->nth(i);
        if (feat->is_method()) {
            method_class *m = dynamic_cast<method_class*>(feat);
            if (m && m->get_name() == main_meth) {
                found_main_meth = true;
                if (m->get_formals()->len() != 0) {
                    semant_error(main_class) << "'main' method in class Main should have no arguments.\n";
                }
                break;
            }
        }
    }

    if (!found_main_meth) {
        semant_error(main_class) << "No 'main' method in class Main.\n";
    }
}

void ClassTable::build_feature_tables() {
    // 1. Procesar clases básicas
    for (size_t idx = 0; idx < basic_classes_list.size(); ++idx) {
        Class_ c = basic_classes_list[idx];
        Symbol c_name = c->get_name();
        Features f = c->get_features();
        for (int i = f->first(); f->more(i); i = f->next(i)) {
            Feature feat = f->nth(i);
            if (feat->is_method()) {
                method_class *m = dynamic_cast<method_class*>(feat);
                std::vector<Symbol> f_types;
                std::vector<Symbol> f_names;
                Formals formals = m->get_formals();
                for (int j = formals->first(); formals->more(j); j = formals->next(j)) {
                    Formal form = formals->nth(j);
                    f_types.push_back(form->get_type_decl());
                    f_names.push_back(form->get_name());
                }
                class_methods[c_name][m->get_name()] = MethodInfo(m->get_name(), f_types, f_names, m->get_return_type(), c);
            } else if (feat->is_attr()) {
                attr_class *a = dynamic_cast<attr_class*>(feat);
                class_attributes[c_name][a->get_name()] = AttrInfo(a->get_name(), a->get_type_decl(), a->get_init(), c);
            }
        }
    }

    // 2. Procesar clases de usuario y verificar declaraciones de métodos y atributos
    for (size_t idx = 0; idx < user_classes.size(); ++idx) {
        Class_ c = user_classes[idx];
        Symbol c_name = c->get_name();
        Features f = c->get_features();

        for (int i = f->first(); f->more(i); i = f->next(i)) {
            Feature feat = f->nth(i);
            if (feat->is_method()) {
                method_class *m = dynamic_cast<method_class*>(feat);
                Symbol m_name = m->get_name();

                // Regla: Método definido más de una vez en la misma clase
                if (class_methods[c_name].count(m_name) > 0) {
                    semant_error(c->get_filename(), m) << "Method " << m_name << " is multiply defined.\n";
                    continue;
                }

                // Validar parámetros formales
                std::set<Symbol> seen_formals;
                std::vector<Symbol> f_types;
                std::vector<Symbol> f_names;
                Formals formals = m->get_formals();

                for (int j = formals->first(); formals->more(j); j = formals->next(j)) {
                    Formal form = formals->nth(j);
                    Symbol f_name = form->get_name();
                    Symbol f_type = form->get_type_decl();

                    // Regla: Parámetro formal llamado 'self'
                    if (f_name == self) {
                        semant_error(c->get_filename(), form) << "'self' cannot be the name of a formal parameter.\n";
                    }

                    // Regla: Parámetros formales repetidos
                    if (seen_formals.count(f_name) > 0) {
                        semant_error(c->get_filename(), form) << "Formal parameter " << f_name << " is multiply defined.\n";
                    }
                    seen_formals.insert(f_name);

                    // Regla: Parámetro formal de tipo SELF_TYPE
                    if (f_type == SELF_TYPE) {
                        semant_error(c->get_filename(), form) << "Formal parameter " << f_name << " cannot have type SELF_TYPE.\n";
                    } else if (!is_valid_type(f_type, false)) {
                        // Regla: Tipo de parámetro que no corresponde a una clase definida
                        semant_error(c->get_filename(), form) << "Type " << f_type << " of parameter " << f_name << " is undefined.\n";
                    }

                    f_types.push_back(f_type);
                    f_names.push_back(f_name);
                }

                // Regla: Tipo de retorno debe ser una clase definida o SELF_TYPE
                Symbol ret_type = m->get_return_type();
                if (!is_valid_type(ret_type, true)) {
                    semant_error(c->get_filename(), m) << "Undefined return type " << ret_type << " in method " << m_name << ".\n";
                }

                class_methods[c_name][m_name] = MethodInfo(m_name, f_types, f_names, ret_type, c);

            } else if (feat->is_attr()) {
                attr_class *a = dynamic_cast<attr_class*>(feat);
                Symbol a_name = a->get_name();
                Symbol a_type = a->get_type_decl();

                // Regla: Un atributo llamado 'self'
                if (a_name == self) {
                    semant_error(c->get_filename(), a) << "'self' cannot be the name of an attribute.\n";
                    continue;
                }

                // Regla: Atributo definido más de una vez en la misma clase
                if (class_attributes[c_name].count(a_name) > 0) {
                    semant_error(c->get_filename(), a) << "Attribute " << a_name << " is multiply defined.\n";
                    continue;
                }

                // Regla: Tipo de atributo que no corresponde a una clase definida (SELF_TYPE sí permitido)
                if (!is_valid_type(a_type, true)) {
                    semant_error(c->get_filename(), a) << "Type " << a_type << " of attribute " << a_name << " is undefined.\n";
                }

                class_attributes[c_name][a_name] = AttrInfo(a_name, a_type, a->get_init(), c);
            }
        }
    }
}

void ClassTable::check_overrides() {
    for (size_t idx = 0; idx < user_classes.size(); ++idx) {
        Class_ c = user_classes[idx];
        Symbol c_name = c->get_name();
        std::vector<Symbol> ancestors = get_ancestors(c_name);

        // 1. Regla: Atributo heredado redefinido en una subclase
        for (std::map<Symbol, AttrInfo>::iterator it = class_attributes[c_name].begin();
             it != class_attributes[c_name].end(); ++it) {
            Symbol a_name = it->first;
            // Buscar en todos los ancestros de c_name
            for (size_t a_idx = 1; a_idx < ancestors.size(); ++a_idx) {
                Symbol anc = ancestors[a_idx];
                if (class_attributes[anc].count(a_name) > 0) {
                    semant_error(c->get_filename(), it->second.defining_class)
                        << "Attribute " << a_name << " is an attribute of an inherited class.\n";
                    break;
                }
            }
        }

        // 2. Regla: Redefinición (override) de métodos heredados
        for (std::map<Symbol, MethodInfo>::iterator it = class_methods[c_name].begin();
             it != class_methods[c_name].end(); ++it) {
            Symbol m_name = it->first;
            MethodInfo &sub_m = it->second;

            // Buscar en los ancestros hacia arriba
            for (size_t a_idx = 1; a_idx < ancestors.size(); ++a_idx) {
                Symbol anc = ancestors[a_idx];
                if (class_methods[anc].count(m_name) > 0) {
                    MethodInfo &anc_m = class_methods[anc][m_name];

                    // Verificar número de parámetros
                    if (sub_m.formals_types.size() != anc_m.formals_types.size()) {
                        semant_error(c->get_filename(), sub_m.defining_class)
                            << "Incompatible number of formal parameters in redefined method " << m_name << ".\n";
                        break;
                    }

                    // Verificar tipo de cada parámetro
                    bool param_err = false;
                    for (size_t k = 0; k < sub_m.formals_types.size(); ++k) {
                        if (sub_m.formals_types[k] != anc_m.formals_types[k]) {
                            semant_error(c->get_filename(), sub_m.defining_class)
                                << "In redefined method " << m_name << ", parameter type " << sub_m.formals_types[k]
                                << " is different from original type " << anc_m.formals_types[k] << ".\n";
                            param_err = true;
                            break;
                        }
                    }
                    if (param_err) break;

                    // Verificar tipo de retorno
                    if (sub_m.return_type != anc_m.return_type) {
                        semant_error(c->get_filename(), sub_m.defining_class)
                            << "In redefined method " << m_name << ", return type " << sub_m.return_type
                            << " is different from original type " << anc_m.return_type << ".\n";
                        break;
                    }

                    break; // Solo chequear contra el ancestro más cercano que define el método
                }
            }
        }
    }
}

void ClassTable::check_types() {
    for (size_t idx = 0; idx < user_classes.size(); ++idx) {
        Class_ c = user_classes[idx];
        Symbol c_name = c->get_name();

        // Construir el ambiente de objetos (O) con self y todos los atributos heredados y propios
        SymbolTable<Symbol, Entry> object_env;
        object_env.enterscope();

        // En todas las clases, self está ligado implícitamente a SELF_TYPE
        object_env.addid(self, SELF_TYPE);

        // Recorrer la cadena de herencia desde Object hacia c_name para poblar atributos
        std::vector<Symbol> ancestors = get_ancestors(c_name);
        for (int a_idx = (int)ancestors.size() - 1; a_idx >= 0; --a_idx) {
            Symbol anc = ancestors[a_idx];
            for (std::map<Symbol, AttrInfo>::iterator it = class_attributes[anc].begin();
                 it != class_attributes[anc].end(); ++it) {
                object_env.addid(it->second.name, it->second.type_decl);
            }
        }

        // Comprobar inicializaciones de atributos propios de la clase
        Features f = c->get_features();
        for (int i = f->first(); f->more(i); i = f->next(i)) {
            Feature feat = f->nth(i);
            if (feat->is_attr()) {
                attr_class *a = dynamic_cast<attr_class*>(feat);
                Expression init = a->get_init();
                Symbol init_type = init->type_check(this, c, object_env);

                if (init_type != No_type) {
                    if (!conforms(init_type, a->get_type_decl(), c)) {
                        semant_error(c->get_filename(), a)
                            << "Inferred type " << init_type << " of initialization of attribute "
                            << a->get_name() << " does not conform to declared type " << a->get_type_decl() << ".\n";
                    }
                }
            }
        }

        // Comprobar cuerpos de métodos
        for (int i = f->first(); f->more(i); i = f->next(i)) {
            Feature feat = f->nth(i);
            if (feat->is_method()) {
                method_class *m = dynamic_cast<method_class*>(feat);
                object_env.enterscope();

                // Agregar parámetros formales al ambiente
                Formals formals = m->get_formals();
                for (int j = formals->first(); formals->more(j); j = formals->next(j)) {
                    Formal form = formals->nth(j);
                    object_env.addid(form->get_name(), form->get_type_decl());
                }

                // Evaluar tipo del cuerpo del método
                Symbol body_type = m->get_expr()->type_check(this, c, object_env);
                Symbol ret_type = m->get_return_type();

                // Regla especial [Method]: Si el tipo de retorno declarado es SELF_TYPE,
                // el cuerpo DEBE inferirse exactamente como SELF_TYPE.
                if (ret_type == SELF_TYPE) {
                    if (body_type != SELF_TYPE) {
                        semant_error(c->get_filename(), m)
                            << "Inferred return type " << body_type << " of method " << m->get_name()
                            << " does not conform to declared return type SELF_TYPE.\n";
                    }
                } else {
                    if (!conforms(body_type, ret_type, c)) {
                        semant_error(c->get_filename(), m)
                            << "Inferred return type " << body_type << " of method " << m->get_name()
                            << " does not conform to declared return type " << ret_type << ".\n";
                    }
                }

                object_env.exitscope();
            }
        }

        object_env.exitscope();
    }
}

//////////////////////////////////////////////////////////////////////
//
// Sistema de Tipos: Conformidad (<=), LUB (Join) y Utilidades
//
//////////////////////////////////////////////////////////////////////

bool ClassTable::is_valid_type(Symbol type, bool allow_self_type) {
    if (allow_self_type && type == SELF_TYPE) return true;
    return name_to_class.count(type) > 0;
}

std::vector<Symbol> ClassTable::get_ancestors(Symbol class_name) {
    std::vector<Symbol> ancestors;
    Symbol curr = class_name;
    while (curr != No_class && curr != NULL && name_to_class.count(curr) > 0) {
        ancestors.push_back(curr);
        if (curr == Object) break;
        curr = parent_map[curr];
    }
    return ancestors;
}

bool ClassTable::conforms(Symbol t1, Symbol t2, Class_ current_class) {
    // 1. Expresión vacía (no_expr) conforma con cualquier tipo
    if (t1 == No_type) return true;

    // 2. Reflexividad
    if (t1 == t2) return true;

    // 3. T1 <= SELF_TYPE solo si T1 == SELF_TYPE
    if (t2 == SELF_TYPE) return false;

    // 4. SELF_TYPE <= T2 <=> clase_actual <= T2
    if (t1 == SELF_TYPE) {
        return conforms(current_class->get_name(), t2, current_class);
    }

    // 5. Clases normales: T1 <= T2 <=> T2 es ancestro de T1 en el árbol de herencia
    Symbol curr = t1;
    while (curr != No_class && curr != NULL && name_to_class.count(curr) > 0) {
        if (curr == t2) return true;
        if (curr == Object) break;
        curr = parent_map[curr];
    }

    return false;
}

Symbol ClassTable::lub(Symbol t1, Symbol t2, Class_ current_class) {
    // 1. Igualdad directa
    if (t1 == t2) return t1;

    // 2. SELF_TYPE con SELF_TYPE
    if (t1 == SELF_TYPE && t2 == SELF_TYPE) return SELF_TYPE;

    // 3. SELF_TYPE con tipo normal: resolver a clase actual
    if (t1 == SELF_TYPE) return lub(current_class->get_name(), t2, current_class);
    if (t2 == SELF_TYPE) return lub(t1, current_class->get_name(), current_class);

    // 4. Mínimo ancestro común en la jerarquía de herencia
    std::vector<Symbol> anc1 = get_ancestors(t1);
    std::vector<Symbol> anc2 = get_ancestors(t2);

    for (size_t i = 0; i < anc1.size(); ++i) {
        for (size_t j = 0; j < anc2.size(); ++j) {
            if (anc1[i] == anc2[j]) {
                return anc1[i];
            }
        }
    }

    return Object;
}

MethodInfo* ClassTable::lookup_method(Symbol class_name, Symbol method_name) {
    Symbol curr = class_name;
    while (curr != No_class && curr != NULL && name_to_class.count(curr) > 0) {
        if (class_methods.count(curr) > 0 && class_methods[curr].count(method_name) > 0) {
            return &class_methods[curr][method_name];
        }
        if (curr == Object) break;
        curr = parent_map[curr];
    }
    return NULL;
}

AttrInfo* ClassTable::lookup_attr(Symbol class_name, Symbol attr_name) {
    Symbol curr = class_name;
    while (curr != No_class && curr != NULL && name_to_class.count(curr) > 0) {
        if (class_attributes.count(curr) > 0 && class_attributes[curr].count(attr_name) > 0) {
            return &class_attributes[curr][attr_name];
        }
        if (curr == Object) break;
        curr = parent_map[curr];
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////
//
// Notificación de Errores Semánticos
//
//////////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c) {                                                             
    return semant_error(c->get_filename(), c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t) {
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error() {                                                 
    semant_errors++;                            
    return error_stream;
} 

//////////////////////////////////////////////////////////////////////
//
// Comprobación de Tipos en Expresiones (Reglas Estáticas de COOL)
//
//////////////////////////////////////////////////////////////////////

// Regla [Assign]
Symbol assign_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    if (name == self) {
        ct->semant_error(cur_class->get_filename(), this) << "Cannot assign to 'self'.\n";
        set_type(Object);
        return Object;
    }

    Symbol var_type = o_table.lookup(name);
    if (var_type == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "Assignment to undeclared variable " << name << ".\n";
        var_type = Object;
    }

    Symbol expr_type = expr->type_check(ct, cur_class, o_table);

    if (var_type != Object && !ct->conforms(expr_type, var_type, cur_class)) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Type " << expr_type << " of assigned expression does not conform to declared type "
            << var_type << " of identifier " << name << ".\n";
    }

    set_type(expr_type);
    return expr_type;
}

// Regla [StaticDispatch]
Symbol static_dispatch_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t0 = expr->type_check(ct, cur_class, o_table);

    if (type_name == SELF_TYPE) {
        ct->semant_error(cur_class->get_filename(), this) << "Static dispatch to SELF_TYPE is illegal.\n";
        set_type(Object);
        return Object;
    }

    if (ct->get_class(type_name) == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "Static dispatch to undefined class " << type_name << ".\n";
        set_type(Object);
        return Object;
    }

    if (!ct->conforms(t0, type_name, cur_class)) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Expression type " << t0 << " does not conform to declared static dispatch type " << type_name << ".\n";
    }

    MethodInfo* m = ct->lookup_method(type_name, name);
    if (m == NULL) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Static dispatch to undefined method " << name << ".\n";
        set_type(Object);
        return Object;
    }

    if (actual->len() != (int)m->formals_types.size()) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Method " << name << " called with wrong number of arguments.\n";
    }

    for (int k = actual->first(); actual->more(k); k = actual->next(k)) {
        Symbol a_type = actual->nth(k)->type_check(ct, cur_class, o_table);
        if (k < (int)m->formals_types.size()) {
            if (!ct->conforms(a_type, m->formals_types[k], cur_class)) {
                ct->semant_error(cur_class->get_filename(), this)
                    << "In call of method " << name << ", type " << a_type << " of parameter "
                    << m->formals_names[k] << " does not conform to declared type " << m->formals_types[k] << ".\n";
            }
        }
    }

    Symbol result_type = m->return_type;
    if (result_type == SELF_TYPE) {
        result_type = t0;
    }

    set_type(result_type);
    return result_type;
}

// Regla [Dispatch]
Symbol dispatch_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t0 = expr->type_check(ct, cur_class, o_table);
    Symbol c_disp = (t0 == SELF_TYPE) ? cur_class->get_name() : t0;

    if (ct->get_class(c_disp) == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "Dispatch on undefined class " << c_disp << ".\n";
        set_type(Object);
        return Object;
    }

    MethodInfo* m = ct->lookup_method(c_disp, name);
    if (m == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "Dispatch to undefined method " << name << ".\n";
        set_type(Object);
        return Object;
    }

    if (actual->len() != (int)m->formals_types.size()) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Method " << name << " called with wrong number of arguments.\n";
    }

    for (int k = actual->first(); actual->more(k); k = actual->next(k)) {
        Symbol a_type = actual->nth(k)->type_check(ct, cur_class, o_table);
        if (k < (int)m->formals_types.size()) {
            if (!ct->conforms(a_type, m->formals_types[k], cur_class)) {
                ct->semant_error(cur_class->get_filename(), this)
                    << "In call of method " << name << ", type " << a_type << " of parameter "
                    << m->formals_names[k] << " does not conform to declared type " << m->formals_types[k] << ".\n";
            }
        }
    }

    Symbol result_type = m->return_type;
    if (result_type == SELF_TYPE) {
        result_type = t0;
    }

    set_type(result_type);
    return result_type;
}

// Regla [If]
Symbol cond_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol pt = pred->type_check(ct, cur_class, o_table);
    if (pt != Bool) {
        ct->semant_error(cur_class->get_filename(), this) << "Predicate of 'if' does not have type Bool.\n";
    }

    Symbol t1 = then_exp->type_check(ct, cur_class, o_table);
    Symbol t2 = else_exp->type_check(ct, cur_class, o_table);

    Symbol res = ct->lub(t1, t2, cur_class);
    set_type(res);
    return res;
}

// Regla [Loop]
Symbol loop_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol pt = pred->type_check(ct, cur_class, o_table);
    if (pt != Bool) {
        ct->semant_error(cur_class->get_filename(), this) << "Loop condition does not have type Bool.\n";
    }

    body->type_check(ct, cur_class, o_table);
    set_type(Object);
    return Object;
}

// Regla [Case]
Symbol typcase_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    expr->type_check(ct, cur_class, o_table);

    std::set<Symbol> seen_types;
    std::vector<Symbol> branch_results;

    for (int k = cases->first(); cases->more(k); k = cases->next(k)) {
        Case br = cases->nth(k);
        Symbol br_name = br->get_name();
        Symbol br_type = br->get_type_decl();

        if (br_name == self) {
            ct->semant_error(cur_class->get_filename(), br) << "'self' bound in 'case'.\n";
        }

        if (br_type == SELF_TYPE) {
            ct->semant_error(cur_class->get_filename(), br)
                << "Identifier " << br_name << " declared with type SELF_TYPE in case branch.\n";
        } else if (ct->get_class(br_type) == NULL) {
            ct->semant_error(cur_class->get_filename(), br)
                << "Class " << br_type << " of case branch is undefined.\n";
        }

        if (seen_types.count(br_type) > 0) {
            ct->semant_error(cur_class->get_filename(), br)
                << "Duplicate branch " << br_type << " in case statement.\n";
        }
        seen_types.insert(br_type);

        o_table.enterscope();
        o_table.addid(br_name, br_type);
        Symbol b_res = br->get_expr()->type_check(ct, cur_class, o_table);
        o_table.exitscope();

        branch_results.push_back(b_res);
    }

    Symbol res = Object;
    if (!branch_results.empty()) {
        res = branch_results[0];
        for (size_t i = 1; i < branch_results.size(); ++i) {
            res = ct->lub(res, branch_results[i], cur_class);
        }
    }

    set_type(res);
    return res;
}

// Regla [Block]
Symbol block_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol last_t = Object;
    for (int k = body->first(); body->more(k); k = body->next(k)) {
        last_t = body->nth(k)->type_check(ct, cur_class, o_table);
    }
    set_type(last_t);
    return last_t;
}

// Regla [Let]
Symbol let_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    if (identifier == self) {
        ct->semant_error(cur_class->get_filename(), this) << "'self' cannot be bound in a 'let' expression.\n";
    }

    if (type_decl != SELF_TYPE && ct->get_class(type_decl) == NULL) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Type " << type_decl << " of let-bound identifier " << identifier << " is undefined.\n";
    }

    // La inicialización se evalúa en el ambiente que aún NO contiene la nueva variable
    Symbol init_t = init->type_check(ct, cur_class, o_table);

    if (init_t != No_type) {
        if (!ct->conforms(init_t, type_decl, cur_class)) {
            ct->semant_error(cur_class->get_filename(), this)
                << "Inferred type " << init_t << " of initialization of " << identifier
                << " does not conform to identifier's declared type " << type_decl << ".\n";
        }
    }

    // El cuerpo se evalúa en el ambiente con la nueva variable ligada
    o_table.enterscope();
    o_table.addid(identifier, type_decl);
    Symbol body_t = body->type_check(ct, cur_class, o_table);
    o_table.exitscope();

    set_type(body_t);
    return body_t;
}

// Reglas aritméticas [Plus, Sub, Mul, Divide]
Symbol plus_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    Symbol t2 = e2->type_check(ct, cur_class, o_table);
    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "non-Int arguments: " << t1 << " + " << t2 << "\n";
    }
    set_type(Int);
    return Int;
}

Symbol sub_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    Symbol t2 = e2->type_check(ct, cur_class, o_table);
    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "non-Int arguments: " << t1 << " - " << t2 << "\n";
    }
    set_type(Int);
    return Int;
}

Symbol mul_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    Symbol t2 = e2->type_check(ct, cur_class, o_table);
    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "non-Int arguments: " << t1 << " * " << t2 << "\n";
    }
    set_type(Int);
    return Int;
}

Symbol divide_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    Symbol t2 = e2->type_check(ct, cur_class, o_table);
    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "non-Int arguments: " << t1 << " / " << t2 << "\n";
    }
    set_type(Int);
    return Int;
}

// Regla [Neg] (~)
Symbol neg_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    if (t1 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "Argument of '~' has type " << t1 << " instead of Int.\n";
    }
    set_type(Int);
    return Int;
}

// Reglas de comparación [Lt, Leq]
Symbol lt_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    Symbol t2 = e2->type_check(ct, cur_class, o_table);
    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "non-Int arguments: " << t1 << " < " << t2 << "\n";
    }
    set_type(Bool);
    return Bool;
}

Symbol leq_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    Symbol t2 = e2->type_check(ct, cur_class, o_table);
    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "non-Int arguments: " << t1 << " <= " << t2 << "\n";
    }
    set_type(Bool);
    return Bool;
}

// Regla [Equal] (=)
Symbol eq_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    Symbol t2 = e2->type_check(ct, cur_class, o_table);

    if (t1 == Int || t2 == Int || t1 == Bool || t2 == Bool || t1 == Str || t2 == Str) {
        if (t1 != t2) {
            ct->semant_error(cur_class->get_filename(), this) << "Illegal comparison with a basic type.\n";
        }
    }

    set_type(Bool);
    return Bool;
}

// Regla [Comp] (not)
Symbol comp_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    if (t1 != Bool) {
        ct->semant_error(cur_class->get_filename(), this) << "Argument of 'not' has type " << t1 << " instead of Bool.\n";
    }
    set_type(Bool);
    return Bool;
}

// Constantes y Literales
Symbol int_const_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    set_type(Int);
    return Int;
}

Symbol bool_const_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    set_type(Bool);
    return Bool;
}

Symbol string_const_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    set_type(Str);
    return Str;
}

// Regla [New]
Symbol new__class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    if (type_name != SELF_TYPE && ct->get_class(type_name) == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "'new' used with undefined class " << type_name << ".\n";
        set_type(Object);
        return Object;
    }
    set_type(type_name);
    return type_name;
}

// Regla [Isvoid]
Symbol isvoid_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    e1->type_check(ct, cur_class, o_table);
    set_type(Bool);
    return Bool;
}

// Regla [No_expr]
Symbol no_expr_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    set_type(No_type);
    return No_type;
}

// Regla [Object] (identificador)
Symbol object_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    if (name == self) {
        set_type(SELF_TYPE);
        return SELF_TYPE;
    }

    Symbol found = o_table.lookup(name);
    if (found == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "Undeclared identifier " << name << ".\n";
        set_type(Object);
        return Object;
    }

    set_type(found);
    return found;
}

//////////////////////////////////////////////////////////////////////
//
// Punto de Entrada Principal al Análisis Semántico
//
//////////////////////////////////////////////////////////////////////

void program_class::semant() {
    initialize_constants();
    ClassTable *classtable = new ClassTable(classes);

    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}
