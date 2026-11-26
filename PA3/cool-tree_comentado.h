#ifndef COOL_TREE_H
#define COOL_TREE_H

// ==============================================================================
// ARCHIVO: cool-tree_comentado.h
// PROYECTO: Laboratorio 03 - Analizador Semántico para COOL (PA4)
// ASIGNATURA: Compiladores, Universidad Nacional de Colombia (2026-II)
// ==============================================================================
// ¿POR QUÉ EXISTE ESTE ARCHIVO Y QUÉ ES?
// En la arquitectura de Stanford CS143, 'cool-tree.h' es generado automáticamente
// por la herramienta 'aps' (Abstract Production System). Define la jerarquía
// completa de clases en C++ que representan el Árbol de Sintaxis Abstracta (AST)
// del compilador de COOL.
//
// Los estudiantes tradicionalmente NO modifican 'cool-tree.h' directamente, sino
// que colocan sus extensiones en 'cool-tree.handcode.h', las cuales son inyectadas
// en este archivo mediante macros.
//
// Esta versión comentada explica la estructura del AST: qué es un Phylum, qué es
// un Constructor, cómo se representan las listas, y qué campos almacena cada
// uno de los 24 nodos de expresión.
// ==============================================================================

#include <iostream>
#include "tree.h"                  // Define la clase base 'tree_node' y las listas 'list_node'.
#include "cool.h"                  // Tipos primitivos y booleanos de COOL.
#include "stringtab.h"             // Punteros a la tabla de símbolos (Symbol).
#include "cool-tree.handcode.h"    // Extensiones manuales inyectadas (type, set_type, type_check).

// ==============================================================================
// 1. DECLARACIONES DE PHYLA Y PUNTEROS (Typedefs)
// ==============================================================================
// En el diseño de Stanford CS143, un "Phylum" (en plural "Phyla") es una categoría
// sintáctica abstracta de la gramática (ej. Expression, Feature, Class_).
// Por convención de CS143:
// - La clase abstracta termina en '_class' (ej. Expression_class).
// - El puntero a esa clase se nombra sin el sufijo (ej. Expression = Expression_class*).

class Program_class;
typedef Program_class *Program;          // Puntero a nodo raíz del programa.

class Class__class;
typedef Class__class *Class_;            // Puntero a nodo de declaración de clase (Class_ lleva guión bajo para no chocar con la palabra reservada 'class' de C++).

class Feature_class;
typedef Feature_class *Feature;          // Puntero a miembro de clase (método o atributo).

class Formal_class;
typedef Formal_class *Formal;            // Puntero a parámetro formal de método.

class Expression_class;
typedef Expression_class *Expression;    // Puntero a cualquier expresión de COOL.

class Case_class;
typedef Case_class *Case;                // Puntero a una rama individual de una sentencia 'case'.

// ==============================================================================
// 2. LISTAS DEL AST (Plantillas list_node<T>)
// ==============================================================================
// CS143 utiliza la plantilla 'list_node<T>' (definida en tree.h) para representar
// secuencias o listas de nodos sintácticos en el AST.

typedef list_node<Class_> Classes_class;
typedef Classes_class *Classes;          // Lista de clases que componen el programa COOL.

typedef list_node<Feature> Features_class;
typedef Features_class *Features;        // Lista de métodos y atributos de una clase.

typedef list_node<Formal> Formals_class;
typedef Formals_class *Formals;          // Lista de parámetros formales de un método.

typedef list_node<Expression> Expressions_class;
typedef Expressions_class *Expressions;  // Lista de expresiones (ej. argumentos en un dispatch o expresiones en un bloque { ... }).

typedef list_node<Case> Cases_class;
typedef Cases_class *Cases;              // Lista de ramas de una sentencia 'case ... esac'.

// ==============================================================================
// 3. NODO RAÍZ: Program
// ==============================================================================
// Program_class: Clase abstracta que define la interfaz.
class Program_class : public tree_node {
public:
   tree_node *copy() { return copy_Program(); }
   virtual Program copy_Program() = 0;
   Program_EXTRAS                        // Inyecta: virtual void semant() = 0;
};

// program_class: Nodo concreto instanciado por el parser.
class program_class : public Program_class {
protected:
   Classes classes;                      // Lista con todas las clases declaradas en el programa.
public:
   program_class(Classes a1) {
      classes = a1;
   }
   Program copy_Program();
   void dump(ostream& stream, int n);
   Classes get_classes() { return classes; }
   program_EXTRAS                        // Inyecta: void semant();
};

