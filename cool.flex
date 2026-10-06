
/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST + 100]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */
#define ERR(err) do { yylval.error_msg = err; return (ERROR); } while (0)
#define ASS(cond, err) do { if (cond) ERR(err); } while (0)

#define string_buf_push(c) \
	ASS(string_buf_ptr - string_buf >= MAX_STR_CONST, "String constant too long"); \
	*string_buf_ptr++ = (c);

int ineof = 0;
int failstr = 0;
%}

/*
 * Define names for regular expressions here.
 */

CLASS		[Cc][Ll][Aa][Ss][Ss]
ELSE		[Ee][Ll][Ss][Ee]
FI		[Ff][Ii]
IF		[Ii][Ff]
IN		[Ii][Nn]
INHERITS	[Ii][Nn][Hh][Ee][Rr][Ii][Tt][Ss]
LET		[Ll][Ee][Tt]
LOOP		[Ll][Oo][Oo][Pp]
POOL		[Pp][Oo][Oo][Ll]
THEN		[Tt][Hh][Ee][Nn]
WHILE		[Ww][Hh][Ii][Ll][Ee]
CASE		[Cc][Aa][Ss][Ee]
ESAC		[Ee][Ss][Aa][Cc]
OF		[Oo][Ff]
DARROW          =>
NEW		[Nn][Ee][Ww]
ISVOID		[Ii][Ss][Vv][Oo][Ii][Dd]
TYPEID		[A-Z][a-zA-Z0-9_]*
OBJECTID	[a-z][a-zA-Z0-9_]*
ASSIGN		<- 
NOT		[Nn][Oo][Tt]
LE		<=

TRUE		t[Rr][Uu][Ee]
FALSE		f[Aa][Ll][Ss][Ee]

WS		[ \v\t\r\f]*

ONECHAR 	[\+\/\-\*\=\<\.\~\,\;\:\(\)\@\{\}]
DIGIT		[0-9]+

%x str

%%

{WS}			{ }

 /*
  *  Nested comments
  */
--[^\n]*		{ }
\n			{ curr_lineno++; }

"(*"			{
	int c;
	int nst = 1;
	while ((c = yyinput()) != 0)
	{
		if (c == '\n')
			curr_lineno++;
		else if (c == '*')
		{
			int cc = yyinput();
			if (cc == ')')
			{
				if (--nst == 0) break;
			}
			else
				unput(cc);
		}
		else if (c == '(')
		{
			int cc = yyinput();
			if (cc == '*')
				nst++;
			else
				unput(cc);
		}
		if (EOF == c)
		{
			if (ineof)
				yyterminate();
			ERR("EOF in comment");

		}
	}	

	ASS(0 == c, "Comments contains null character");
			} // end of comments


"*)" 			{ ERR("Unmatched *)"); }

 
 /*
  *  The multiple-character operators.
  */
{DARROW}		{ return (DARROW); }
{ASSIGN}		{ return (ASSIGN); }
{LE}			{ return (LE); }

{ONECHAR}		{ return (yytext[0]); }
{DIGIT}			{ yylval.symbol = inttable.add_string(yytext); return (INT_CONST); }

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */

{CLASS} 		{ return (CLASS); }
{ELSE}			{ return (ELSE); }
{FI}			{ return (FI); }
{IF}			{ return (IF); }
{IN}			{ return (IN); }
{INHERITS}		{ return (INHERITS); }
{LET}			{ return (LET); }
{LOOP}			{ return (LOOP); }
{POOL}			{ return (POOL); }
{THEN}			{ return (THEN); }
{WHILE}			{ return (WHILE); }
{CASE}			{ return (CASE); }
{ESAC}			{ return (ESAC); }
{OF}			{ return (OF); }
{NEW}			{ return (NEW); }
{ISVOID}		{ return (ISVOID); }
{NOT}			{ return (NOT); }

{TRUE}			{ yylval.boolean = 1; return (BOOL_CONST); }
{FALSE}			{ yylval.boolean = 0; return (BOOL_CONST); }

{TYPEID}		{ yylval.symbol = idtable.add_string(yytext); return (TYPEID); }
{OBJECTID}		{ yylval.symbol = idtable.add_string(yytext); return (OBJECTID); }

_			{ ERR("_"); }

 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

\"			{ string_buf_ptr = string_buf; failstr = 0; BEGIN(str);}
<str>(\"|\n)		{ 
	BEGIN(INITIAL); 
	if (failstr == 0)
	{
		ASS(*yytext == '\n', "Unterminated string constant");
		string_buf_push('\0');
		yylval.symbol = stringtable.add_string(string_buf);
		return (STR_CONST);
	}
			}
<str>\0			{ failstr = 1; ERR("String contains null character."); }
<str><<EOF>>		{ if (ineof) yyterminate(); ineof = 1; ERR("EOF in string constant");}
<str>\\n		{ string_buf_push('\n'); }
<str>\\t		{ string_buf_push('\t'); }
<str>\\b		{ string_buf_push('\b'); }
<str>\\f		{ string_buf_push('\f'); }

<str>\\(.|\n)		{ 
	if (0 == yytext[1])
		failstr = 1;
	ASS(0 == yytext[1], "String contains escaped null character.");

	if (EOF == yytext[1])
	{
		if (ineof)
			yyterminate(); 
		ineof = 1;
		ERR("EOF in string constant");
	}
	string_buf_push(yytext[1]); curr_lineno += (yytext[1] == '\n'); 
			}
<str>[^\\\n\"]		{ 
	if (EOF == yytext[0])
	{
		if (ineof)
			yyterminate(); 
		ineof = 1;
		ERR("EOF in string constant");
	}
	if (0 == yytext[0])
		failstr = 1;
	ASS(0 == yytext[0], "String contains null character.");

	string_buf_push(yytext[0]); 
			}


.			{ ERR(yytext); }

%%
