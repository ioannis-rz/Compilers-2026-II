#ifndef COOL_TREE_HANDCODE_H
#define COOL_TREE_HANDCODE_H

#include <iostream>
#include "tree.h"
#include "cool.h"
#include "stringtab.h"
#include "symtab.h"

#define yylineno_the_ yy_g_yylineno
extern int yylineno;

inline Boolean copy_Boolean(Boolean b) { return b; }
inline void assert_Boolean(Boolean) {}
inline void dump_Boolean(ostream& stream, int padding, Boolean b) {
    stream << pad(padding) << (b ? "true" : "false") << "\n";
}

void dump_Symbol(ostream& stream, int padding, Symbol b);
void assert_Symbol(Symbol b);
Symbol copy_Symbol(Symbol b);

class ClassTable;

// Macros de extensión para Program
#define Program_EXTRAS \
virtual void semant() = 0; \
virtual void dump_with_types(ostream&, int) = 0;

#define program_EXTRAS \
void semant(); \
void dump_with_types(ostream&, int);

// Macros de extensión para Class_
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

// Macros de extensión para Feature
#define Feature_EXTRAS \
virtual void dump_with_types(ostream&, int) = 0; \
virtual bool is_method() { return false; } \
virtual bool is_attr() { return false; }

#define Feature_SHARED_EXTRAS \
void dump_with_types(ostream&, int);

// Macros de extensión para Formal
#define Formal_EXTRAS \
virtual void dump_with_types(ostream&, int) = 0; \
virtual Symbol get_name() = 0; \
virtual Symbol get_type_decl() = 0;

#define formal_EXTRAS \
void dump_with_types(ostream&, int); \
Symbol get_name() { return name; } \
Symbol get_type_decl() { return type_decl; }

// Macros de extensión para Case
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

// Macros de extensión para Expression
#define Expression_EXTRAS \
Symbol type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table);

#define Expression_SHARED_EXTRAS \
void dump_with_types(ostream&, int); \
Symbol type; \
Symbol get_type() { return type; } \
Expression set_type(Symbol s) { type = s; return this; } \
virtual Symbol type_check(ClassTable* ct, Class_ cur_class, SymbolTable<Symbol, Entry>& o_table) = 0;

#endif