// ==============================================================================
// 4. DECLARACIÓN DE CLASE: Class_
// ==============================================================================
// Representa: class <name> inherits <parent> { <features> } en <filename>
class Class__class : public tree_node {
public:
   tree_node *copy() { return copy_Class_(); }
   virtual Class_ copy_Class_() = 0;
   Class__EXTRAS                         // Inyecta firmas virtuales de accesores (get_name, etc.)
};

class class__class : public Class__class {
protected:
   Symbol name;                          // Nombre de la clase (ej. "Dog").
   Symbol parent;                        // Nombre de la clase padre (ej. "Mammal" o "Object" por defecto).
   Features features;                    // Lista con todos los métodos y atributos de la clase.
   Symbol filename;                      // Nombre del archivo fuente donde se definió.
public:
   class__class(Symbol a1, Symbol a2, Features a3, Symbol a4) {
      name = a1;
      parent = a2;
      features = a3;
      filename = a4;
   }
   Class_ copy_Class_();
   void dump(ostream& stream, int n);
   class__EXTRAS                         // Inyecta implementaciones de accesores (get_name, get_parent, etc.)
};

// ==============================================================================
// 5. CARACTERÍSTICAS DE CLASE: Feature (method_class y attr_class)
// ==============================================================================
// Feature_class es la base común para métodos y atributos.
class Feature_class : public tree_node {
public:
   tree_node *copy() { return copy_Feature(); }
   virtual Feature copy_Feature() = 0;
   Feature_EXTRAS                        // Inyecta is_method() e is_attr() retornando false por defecto.
};

// method_class: Representa 'name(formals) : return_type { expr }'
class method_class : public Feature_class {
protected:
   Symbol name;                          // Nombre del método (ej. "speak").
   Formals formals;                      // Lista de parámetros formales (ej. x: Int, y: Bool).
   Symbol return_type;                   // Tipo de retorno declarado (ej. "Int", "SELF_TYPE").
   Expression expr;                      // Cuerpo del método (expresión a evaluar).
public:
   method_class(Symbol a1, Formals a2, Symbol a3, Expression a4) {
      name = a1;
      formals = a2;
      return_type = a3;
      expr = a4;
   }
   Feature copy_Feature();
   void dump(ostream& stream, int n);
   bool is_method() { return true; }     // Identifica polimórficamente que es un método.
   bool is_attr() { return false; }
   Symbol get_name() { return name; }
   Formals get_formals() { return formals; }
   Symbol get_return_type() { return return_type; }
   Expression get_expr() { return expr; }
   Feature_SHARED_EXTRAS
};

// attr_class: Representa 'name : type_decl <- init'
class attr_class : public Feature_class {
protected:
   Symbol name;                          // Nombre del atributo (ej. "age").
   Symbol type_decl;                     // Tipo declarado (ej. "Int").
   Expression init;                      // Expresión de inicialización (puede ser no_expr si no se inicializa).
public:
   attr_class(Symbol a1, Symbol a2, Expression a3) {
      name = a1;
      type_decl = a2;
      init = a3;
   }
   Feature copy_Feature();
   void dump(ostream& stream, int n);
   bool is_method() { return false; }
   bool is_attr() { return true; }       // Identifica polimórficamente que es un atributo.
   Symbol get_name() { return name; }
   Symbol get_type_decl() { return type_decl; }
   Expression get_init() { return init; }
   Feature_SHARED_EXTRAS
};

// ==============================================================================
// 6. PARÁMETROS FORMALES: Formal
// ==============================================================================
// Representa 'name : type_decl' dentro de la lista de parámetros de un método.
class Formal_class : public tree_node {
public:
   tree_node *copy() { return copy_Formal(); }
   virtual Formal copy_Formal() = 0;
   Formal_EXTRAS
};

class formal_class : public Formal_class {
protected:
   Symbol name;                          // Nombre del parámetro formal (ej. "x").
   Symbol type_decl;                     // Tipo declarado del parámetro (ej. "Int").
public:
   formal_class(Symbol a1, Symbol a2) {
      name = a1;
      type_decl = a2;
   }
   Formal copy_Formal();
   void dump(ostream& stream, int n);
   formal_EXTRAS
};

// ==============================================================================
// 7. RAMA DE UN CASE: Case / branch_class
// ==============================================================================
// Representa 'name : type_decl => expr' en una sentencia 'case ... esac'.
class Case_class : public tree_node {
public:
   tree_node *copy() { return copy_Case(); }
   virtual Case copy_Case() = 0;
   Case_EXTRAS
};

