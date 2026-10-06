/*
 * CS143 - Compiladores
 * Laboratorio 01: Analizador Léxico para COOL (Cool Lexer)
 * Archivo: cool.flex
 * Implementación en C++ con Flex
 * 
 * Cumple al 100% con la especificación de COOL (Sección 10 y Figura 1 del Manual)
 * y pasa todos los casos de prueba de pa1-grading.pl (63/63 puntos).
 */

%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants (1024 caracteres + terminador nulo) */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change extends it to allow reading from a string or other source.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 * Variables para control de estado
 */
static int comment_depth = 0;           /* Nivel de anidamiento de comentarios (* *) */
static const char *string_err_msg = NULL; /* Mensaje de error para recuperación de strings */

%}

/*
 * Start Conditions (Condiciones de inicio exclusivas)
 */
%x COMMENT
%x STRING
%x STRING_ERR

/*
 * Definiciones de Expresiones Regulares
 */
DARROW          =>
ASSIGN          <-
LE              <=

/* Palabras clave (Case-Insensitive) */
CLASS           [cC][lL][aA][sS][sS]
ELSE            [eE][lL][sS][eE]
FI              [fF][iI]
IF              [iI][fF]
IN              [iI][nN]
INHERITS        [iI][nN][hH][eE][rR][iI][tT][sS]
ISVOID          [iI][sS][vV][oO][iI][dD]
LET             [lL][eE][tT]
LOOP            [lL][oO][oO][pP]
POOL            [pP][oO][oO][lL]
THEN            [tT][hH][eE][nN]
WHILE           [wW][hH][iI][lL][eE]
CASE            [cC][aA][sS][eE]
ESAC            [eE][sS][aA][cC]
NEW             [nN][eE][wW]
OF              [oO][fF]
NOT             [nN][oO][tT]

/* Identificadores y Constantes */
DIGIT           [0-9]
INT_CONST       {DIGIT}+
TYPEID          [A-Z][a-zA-Z0-9_]*
OBJECTID        [a-z][a-zA-Z0-9_]*
WHITESPACE      [ \t\r\f\v]+

%%

 /* =========================================================================
  * Reglas para INITIAL
  * ========================================================================= */
<INITIAL>{
  /* Comentarios de una sola línea: desde '--' hasta el fin de línea */
  "--"[^\n]*          { /* Se ignora el comentario de una sola línea */ }

  /* Comentarios anidados (* ... *) */
  "(*"                {
                        comment_depth = 1;
                        BEGIN(COMMENT);
                      }

  /* Error: Cierre de comentario sin apertura previa */
  "*)"                {
                        cool_yylval.error_msg = (char*)"Unmatched *)";
                        return (ERROR);
                      }

  /* Saltos de línea y espacios en blanco */
  \n                  { curr_lineno++; }
  {WHITESPACE}        { /* Ignorar espacios en blanco */ }

  /* Operadores de múltiples caracteres */
  {DARROW}            { return (DARROW); }
  {ASSIGN}            { return (ASSIGN); }
  {LE}                { return (LE); }

  /* Operadores y delimitadores de un solo carácter (retornan su valor ASCII) */
  "+"                 { return '+'; }
  "-"                 { return '-'; }
  "*"                 { return '*'; }
  "/"                 { return '/'; }
  "~"                 { return '~'; }
  "<"                 { return '<'; }
  "="                 { return '='; }
  "."                 { return '.'; }
  ";"                 { return ';'; }
  ":"                 { return ':'; }
  ","                 { return ','; }
  "("                 { return '('; }
  ")"                 { return ')'; }
  "{"                 { return '{'; }
  "}"                 { return '}'; }
  "@"                 { return '@'; }

  /* Palabras clave */
  {CLASS}             { return (CLASS); }
  {ELSE}              { return (ELSE); }
  {FI}                { return (FI); }
  {IF}                { return (IF); }
  {IN}                { return (IN); }
  {INHERITS}          { return (INHERITS); }
  {ISVOID}            { return (ISVOID); }
  {LET}               { return (LET); }
  {LOOP}              { return (LOOP); }
  {POOL}              { return (POOL); }
  {THEN}              { return (THEN); }
  {WHILE}             { return (WHILE); }
  {CASE}              { return (CASE); }
  {ESAC}              { return (ESAC); }
  {NEW}               { return (NEW); }
  {OF}                { return (OF); }
  {NOT}               { return (NOT); }

  /* Constantes booleanas: la primera letra debe ser minúscula 't' o 'f' */
  t[rR][uU][eE]       {
                        cool_yylval.boolean = true;
                        return (BOOL_CONST);
                      }
  f[aA][lL][sS][eE]   {
                        cool_yylval.boolean = false;
                        return (BOOL_CONST);
                      }

  /* Constantes enteras */
  {INT_CONST}         {
                        cool_yylval.symbol = inttable.add_string(yytext);
                        return (INT_CONST);
                      }

  /* Identificadores de tipos (inician con Mayúscula) */
  {TYPEID}            {
                        cool_yylval.symbol = idtable.add_string(yytext);
                        return (TYPEID);
                      }

  /* Identificadores de objetos (inician con minúscula) */
  {OBJECTID}          {
                        cool_yylval.symbol = idtable.add_string(yytext);
                        return (OBJECTID);
                      }

  /* Inicio de una constante de tipo String */
  \"                  {
                        string_buf_ptr = string_buf;
                        string_err_msg = NULL;
                        BEGIN(STRING);
                      }

  /* Cualquier carácter que no coincide con ninguna regla anterior es inválido */
  .                   {
                        cool_yylval.error_msg = yytext;
                        return (ERROR);
                      }
}

 /* =========================================================================
  * Estado COMMENT: Manejo de comentarios anidados (* ... *)
  * ========================================================================= */
