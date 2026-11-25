#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include "semant.h"
#include "utilities.h"

// ==============================================================================
// ARCHIVO: semant_comentado.cc
// PROYECTO: Laboratorio 03 - Analizador Semántico para COOL (PA4)
// ASIGNATURA: Compiladores, Universidad Nacional de Colombia (2026-II)
// ==============================================================================
// PROPÓSITO:
// Implementación completa, rigurosa y didáctica del analizador semántico estático
// para el lenguaje COOL (Classroom Object-Oriented Language).
//
// Este archivo contiene explicaciones exhaustivas sobre cada función, estructura
// de datos, regla de inferencia formal y decisión de diseño, sirviendo como
// material definitivo para la sustentación oral.
// ==============================================================================

// Banderas externas del entorno de compilación de CS143:
extern int semant_debug;       // Si se activa con '-s', imprime trazas de depuración del analizador.
extern char *curr_filename;    // Nombre del archivo actual procesado por el driver de compilación.

//////////////////////////////////////////////////////////////////////
//
// Símbolos Globales Predefinidos
//
// En el runtime de CS143, los identificadores son punteros a entradas de la
// tabla de cadenas ('idtable'). La comparación entre símbolos se realiza
// mediante punteros (==) para máxima eficiencia.
//
//////////////////////////////////////////////////////////////////////
static Symbol arg, arg2, Bool, concat, cool_abort, copy, Int, in_int, 
              in_string, IO, length, Main, main_meth, No_class, No_type, 
              Object, out_int, out_string, prim_slot, self, SELF_TYPE, 
              Str, str_field, substr, type_name, val;

// ==============================================================================
// FUNCIÓN: initialize_constants
// ==============================================================================
// Registra en 'idtable' los identificadores canónicos de tipos primitivos,
// palabras clave y nombres de métodos integrados del lenguaje COOL.
// Es llamada una única vez al inicio de 'program_class::semant()'.
// ==============================================================================
static void initialize_constants(void) {
  arg         = idtable.add_string("arg");          // Nombre por defecto para parámetros de métodos integrados.
  arg2        = idtable.add_string("arg2");         // Segundo parámetro para substr(arg, arg2).
  Bool        = idtable.add_string("Bool");         // Identificador del tipo booleano primitivo.
  concat      = idtable.add_string("concat");       // Método String::concat(String): String.
  cool_abort  = idtable.add_string("abort");        // Método Object::abort(): Object.
  copy        = idtable.add_string("copy");         // Método Object::copy(): SELF_TYPE.
  Int         = idtable.add_string("Int");          // Identificador del tipo entero primitivo de 32 bits.
  in_int      = idtable.add_string("in_int");       // Método IO::in_int(): Int.
  in_string   = idtable.add_string("in_string");    // Método IO::in_string(): String.
  IO          = idtable.add_string("IO");           // Clase integrada de entrada/salida.
  length      = idtable.add_string("length");       // Método String::length(): Int.
  Main        = idtable.add_string("Main");         // Nombre obligatorio de la clase principal del programa.
  main_meth   = idtable.add_string("main");         // Nombre obligatorio del método de entrada en Main.
  No_class    = idtable.add_string("_no_class");    // Centinela que representa la ausencia de clase padre (padre de Object).
  No_type     = idtable.add_string("_no_type");     // Centinela para expresiones sin inicialización (no_expr).
  Object      = idtable.add_string("Object");       // Raíz de toda la jerarquía de herencia de COOL.
  out_int     = idtable.add_string("out_int");      // Método IO::out_int(Int): SELF_TYPE.
  out_string  = idtable.add_string("out_string");   // Método IO::out_string(String): SELF_TYPE.
  prim_slot   = idtable.add_string("_prim_slot");   // Representación interna en memoria de tipos primitivos.
  self        = idtable.add_string("self");         // Identificador reservado del objeto receptor actual.
  SELF_TYPE   = idtable.add_string("SELF_TYPE");    // Metatipo que denota la clase dinámica del objeto actual.
  Str         = idtable.add_string("String");       // En COOL el tipo se escribe "String"; internamente usamos el puntero 'Str'.
  str_field   = idtable.add_string("_prim_slot");   // Ranura primitiva interna para strings.
  substr      = idtable.add_string("substr");       // Método String::substr(Int, Int): String.
  type_name   = idtable.add_string("type_name");    // Método Object::type_name(): String.
  val         = idtable.add_string("_val");         // Ranura interna que almacena el valor escalar de Int y Bool.
}