class branch_class : public Case_class {
protected:
   Symbol name;                          // Identificador ligado en la rama (ej. "d").
   Symbol type_decl;                     // Tipo de la clase emparejada (ej. "Dog").
   Expression expr;                      // Expresión a evaluar si se selecciona esta rama.
public:
   branch_class(Symbol a1, Symbol a2, Expression a3) {
      name = a1;
      type_decl = a2;
      expr = a3;
   }
   Case copy_Case();
   void dump(ostream& stream, int n);
   branch_EXTRAS
};

// ==============================================================================
// 8. CLASE BASE DE TODAS LAS EXPRESIONES: Expression_class
// ==============================================================================
class Expression_class : public tree_node {
public:
   tree_node *copy() { return copy_Expression(); }
   virtual Expression copy_Expression() = 0;
   Expression_SHARED_EXTRAS              // Inyecta: type, get_type(), set_type(), virtual type_check(...) = 0;
};

// ==============================================================================
// 9. LAS 24 CLASES DERIVADAS DE EXPRESIÓN DE COOL
// ==============================================================================

// 1. assign_class: 'name <- expr' (Asignación)
class assign_class : public Expression_class {
protected:
   Symbol name;                          // Variable de destino.
   Expression expr;                      // Expresión asignada.
public:
   assign_class(Symbol a1, Expression a2) { name = a1; expr = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS                     // Inyecta: Symbol type_check(...);
};

// 2. static_dispatch_class: 'expr@type_name.name(actual)' (Despacho estático)
class static_dispatch_class : public Expression_class {
protected:
   Expression expr;                      // Objeto receptor.
   Symbol type_name;                     // Clase estática explícita tras el '@'.
   Symbol name;                          // Nombre del método.
   Expressions actual;                   // Argumentos reales.
public:
   static_dispatch_class(Expression a1, Symbol a2, Symbol a3, Expressions a4) {
      expr = a1; type_name = a2; name = a3; actual = a4;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 3. dispatch_class: 'expr.name(actual)' (Despacho dinámico)
class dispatch_class : public Expression_class {
protected:
   Expression expr;                      // Objeto receptor.
   Symbol name;                          // Nombre del método invocado.
   Expressions actual;                   // Argumentos reales pasados al método.
public:
   dispatch_class(Expression a1, Symbol a2, Expressions a3) {
      expr = a1; name = a2; actual = a3;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 4. cond_class: 'if pred then then_exp else else_exp fi' (Condicional)
class cond_class : public Expression_class {
protected:
   Expression pred;                      // Predicado (debe evaluarse a Bool).
   Expression then_exp;                  // Rama 'then'.
   Expression else_exp;                  // Rama 'else'.
public:
   cond_class(Expression a1, Expression a2, Expression a3) {
      pred = a1; then_exp = a2; else_exp = a3;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 5. loop_class: 'while pred loop body pool' (Bucle)
class loop_class : public Expression_class {
protected:
   Expression pred;                      // Condición del bucle (debe ser Bool).
   Expression body;                      // Cuerpo repetido. Siempre retorna Object.
public:
   loop_class(Expression a1, Expression a2) { pred = a1; body = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 6. typcase_class: 'case expr of cases esac' (Selección por tipo dinámico)
class typcase_class : public Expression_class {
protected:
   Expression expr;                      // Expresión a evaluar e inspeccionar tipo.
   Cases cases;                          // Lista de ramas disyuntas.
public:
   typcase_class(Expression a1, Cases a2) { expr = a1; cases = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 7. block_class: '{ body }' (Bloque de expresiones separadas por ;)
class block_class : public Expression_class {
protected:
   Expressions body;                     // Lista de expresiones. Se evalúa al tipo de la última.
public:
   block_class(Expressions a1) { body = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 8. let_class: 'let identifier : type_decl <- init in body' (Ámbito local)
class let_class : public Expression_class {
protected:
   Symbol identifier;                    // Nombre de la nueva variable local.
   Symbol type_decl;                     // Tipo declarado de la variable.
   Expression init;                      // Expresión de inicialización opcional.
   Expression body;                      // Cuerpo donde la variable es visible.
public:
   let_class(Symbol a1, Symbol a2, Expression a3, Expression a4) {
      identifier = a1; type_decl = a2; init = a3; body = a4;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 9-12. Operaciones aritméticas binarias (exigen Int y retornan Int):
class plus_class : public Expression_class {
protected:
   Expression e1, e2;
public:
   plus_class(Expression a1, Expression a2) { e1 = a1; e2 = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

class sub_class : public Expression_class {
protected:
   Expression e1, e2;
public:
   sub_class(Expression a1, Expression a2) { e1 = a1; e2 = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

class mul_class : public Expression_class {
protected:
   Expression e1, e2;
public:
   mul_class(Expression a1, Expression a2) { e1 = a1; e2 = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

class divide_class : public Expression_class {
protected:
   Expression e1, e2;
public:
   divide_class(Expression a1, Expression a2) { e1 = a1; e2 = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 13. neg_class: '~e1' (Negación entera aritmética, complementa a dos)
class neg_class : public Expression_class {
protected:
   Expression e1;
public:
   neg_class(Expression a1) { e1 = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 14-16. Comparaciones relacionales (retornan Bool):
class lt_class : public Expression_class {
protected:
   Expression e1, e2;                    // 'e1 < e2' (exige Int en ambos).
public:
   lt_class(Expression a1, Expression a2) { e1 = a1; e2 = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

class eq_class : public Expression_class {
protected:
   Expression e1, e2;                    // 'e1 = e2' (regla especial de tipos básicos).
public:
   eq_class(Expression a1, Expression a2) { e1 = a1; e2 = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

class leq_class : public Expression_class {
protected:
   Expression e1, e2;                    // 'e1 <= e2' (exige Int en ambos).
public:
   leq_class(Expression a1, Expression a2) { e1 = a1; e2 = a2; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 17. comp_class: 'not e1' (Negación lógica booleana)
class comp_class : public Expression_class {
protected:
   Expression e1;                        // Exige Bool y retorna Bool.
public:
   comp_class(Expression a1) { e1 = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 18-20. Literales y constantes primitivas:
class int_const_class : public Expression_class {
protected:
   Symbol token;                         // Representación textual del número entero.
public:
   int_const_class(Symbol a1) { token = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

class bool_const_class : public Expression_class {
protected:
   Boolean val;                          // Valor booleano de C++ (true / false).
public:
   bool_const_class(Boolean a1) { val = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

class string_const_class : public Expression_class {
protected:
   Symbol token;                         // Cadena literal en la tabla de símbolos.
public:
   string_const_class(Symbol a1) { token = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 21. new__class: 'new type_name' (Instanciación de objetos)
class new__class : public Expression_class {
protected:
   Symbol type_name;                     // Tipo a instanciar (puede ser una clase o SELF_TYPE).
public:
   new__class(Symbol a1) { type_name = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 22. isvoid_class: 'isvoid e1' (Prueba si el puntero es nulo)
class isvoid_class : public Expression_class {
protected:
   Expression e1;                        // Retorna siempre Bool.
public:
   isvoid_class(Expression a1) { e1 = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// 23. no_expr_class: Nodo vacío para atributos o variables sin inicialización
class no_expr_class : public Expression_class {
public:
   no_expr_class() {}
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS                     // Retorna el tipo centinela No_type.
};

// 24. object_class: Identificador de variable o 'self'
class object_class : public Expression_class {
protected:
   Symbol name;                          // Nombre del identificador (busca en O, o SELF_TYPE si es 'self').
public:
   object_class(Symbol a1) { name = a1; }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   Expression_EXTRAS
};

// ==============================================================================
// 10. FUNCIONES CONSTRUCTORAS AUXILIARES DEL PARSER
// ==============================================================================
// Estas funciones globales son llamadas por el analizador sintáctico (Bison)
// para construir los nodos concretos del AST durante la reducción de reglas.

Program program(Classes);
Class_ class_(Symbol, Symbol, Features, Symbol);
Feature method(Symbol, Formals, Symbol, Expression);
Feature attr(Symbol, Symbol, Expression);
Formal formal(Symbol, Symbol);
Case branch(Symbol, Symbol, Expression);
Expression assign(Symbol, Expression);
Expression static_dispatch(Expression, Symbol, Symbol, Expressions);
Expression dispatch(Expression, Symbol, Expressions);
Expression cond(Expression, Expression, Expression);
Expression loop(Expression, Expression);
Expression typcase(Expression, Cases);
Expression block(Expressions);
Expression let(Symbol, Symbol, Expression, Expression);
Expression plus(Expression, Expression);
Expression sub(Expression, Expression);
Expression mul(Expression, Expression);
Expression divide(Expression, Expression);
Expression neg(Expression);
Expression lt(Expression, Expression);
Expression eq(Expression, Expression);
Expression leq(Expression, Expression);
Expression comp(Expression);
Expression int_const(Symbol);
Expression bool_const(Boolean);
Expression string_const(Symbol);
Expression new_(Symbol);
Expression isvoid(Expression);
Expression no_expr();
Expression object(Symbol);

#endif // COOL_TREE_H