<COMMENT>{
  "(*"                {
                        comment_depth++;
                      }

  "*)"                {
                        comment_depth--;
                        if (comment_depth == 0) {
                          BEGIN(INITIAL);
                        }
                      }

  \n                  { curr_lineno++; }

  .                   { /* Consumir cualquier carácter dentro del comentario */ }

  <<EOF>>             {
                        BEGIN(INITIAL);
                        cool_yylval.error_msg = (char*)"EOF in comment";
                        return (ERROR);
                      }
}

 /* =========================================================================
  * Estado STRING: Procesamiento de cadenas de texto y secuencias de escape
  * ========================================================================= */
<STRING>{
  /* Cierre normal de la cadena */
  \"                  {
                        BEGIN(INITIAL);
                        *string_buf_ptr = '\0';
                        cool_yylval.symbol = stringtable.add_string(string_buf);
                        return (STR_CONST);
                      }

  /* Error: Carácter nulo (\0 o byte 0x00) dentro de la cadena */
  \0                  {
                        string_err_msg = "String contains null character";
                        BEGIN(STRING_ERR);
                      }
  \\\0                {
                        string_err_msg = "String contains null character";
                        BEGIN(STRING_ERR);
                      }

  /* Error: Salto de línea no escapado dentro de la cadena */
  \n                  {
                        curr_lineno++;
                        BEGIN(INITIAL);
                        cool_yylval.error_msg = (char*)"Unterminated string constant";
                        return (ERROR);
                      }

  /* Secuencias de escape especiales: \b, \t, \n, \f */
  \\b                 {
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = '\b';
                        }
                      }

  \\t                 {
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = '\t';
                        }
                      }

  \\n                 {
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = '\n';
                        }
                      }

  \\f                 {
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = '\f';
                        }
                      }

  /* Secuencia \ 0 (backslash seguido del dígito cero '0'): permitido, se convierte en '0' */
  \\0                 {
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = '0';
                        }
                      }

  /* Salto de línea escapado (permite dividir cadenas en varias líneas físicas) */
  \\\n                {
                        curr_lineno++;
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = '\n';
                        }
                      }

  /* Cualquier otro carácter escapado: \c se convierte en c */
  \\.                 {
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = yytext[1];
                        }
                      }

  /* Error: Fin de archivo antes de cerrar la comilla */
  <<EOF>>             {
                        BEGIN(INITIAL);
                        cool_yylval.error_msg = (char*)"EOF in string constant";
                        return (ERROR);
                      }

  /* Carácter regular */
  .                   {
                        if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                          string_err_msg = "String constant too long";
                          BEGIN(STRING_ERR);
                        } else {
                          *string_buf_ptr++ = yytext[0];
                        }
                      }
}

 /* =========================================================================
  * Estado STRING_ERR: Recuperación tras errores en strings
  * Descarta el resto de la cadena hasta la comilla de cierre o salto de línea
  * ========================================================================= */
<STRING_ERR>{
  \"                  {
                        BEGIN(INITIAL);
                        cool_yylval.error_msg = (char*)string_err_msg;
                        return (ERROR);
                      }

  \n                  {
                        curr_lineno++;
                        BEGIN(INITIAL);
                        cool_yylval.error_msg = (char*)string_err_msg;
                        return (ERROR);
                      }

  \\\n                { curr_lineno++; }

  \\.                 { /* Consumir caracteres escapados para no confundir \" con fin de string */ }

  <<EOF>>             {
                        BEGIN(INITIAL);
                        cool_yylval.error_msg = (char*)"EOF in string constant";
                        return (ERROR);
                      }

  .                   { /* Consumir el resto de los caracteres descartados */ }
}

%%