//////////////////////////////////////////////////////////////////////
//
// Constructor de ClassTable y Flujo Principal de Análisis Semántico
//
// Ejecuta el pipeline en 7 fases ordenadas. La separación en fases
// garantiza que las dependencias mutuas entre clases se resuelvan
// adecuadamente antes de comprobar tipos en las expresiones.
//
//////////////////////////////////////////////////////////////////////

ClassTable::ClassTable(Classes classes) : semant_errors(0), error_stream(cerr) {
    // -------------------------------------------------------------------------
    // FASE 1: Instalar las 5 clases básicas estándar de COOL en el grafo.
    // -------------------------------------------------------------------------
    install_basic_classes();

    // -------------------------------------------------------------------------
    // FASE 2: Registrar las clases definidas por el usuario en el archivo .cl.
    // Detecta: Clases duplicadas (A.1), redefinición de clases básicas (A.2),
    // y uso de SELF_TYPE como nombre de clase (A.3).
    // -------------------------------------------------------------------------
    install_classes(classes);

    // -------------------------------------------------------------------------
    // FASE 3: Validar la integridad del grafo de herencia.
    // Detecta: Herencia de clases selladas Int/Bool/String/SELF_TYPE (A.5),
    // herencia de clases no definidas (A.4), y ciclos en la herencia (A.6).
    // -------------------------------------------------------------------------
    check_inheritance();

    // Invariante de seguridad: si el grafo de herencia tiene ciclos o padres inexistentes,
    // es imposible continuar porque las búsquedas de ancestros entrarían en loops infinitos.
    if (errors() > 0) return;

    // -------------------------------------------------------------------------
    // FASE 4: Verificar la presencia obligatoria de la clase Main y su método main().
    // Detecta: Ausencia de Main (A.7), ausencia de main(), o main con parámetros (A.8).
    // -------------------------------------------------------------------------
    check_main();

    // -------------------------------------------------------------------------
    // FASE 5 (Pasada 1): Recolectar firmas de métodos y declaraciones de atributos.
    // Detecta: Métodos/atributos duplicados en la misma clase (B.2, C.1),
    // parámetros con nombres duplicados (C.2), 'self' como parámetro/atributo (B.1, C.3),
    // SELF_TYPE como parámetro (C.4), y tipos indefinidos en firmas (B.4, C.5).
    // -------------------------------------------------------------------------
    build_feature_tables();

    // -------------------------------------------------------------------------
    // FASE 6: Verificar reglas de sobreescritura (Overrides) entre clases y ancestros.
    // Detecta: Atributos heredados redefinidos (B.3) y sobreescritura de métodos con
    // firmas incompatibles en número de parámetros, tipo de parámetros o retorno (C.7).
    // -------------------------------------------------------------------------
    check_overrides();

    // -------------------------------------------------------------------------
    // FASE 7 (Pasada 2): Comprobación estática de tipos en el cuerpo de cada método,
    // en inicializadores de atributos y en todas las expresiones del AST.
    // Detecta: Todos los errores de las categorías D (alcance) y E (expresiones).
    // -------------------------------------------------------------------------
    check_types();
}

