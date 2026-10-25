/*
 *  cool.y
 *
 *  Parser de Cool (Classroom Object-Oriented Language)
 *  Laboratorio 02 - Compiladores - Prof. Arles Rodriguez
 *
 *  Implementa un analizador sintactico (parser) completo para Cool,
 *  construyendo el AST (Abstract Syntax Tree) mediante acciones semanticas.
 *
 *  Basado en el curso CS143 de Stanford University - Alex Aiken
 */

%{
  #include <iostream>
  #include "cool-tree.h"
  #include "stringtab.h"
  #include "utilities.h"

  extern char *curr_filename;

  /* Prototipo de la funcion de error del parser */
  void yyerror(char *s);
  int yylex();

  /* El arbol de parse resultante */
  Program ast_root;
  Classes parse_results;

  /* Numero de errores de parser */
  int omerrs = 0;

%}

/* ========================
   Declaraciones de BISON
   ======================== */

/* La union define los tipos posibles de valores semanticos */
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

/* Terminales sin valor semantico */
%token CLASS 258
%token ELSE 259
%token FI 260
%token IF 261
%token IN 262
%token INHERITS 263
%token LET 264
%token LOOP 265
%token POOL 266
%token THEN 267
%token WHILE 268
%token CASE 269
%token ESAC 270
%token OF 271
%token DARROW 272
%token NEW 273
%token ISVOID 274

/* Terminales con valor semantico */
%token <symbol>  STR_CONST 275
%token <symbol>  INT_CONST 276
%token <boolean> BOOL_CONST 277
%token <symbol>  TYPEID 278
%token <symbol>  OBJECTID 279
%token          ASSIGN 280
%token          NOT 281
%token          LE 282
%token          ERROR 283
%token          LET_STMT 285

/* Tipos de los no-terminales */
%type <program>     program
%type <classes>     class_list
%type <class_>      class
%type <features>    feature_list
%type <feature>     feature
%type <formals>     formal_list formal_list_ne
%type <formal>      formal
%type <expression>  expr let_expr
%type <expressions> expr_list actual_list actual_list_ne
%type <cases>       case_list
%type <case_>       case_branch

/* ===============================================
   Declaraciones de precedencia y asociatividad
   (de menor a mayor: primero declarado = menor)
   =============================================== */
%right LET_PREC
%right ASSIGN
%left NOT
%nonassoc LE '<' '='
%left '+' '-'
%left '*' '/'
%left ISVOID
%left '~'
%left '@'
%left '.'

%%

/* ====================================================
   REGLAS GRAMATICALES
   ==================================================== */

/*
 * program ::= [ class ; ]+
 * Un programa es una lista de una o mas clases
 */
program
  : class_list
      {
        @$ = @1;
        ast_root = program($1);
      }
  ;

/*
 * Lista de clases con recuperacion de errores
 */
class_list
  : class ';'
      {
        $$ = single_Classes($1);
        parse_results = $$;
      }
  | class_list class ';'
      {
        $$ = append_Classes($1, single_Classes($2));
        parse_results = $$;
      }
  | error ';'
      {
        $$ = nil_Classes();
        parse_results = $$;
      }
  | class_list error ';'
      {
        $$ = $1;
        parse_results = $$;
      }
  ;

/*
 * class ::= class TYPE [ inherits TYPE ] { [ feature ; ]* }
 */
class
  : CLASS TYPEID '{' feature_list '}'
      {
        /* Sin herencia explicita: hereda de Object */
        $$ = class_($2,
                    idtable.add_string("Object"),
                    $4,
                    stringtable.add_string(curr_filename));
      }
  | CLASS TYPEID INHERITS TYPEID '{' feature_list '}'
      {
        /* Con herencia explicita */
        $$ = class_($2, $4, $6, stringtable.add_string(curr_filename));
      }
  ;

/*
 * feature_list ::= [ feature ; ]*
 * Lista de features (metodos y atributos)
 * Recuperacion de errores: salta al siguiente feature
 */
feature_list
  : /* vacio */
      { $$ = nil_Features(); }
  | feature_list feature ';'
      { $$ = append_Features($1, single_Features($2)); }
  | feature_list error ';'
      { $$ = $1; }
  ;

/*
 * feature ::= ID ( formal_list ) : TYPE { expr }   (metodo)
 *           | ID : TYPE [ <- expr ]                (atributo)
 */
feature
  : OBJECTID '(' formal_list ')' ':' TYPEID '{' expr '}'
      { $$ = method($1, $3, $6, $8); }
  | OBJECTID ':' TYPEID ASSIGN expr
      { $$ = attr($1, $3, $5); }
  | OBJECTID ':' TYPEID
      { $$ = attr($1, $3, no_expr()); }
  ;

/*
 * formal_list ::= [ formal [ , formal ]* ]
 */
formal_list
  : /* vacio */
      { $$ = nil_Formals(); }
  | formal_list_ne
      { $$ = $1; }
  ;

formal_list_ne
  : formal
      { $$ = single_Formals($1); }
  | formal_list_ne ',' formal
      { $$ = append_Formals($1, single_Formals($3)); }
  ;

/*
 * formal ::= ID : TYPE
 */
formal
  : OBJECTID ':' TYPEID
      { $$ = formal($1, $3); }
  ;

/*
 * expr - expresiones principales
 * La precedencia y asociatividad la manejan las directivas arriba
 */
expr
  /* Asignacion: ID <- expr */
  : OBJECTID ASSIGN expr
      { $$ = assign($1, $3); }

  /* Dispatch dinamico: expr.ID(args) */
  | expr '.' OBJECTID '(' actual_list ')'
      { $$ = dispatch($1, $3, $5); }

  /* Dispatch estatico: expr@TYPE.ID(args) */
  | expr '@' TYPEID '.' OBJECTID '(' actual_list ')'
      { $$ = static_dispatch($1, $3, $5, $7); }

  /* Dispatch implicito (self.): ID(args) */
  | OBJECTID '(' actual_list ')'
      { $$ = dispatch(object(idtable.add_string("self")), $1, $3); }

  /* Condicional: if expr then expr else expr fi */
  | IF expr THEN expr ELSE expr FI
      { $$ = cond($2, $4, $6); }

  /* Bucle: while expr loop expr pool */
  | WHILE expr LOOP expr POOL
      { $$ = loop($2, $4); }

  /* Bloque: { expr; expr; ... } */
  | '{' expr_list '}'
      { $$ = block($2); }

  /* Let con uno o mas identificadores */
  | LET let_expr %prec LET_PREC
      { $$ = $2; }

  /* Case: case expr of branches esac */
  | CASE expr OF case_list ESAC
      { $$ = typcase($2, $4); }

  /* new TYPE */
  | NEW TYPEID
      { $$ = new_($2); }

  /* isvoid expr */
  | ISVOID expr
      { $$ = isvoid($2); }

  /* Aritmetica */
  | expr '+' expr
      { $$ = plus($1, $3); }
  | expr '-' expr
      { $$ = sub($1, $3); }
  | expr '*' expr
      { $$ = mul($1, $3); }
  | expr '/' expr
      { $$ = divide($1, $3); }

  /* Negacion aritmetica: ~expr */
  | '~' expr
      { $$ = neg($2); }

  /* Comparadores */
  | expr '<' expr
      { $$ = lt($1, $3); }
  | expr LE expr
      { $$ = leq($1, $3); }
  | expr '=' expr
      { $$ = eq($1, $3); }

  /* Negacion logica: not expr */
  | NOT expr
      { $$ = comp($2); }

  /* Expresion parentizada */
  | '(' expr ')'
      { $$ = $2; }

  /* Identificador de objeto */
  | OBJECTID
      { $$ = object($1); }

  /* Constante entera */
  | INT_CONST
      { $$ = int_const($1); }

  /* Constante de cadena */
  | STR_CONST
      { $$ = string_const($1); }

  /* Constante booleana */
  | BOOL_CONST
      { $$ = bool_const($1); }
  ;

/*
 * let_expr - maneja la lista de variables del let
 * El let se transforma en lets anidados con un solo identificador
 *
 * let ID : TYPE [ <- expr ] in expr
 * let ID : TYPE [ <- expr ], ID : TYPE [ <- expr ], ... in expr
 */
let_expr
  : OBJECTID ':' TYPEID ASSIGN expr IN expr
      { $$ = let($1, $3, $5, $7); }
  | OBJECTID ':' TYPEID IN expr
      { $$ = let($1, $3, no_expr(), $5); }
  | OBJECTID ':' TYPEID ASSIGN expr ',' let_expr
      { $$ = let($1, $3, $5, $7); }
  | OBJECTID ':' TYPEID ',' let_expr
      { $$ = let($1, $3, no_expr(), $5); }
  | error IN expr
      { $$ = $3; }
  | error ',' let_expr
      { $$ = $3; }
  ;

/*
 * case_list - ramas del case
 */
case_list
  : case_branch
      { $$ = single_Cases($1); }
  | case_list case_branch
      { $$ = append_Cases($1, single_Cases($2)); }
  ;

/*
 * case_branch ::= ID : TYPE => expr ;
 */
case_branch
  : OBJECTID ':' TYPEID DARROW expr ';'
      { $$ = branch($1, $3, $5); }
  ;

/*
 * expr_list - lista de expresiones en un bloque
 * Con recuperacion de errores
 */
expr_list
  : expr ';'
      { $$ = single_Expressions($1); }
  | expr_list expr ';'
      { $$ = append_Expressions($1, single_Expressions($2)); }
  | error ';'
      { $$ = nil_Expressions(); }
  | expr_list error ';'
      { $$ = $1; }
  ;

/*
 * actual_list - argumentos de una llamada a metodo
 */
actual_list
  : /* vacio */
      { $$ = nil_Expressions(); }
  | actual_list_ne
      { $$ = $1; }
  ;

actual_list_ne
  : expr
      { $$ = single_Expressions($1); }
  | actual_list_ne ',' expr
      { $$ = append_Expressions($1, single_Expressions($3)); }
  ;

%%

/* ========================
   CODIGO C AUXILIAR
   ======================== */

/*
 * yyerror: funcion de reporte de errores del parser.
 * Bison la invoca automaticamente cuando detecta un error sintactico.
 */
void yyerror(char *s)
{
  extern int curr_lineno;

  cerr << "\"" << curr_filename << "\", line " << curr_lineno << ": "
       << s << " at or near ";
  print_cool_token(yychar);
  cerr << endl;
  omerrs++;

  if(omerrs > 50) {
    fprintf(stderr, "More than 50 errors\n");
    exit(1);
  }
}