// ==============================================================================
// MÉTODO: install_basic_classes
// ==============================================================================
// Crea manualmente los nodos AST para las 5 clases base de COOL:
// 1. Object: raíz sin padre (_no_class), define abort, type_name, copy.
// 2. IO: hereda de Object, define out_string, out_int, in_string, in_int.
// 3. Int: hereda de Object, envuelve una ranura primitiva _val.
// 4. Bool: hereda de Object, envuelve una ranura primitiva _val.
// 5. String: hereda de Object, define length, concat, substr.
// ==============================================================================
void ClassTable::install_basic_classes() {
    // NOTA TÉCNICA CRÍTICA:
    // En el esqueleto oficial de Stanford CS143, 'curr_lineno' se encuentra comentado
    // porque en PA4 no se utiliza el analizador léxico/sintáctico sino el AST preconstruido.
    // Descomentar esta línea causa error de compilación: 'curr_lineno was not declared in this scope'.
    // // curr_lineno = 0;

    Symbol filename = stringtable.add_string("<basic class>");

    // 1. Clase Object (sin padre: parent = No_class)
    Class_ Object_class =
        class_(Object, 
               No_class,
               append_Features(
                   append_Features(
                       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
                       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
                   single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
               filename);

    // 2. Clase IO (hereda de Object)
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

    // 3. Clase Int (hereda de Object, clase sellada)
    Class_ Int_class =
        class_(Int, 
               Object,
               single_Features(attr(val, prim_slot, no_expr())),
               filename);

    // 4. Clase Bool (hereda de Object, clase sellada)
    Class_ Bool_class =
        class_(Bool, 
               Object,
               single_Features(attr(val, prim_slot, no_expr())),
               filename);

    // 5. Clase String (hereda de Object, clase sellada)
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

    // Guardar las clases básicas en la lista y registrarlas en las tablas de búsqueda
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

// ==============================================================================
// MÉTODO: install_classes
// ==============================================================================
// Itera sobre las clases enviadas por el parser y las registra en 'name_to_class'
// y 'parent_map'. Aplica validaciones inmediatas sobre el identificador de clase.
// ==============================================================================
void ClassTable::install_classes(Classes classes) {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ c = classes->nth(i);
        Symbol c_name = c->get_name();

        // Regla A.3: No se puede llamar SELF_TYPE a una clase de usuario.
        if (c_name == SELF_TYPE) {
            semant_error(c) << "Redefinition of basic class SELF_TYPE.\n";
            continue;
        }

        // Regla A.2: No se pueden redefinir clases básicas integradas.
        if (c_name == Object || c_name == IO || c_name == Int || c_name == Bool || c_name == Str) {
            semant_error(c) << "Redefinition of basic class " << c_name << ".\n";
            continue;
        }

        // Regla A.1: No se puede definir la misma clase más de una vez en el programa.
        if (name_to_class.count(c_name) > 0) {
            semant_error(c) << "Class " << c_name << " was previously defined.\n";
            continue;
        }

        // Si es válida, se inserta en los mapas y en el vector de clases de usuario:
        name_to_class[c_name] = c;
        parent_map[c_name] = c->get_parent();
        user_classes.push_back(c);
    }
}

// ==============================================================================
// MÉTODO: check_inheritance
// ==============================================================================
// Valida que el grafo de herencia sea un árbol dirigido acíclico bien fundamentado.
// ==============================================================================
void ClassTable::check_inheritance() {
    for (size_t i = 0; i < user_classes.size(); ++i) {
        Class_ c = user_classes[i];
        Symbol c_name = c->get_name();
        Symbol p_name = c->get_parent();

        // Regla A.5: Prohibido heredar de clases selladas o de SELF_TYPE.
        if (p_name == Int || p_name == Str || p_name == Bool || p_name == SELF_TYPE) {
            semant_error(c) << "Class " << c_name << " cannot inherit class " << p_name << ".\n";
            continue;
        }

        // Regla A.4: Herencia de una clase inexistente / no definida.
        if (name_to_class.count(p_name) == 0) {
            semant_error(c) << "Class " << c_name << " inherits from an undefined class " << p_name << ".\n";
            continue;
        }

        // Regla A.6: Detección de ciclos en la cadena de herencia.
        // Algoritmo: Comenzamos en 'c_name' y seguimos los punteros 'parent_map'
        // registrando cada clase en un conjunto 'visited'. Si encontramos una clase
        // que ya estaba en el conjunto antes de llegar a Object, hay un ciclo.
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

// ==============================================================================
// MÉTODO: check_main
// ==============================================================================
// Comprueba los requisitos obligatorios de punto de entrada del programa COOL:
// 1. Debe existir una clase llamada 'Main' (Regla A.7).
// 2. 'Main' debe definir un método 'main()' que tome 0 argumentos (Regla A.8).
// ==============================================================================
void ClassTable::check_main() {
    if (name_to_class.count(Main) == 0) {
        semant_error() << "Class Main is not defined.\n";
        return;
    }

    Class_ main_class = name_to_class[Main];
    Features f = main_class->get_features();
    bool found_main_meth = false;

    for (int i = f->first(); f->more(i); i = f->next(i)) {
        Feature feat = f->nth(i);
        if (feat->is_method()) {
            method_class *m = dynamic_cast<method_class*>(feat);
            if (m && m->get_name() == main_meth) {
                found_main_meth = true;
                // Si toma argumentos, es una violación de la regla A.8:
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

// ==============================================================================
// MÉTODO: build_feature_tables (Pasada 1)
// ==============================================================================
// Recolecta las firmas de métodos y declaraciones de atributos en 'class_methods'
// y 'class_attributes' para todas las clases del programa.
// Valida duplicados internos, nombres restringidos y validez de tipos.
// ==============================================================================
void ClassTable::build_feature_tables() {
    // 1. Procesar clases básicas para tener sus métodos estándar registrados
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

    // 2. Procesar clases de usuario y verificar validez sintáctica de cada feature
    for (size_t idx = 0; idx < user_classes.size(); ++idx) {
        Class_ c = user_classes[idx];
        Symbol c_name = c->get_name();
        Features f = c->get_features();

        for (int i = f->first(); f->more(i); i = f->next(i)) {
            Feature feat = f->nth(i);
            if (feat->is_method()) {
                method_class *m = dynamic_cast<method_class*>(feat);
                Symbol m_name = m->get_name();

                // Regla C.1: Método definido más de una vez en la misma clase.
                if (class_methods[c_name].count(m_name) > 0) {
                    semant_error(c->get_filename(), m) << "Method " << m_name << " is multiply defined.\n";
                    continue;
                }

                std::set<Symbol> seen_formals;
                std::vector<Symbol> f_types;
                std::vector<Symbol> f_names;
                Formals formals = m->get_formals();

                for (int j = formals->first(); formals->more(j); j = formals->next(j)) {
                    Formal form = formals->nth(j);
                    Symbol f_name = form->get_name();
                    Symbol f_type = form->get_type_decl();

                    // Regla C.3: Parámetro formal no puede llamarse 'self'.
                    if (f_name == self) {
                        semant_error(c->get_filename(), form) << "'self' cannot be the name of a formal parameter.\n";
                    }

                    // Regla C.2: Parámetros formales con nombres repetidos en el mismo método.
                    if (seen_formals.count(f_name) > 0) {
                        semant_error(c->get_filename(), form) << "Formal parameter " << f_name << " is multiply defined.\n";
                    }
                    seen_formals.insert(f_name);

                    // Regla C.4: SELF_TYPE no está permitido como tipo de parámetro formal.
                    if (f_type == SELF_TYPE) {
                        semant_error(c->get_filename(), form) << "Formal parameter " << f_name << " cannot have type SELF_TYPE.\n";
                    } else if (!is_valid_type(f_type, false)) {
                        // Regla C.5: Tipo de parámetro formal que no existe.
                        semant_error(c->get_filename(), form) << "Type " << f_type << " of parameter " << f_name << " is undefined.\n";
                    }

                    f_types.push_back(f_type);
                    f_names.push_back(f_name);
                }

                // Regla C.5: Tipo de retorno debe ser una clase existente o SELF_TYPE.
                Symbol ret_type = m->get_return_type();
                if (!is_valid_type(ret_type, true)) {
                    semant_error(c->get_filename(), m) << "Undefined return type " << ret_type << " in method " << m_name << ".\n";
                }

                class_methods[c_name][m_name] = MethodInfo(m_name, f_types, f_names, ret_type, c);

            } else if (feat->is_attr()) {
                attr_class *a = dynamic_cast<attr_class*>(feat);
                Symbol a_name = a->get_name();
                Symbol a_type = a->get_type_decl();

                // Regla B.1: Prohibido tener un atributo llamado 'self'.
                if (a_name == self) {
                    semant_error(c->get_filename(), a) << "'self' cannot be the name of an attribute.\n";
                    continue;
                }

                // Regla B.2: Atributo definido más de una vez en la misma clase.
                if (class_attributes[c_name].count(a_name) > 0) {
                    semant_error(c->get_filename(), a) << "Attribute " << a_name << " is multiply defined.\n";
                    continue;
                }

                // Regla B.4: Tipo del atributo debe existir en name_to_class (o ser SELF_TYPE).
                if (!is_valid_type(a_type, true)) {
                    semant_error(c->get_filename(), a) << "Type " << a_type << " of attribute " << a_name << " is undefined.\n";
                }

                class_attributes[c_name][a_name] = AttrInfo(a_name, a_type, a->get_init(), c);
            }
        }
    }
}

// ==============================================================================
// MÉTODO: check_overrides
// ==============================================================================
// Verifica las reglas semánticas de sobreescritura con respecto a ancestros:
// 1. Los atributos NO pueden ser sobreescritos (Regla B.3).
// 2. Los métodos que se sobreescriben deben mantener INVARIANZA ESTRICTA de firma:
//    - Mismo número de parámetros formales.
//    - Mismo tipo en cada posición de parámetro.
//    - Mismo tipo de retorno exacto (Regla C.7).
// ==============================================================================
void ClassTable::check_overrides() {
    for (size_t idx = 0; idx < user_classes.size(); ++idx) {
        Class_ c = user_classes[idx];
        Symbol c_name = c->get_name();
        std::vector<Symbol> ancestors = get_ancestors(c_name);

        // 1. Verificar prohibición de sobreescritura de atributos
        for (std::map<Symbol, AttrInfo>::iterator it = class_attributes[c_name].begin();
             it != class_attributes[c_name].end(); ++it) {
            Symbol a_name = it->first;
            // Buscar si 'a_name' ya existía en alguno de los ancestros superiores
            for (size_t a_idx = 1; a_idx < ancestors.size(); ++a_idx) {
                Symbol anc = ancestors[a_idx];
                if (class_attributes[anc].count(a_name) > 0) {
                    semant_error(c->get_filename(), it->second.defining_class)
                        << "Attribute " << a_name << " is an attribute of an inherited class.\n";
                    break;
                }
            }
        }

        // 2. Verificar invariancia estricta de métodos sobreescritos
        for (std::map<Symbol, MethodInfo>::iterator it = class_methods[c_name].begin();
             it != class_methods[c_name].end(); ++it) {
            Symbol m_name = it->first;
            MethodInfo &sub_m = it->second;

            for (size_t a_idx = 1; a_idx < ancestors.size(); ++a_idx) {
                Symbol anc = ancestors[a_idx];
                if (class_methods[anc].count(m_name) > 0) {
                    MethodInfo &anc_m = class_methods[anc][m_name];

                    // Verificar cantidad idéntica de parámetros
                    if (sub_m.formals_types.size() != anc_m.formals_types.size()) {
                        semant_error(c->get_filename(), sub_m.defining_class)
                            << "Incompatible number of formal parameters in redefined method " << m_name << ".\n";
                        break;
                    }

                    // Verificar tipo idéntico para cada parámetro
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

                    // Verificar tipo de retorno idéntico
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

// ==============================================================================
// MÉTODO: check_types (Pasada 2)
// ==============================================================================
// Para cada clase de usuario:
// 1. Construye el ambiente de objetos (O) insertando 'self : SELF_TYPE' y todos los
//    atributos heredados y propios en orden top-down (de Object hacia la clase).
// 2. Comprueba las expresiones de inicialización de atributos (Regla B.5).
// 3. Comprueba los cuerpos de los métodos y su conformidad con el retorno (Regla C.6).
// ==============================================================================
void ClassTable::check_types() {
    for (size_t idx = 0; idx < user_classes.size(); ++idx) {
        Class_ c = user_classes[idx];
        Symbol c_name = c->get_name();

        // Construir el ambiente de objetos (O)
        SymbolTable<Symbol, Entry> object_env;
        object_env.enterscope();

        // Invariante de COOL: 'self' siempre está ligado a SELF_TYPE en O
        object_env.addid(self, SELF_TYPE);

        // Poblar atributos de ancestros desde la raíz (Object) hasta la clase actual:
        std::vector<Symbol> ancestors = get_ancestors(c_name);
        for (int a_idx = (int)ancestors.size() - 1; a_idx >= 0; --a_idx) {
            Symbol anc = ancestors[a_idx];
            for (std::map<Symbol, AttrInfo>::iterator it = class_attributes[anc].begin();
                 it != class_attributes[anc].end(); ++it) {
                object_env.addid(it->second.name, it->second.type_decl);
            }
        }

        // 1. Comprobar inicializaciones de atributos propios
        Features f = c->get_features();
        for (int i = f->first(); f->more(i); i = f->next(i)) {
            Feature feat = f->nth(i);
            if (feat->is_attr()) {
                attr_class *a = dynamic_cast<attr_class*>(feat);
                Expression init = a->get_init();
                Symbol init_type = init->type_check(this, c, object_env);

                // Si tiene inicialización explícita, debe conformar: init_type <= declared_type
                if (init_type != No_type) {
                    if (!conforms(init_type, a->get_type_decl(), c)) {
                        semant_error(c->get_filename(), a)
                            << "Inferred type " << init_type << " of initialization of attribute "
                            << a->get_name() << " does not conform to declared type " << a->get_type_decl() << ".\n";
                    }
                }
            }
        }

        // 2. Comprobar cuerpos de métodos
        for (int i = f->first(); f->more(i); i = f->next(i)) {
            Feature feat = f->nth(i);
            if (feat->is_method()) {
                method_class *m = dynamic_cast<method_class*>(feat);
                object_env.enterscope(); // Nuevo scope para los parámetros formales

                Formals formals = m->get_formals();
                for (int j = formals->first(); formals->more(j); j = formals->next(j)) {
                    Formal form = formals->nth(j);
                    object_env.addid(form->get_name(), form->get_type_decl());
                }

                // Evaluar recursivamente el tipo del cuerpo del método:
                Symbol body_type = m->get_expr()->type_check(this, c, object_env);
                Symbol ret_type = m->get_return_type();

                // Regla especial [Method]: Si el retorno declarado es SELF_TYPE,
                // el cuerpo DEBE evaluarse estrictamente a SELF_TYPE.
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

// ==============================================================================
// FUNCIÓN: conforms (Subtipado: T1 <= T2)
// ==============================================================================
// Define la relación de conformidad en COOL:
// 1. No_type conforma con todo (permite atributos sin inicializar).
// 2. Reflexividad: T <= T.
// 3. T1 <= SELF_TYPE solo si T1 == SELF_TYPE.
// 4. SELF_TYPE <= T2 <=> current_class <= T2.
// 5. Clases normales: T1 <= T2 si T2 es ancestro de T1 en el árbol de herencia.
// ==============================================================================
bool ClassTable::conforms(Symbol t1, Symbol t2, Class_ current_class) {
    if (t1 == No_type) return true;
    if (t1 == t2) return true;
    if (t2 == SELF_TYPE) return false;

    if (t1 == SELF_TYPE) {
        return conforms(current_class->get_name(), t2, current_class);
    }

    Symbol curr = t1;
    while (curr != No_class && curr != NULL && name_to_class.count(curr) > 0) {
        if (curr == t2) return true;
        if (curr == Object) break;
        curr = parent_map[curr];
    }

    return false;
}

// ==============================================================================
// FUNCIÓN: lub (Least Upper Bound / Mínimo Ancestro Común)
// ==============================================================================
// Calcula el tipo común más específico que cubre tanto a T1 como a T2:
// - lub(T, T) = T
// - lub(SELF_TYPE, SELF_TYPE) = SELF_TYPE
// - lub(SELF_TYPE, T2) = lub(current_class, T2)
// - Para tipos normales: primer ancestro compartido al subir hacia Object.
// ==============================================================================
Symbol ClassTable::lub(Symbol t1, Symbol t2, Class_ current_class) {
    if (t1 == t2) return t1;
    if (t1 == SELF_TYPE && t2 == SELF_TYPE) return SELF_TYPE;

    if (t1 == SELF_TYPE) return lub(current_class->get_name(), t2, current_class);
    if (t2 == SELF_TYPE) return lub(t1, current_class->get_name(), current_class);

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
// Notificación de Errores Semánticos Formateados
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
// Cada clase de Expression implementa el método virtual 'type_check',
// el cual computa el tipo estático según las reglas formales O, M, C |- e : T,
// anota el nodo con set_type(T), y retorna T.
//
//////////////////////////////////////////////////////////////////////

// ==============================================================================
// REGLA FORMAL [Assign]: O, M, C |- id <- e : T'
// O(id) = T, O, M, C |- e : T', T' <= T, id != self
// ==============================================================================
Symbol assign_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    // Error D.2: Prohibido asignar a 'self'
    if (name == self) {
        ct->semant_error(cur_class->get_filename(), this) << "Cannot assign to 'self'.\n";
        set_type(Object);
        return Object;
    }

    // Error D.1: Variable no declarada en el ambiente O
    Symbol var_type = o_table.lookup(name);
    if (var_type == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "Assignment to undeclared variable " << name << ".\n";
        var_type = Object; // Recuperación con Object
    }

    Symbol expr_type = expr->type_check(ct, cur_class, o_table);

    // Error E.1: El tipo de la expresión asignada debe conformar con el declarado
    if (var_type != Object && !ct->conforms(expr_type, var_type, cur_class)) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Type " << expr_type << " of assigned expression does not conform to declared type "
            << var_type << " of identifier " << name << ".\n";
    }

    set_type(expr_type);
    return expr_type;
}

// ==============================================================================
// REGLA FORMAL [StaticDispatch]: O, M, C |- e0@T.m(e1, ..., en) : Tr'
// e0 : T0, T0 <= T, T != SELF_TYPE, M(T, m) = (T1, ..., Tn, Tr)
// ==============================================================================
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

    // Expresión receptora debe conformar con la clase estática especificada (@T)
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

    // Verificar número de argumentos
    if (actual->len() != (int)m->formals_types.size()) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Method " << name << " called with wrong number of arguments.\n";
    }

    // Verificar conformidad de cada argumento real contra el parámetro formal
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

    // Si el método declara retornar SELF_TYPE, en despacho estático se especializa al tipo del receptor t0
    Symbol result_type = m->return_type;
    if (result_type == SELF_TYPE) {
        result_type = t0;
    }

    set_type(result_type);
    return result_type;
}

// ==============================================================================
// REGLA FORMAL [Dispatch]: O, M, C |- e0.m(e1, ..., en) : Tr'
// e0 : T0, C' = (T0 == SELF_TYPE ? C : T0), M(C', m) = (T1, ..., Tn, Tr)
// ==============================================================================
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

    // Regla de retorno de SELF_TYPE: el despacho evalúa al tipo dinámico del receptor (t0)
    Symbol result_type = m->return_type;
    if (result_type == SELF_TYPE) {
        result_type = t0;
    }

    set_type(result_type);
    return result_type;
}

// ==============================================================================
// REGLA FORMAL [If]: O, M, C |- if pred then then_exp else else_exp fi : lub(T1, T2)
// pred : Bool, then_exp : T1, else_exp : T2
// ==============================================================================
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

// ==============================================================================
// REGLA FORMAL [Loop]: O, M, C |- while pred loop body pool : Object
// pred : Bool. El tipo del bucle es SIEMPRE Object en COOL.
// ==============================================================================
Symbol loop_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol pt = pred->type_check(ct, cur_class, o_table);
    if (pt != Bool) {
        ct->semant_error(cur_class->get_filename(), this) << "Loop condition does not have type Bool.\n";
    }

    body->type_check(ct, cur_class, o_table);
    set_type(Object);
    return Object;
}

// ==============================================================================
// REGLA FORMAL [Case]: O, M, C |- case e0 of id1:T1 => e1; ... idn:Tn => en esac : lub(Tk)
// Ti no repetidos, Ti != SELF_TYPE, Ti != No_class, idi != self
// ==============================================================================
Symbol typcase_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    expr->type_check(ct, cur_class, o_table);

    std::set<Symbol> seen_types;
    std::vector<Symbol> branch_results;

    for (int k = cases->first(); cases->more(k); k = cases->next(k)) {
        Case br = cases->nth(k);
        Symbol br_name = br->get_name();
        Symbol br_type = br->get_type_decl();

        // Error D.3: 'self' ligado como variable en case
        if (br_name == self) {
            ct->semant_error(cur_class->get_filename(), br) << "'self' bound in 'case'.\n";
        }

        // Error E.9: SELF_TYPE en rama de case
        if (br_type == SELF_TYPE) {
            ct->semant_error(cur_class->get_filename(), br)
                << "Identifier " << br_name << " declared with type SELF_TYPE in case branch.\n";
        } else if (ct->get_class(br_type) == NULL) {
            // Error E.9: Tipo de rama inexistente
            ct->semant_error(cur_class->get_filename(), br)
                << "Class " << br_type << " of case branch is undefined.\n";
        }

        // Error E.9: Ramas duplicadas con el mismo tipo
        if (seen_types.count(br_type) > 0) {
            ct->semant_error(cur_class->get_filename(), br)
                << "Duplicate branch " << br_type << " in case statement.\n";
        }
        seen_types.insert(br_type);

        // Cada rama introduce una nueva variable con su propio alcance:
        o_table.enterscope();
        o_table.addid(br_name, br_type);
        Symbol b_res = br->get_expr()->type_check(ct, cur_class, o_table);
        o_table.exitscope();

        branch_results.push_back(b_res);
    }

    // El tipo resultante del case es el Least Upper Bound (LUB) de todas las ramas
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

// ==============================================================================
// REGLA FORMAL [Block]: O, M, C |- { e1; e2; ...; en } : Tn
// El tipo estático de un bloque es el tipo de su ÚLTIMA expresión.
// ==============================================================================
Symbol block_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol last_t = Object;
    for (int k = body->first(); body->more(k); k = body->next(k)) {
        last_t = body->nth(k)->type_check(ct, cur_class, o_table);
    }
    set_type(last_t);
    return last_t;
}

// ==============================================================================
// REGLA FORMAL [Let]: O, M, C |- let id:T <- init in body : T_body
// id != self. 'init' se tipa en el scope exterior; 'body' en el scope con id:T.
// ==============================================================================
Symbol let_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    if (identifier == self) {
        ct->semant_error(cur_class->get_filename(), this) << "'self' cannot be bound in a 'let' expression.\n";
    }

    if (type_decl != SELF_TYPE && ct->get_class(type_decl) == NULL) {
        ct->semant_error(cur_class->get_filename(), this)
            << "Type " << type_decl << " of let-bound identifier " << identifier << " is undefined.\n";
    }

    // La inicialización se evalúa en el ambiente donde 'identifier' AÚN NO existe
    Symbol init_t = init->type_check(ct, cur_class, o_table);

    if (init_t != No_type) {
        if (!ct->conforms(init_t, type_decl, cur_class)) {
            ct->semant_error(cur_class->get_filename(), this)
                << "Inferred type " << init_t << " of initialization of " << identifier
                << " does not conform to identifier's declared type " << type_decl << ".\n";
        }
    }

    // El cuerpo se evalúa con 'identifier' en el nuevo scope
    o_table.enterscope();
    o_table.addid(identifier, type_decl);
    Symbol body_t = body->type_check(ct, cur_class, o_table);
    o_table.exitscope();

    set_type(body_t);
    return body_t;
}

// ==============================================================================
// REGLAS ARITMÉTICAS: Plus (+), Sub (-), Mul (*), Divide (/)
// Exigen estrictamente Int en ambos operandos y retornan Int.
// ==============================================================================
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

// Negación entera (~)
Symbol neg_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    if (t1 != Int) {
        ct->semant_error(cur_class->get_filename(), this) << "Argument of '~' has type " << t1 << " instead of Int.\n";
    }
    set_type(Int);
    return Int;
}

// Comparaciones (<, <=): Exigen Int y retornan Bool
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

// ==============================================================================
// REGLA FORMAL [Equal] (=):
// Si cualquiera de los dos lados es Int, Bool o String, ambos DEBEN ser del
// MISMO tipo básico primitivo. Los demás tipos comparan identidad de punteros.
// ==============================================================================
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

// Complemento booleano (not): Exige Bool y retorna Bool
Symbol comp_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    Symbol t1 = e1->type_check(ct, cur_class, o_table);
    if (t1 != Bool) {
        ct->semant_error(cur_class->get_filename(), this) << "Argument of 'not' has type " << t1 << " instead of Bool.\n";
    }
    set_type(Bool);
    return Bool;
}

// Literales y Constantes:
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

// Operador 'new':
Symbol new__class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    if (type_name != SELF_TYPE && ct->get_class(type_name) == NULL) {
        ct->semant_error(cur_class->get_filename(), this) << "'new' used with undefined class " << type_name << ".\n";
        set_type(Object);
        return Object;
    }
    set_type(type_name);
    return type_name;
}

// Operador 'isvoid': Siempre retorna Bool
Symbol isvoid_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    e1->type_check(ct, cur_class, o_table);
    set_type(Bool);
    return Bool;
}

// Inicializador vacío (no_expr): Centinela No_type
Symbol no_expr_class::type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) {
    set_type(No_type);
    return No_type;
}

// Identificador (object_class):
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
