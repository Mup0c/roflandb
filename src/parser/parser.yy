%{ /*** C/C++ Declarations ***/

#include <stdio.h>
#include <string>
#include <vector>

%}

%code requires{
    #include "command.h"
    #include "statements.h"
}

/*** yacc/bison Declarations ***/
%require "3.0"

/* add debug output code to generated parser. disable this for release
 * versions. */
%debug

/* start symbol is named "start" */
%start start

%output "parser.cc"

/* write out a header file containing the token defines */
%defines "parser.h"

/* use newer C++ skeleton file */
%skeleton "lalr1.cc"

/* namespace to enclose parser in */
%define api.prefix RoflanParser

/* set the parser's class identifier */
%define "parser_class_name" "Parser"

/* keep track of the current position within the input */
%locations
%initial-action
{
    // initialize the initial location object
    @$.begin.filename = @$.end.filename = &driver.streamname;
};

/* The driver is passed by reference to the parser and to the scanner. This
 * provides a simple but effective pure interface, not relying on global
 * variables. */
%parse-param { class Driver& driver }

/* verbose error messages */
%error-verbose

 /*** TOKENS ***/
%define api.value.type variant

%token			END	     0	"end of file"
%token			EOL		    "end of line"
%token <int>    INTEGER		"integer"
%token <std::string> 	    STRING "string"

%token CREATE
%token TABLE
%token SHOW
%token DROP
%token INT_TYPE

/*%type <int>	expr create_table*/
%type <std::shared_ptr<cmd::CreateStatement>> create_statement
%type <ColumnType>       column_type
%type <std::shared_ptr<cmd::Column>> column_def
%type <std::shared_ptr<std::vector<std::shared_ptr<cmd::Column>>>> column_def_list

 /*** END TOKENS ***/

%{

#include "driver.h"
#include "scanner.h"

/* this "connects" the bison parser in the driver to the flex scanner class
 * object. it defines the yylex() function call to pull the next token from the
 * current lexer object of the driver context. */
#undef yylex
#define yylex driver.lexer->lex

%}

%% /*** Grammar Rules ***/
start	: STRING {}
        | create_statement {}

create_statement :
        CREATE TABLE STRING '(' column_def_list ')' {
            $$ = std::make_shared<cmd::CreateStatement>($3.c_str());
            $$.get()->set_columns($5);
            driver.create_statement_shared = $$;
        }

column_def_list:
        column_def{
            $$ = std::make_shared<std::vector<std::shared_ptr<cmd::Column>>>();
            $$.get()->emplace_back($1);
        }
    |   column_def_list ',' column_def{
            $1->emplace_back($3);
            $$ = $1;
        }

column_def:
		STRING column_type {
		    $$ = std::make_shared<cmd::Column>($2, $1.c_str());
		}
	;

column_type:
        INT_TYPE { $$ = ColumnType::INT; }
    ;

%% /*** Additional Code ***/

void RoflanParser::Parser::error(const Parser::location_type& l,
			    const std::string& m)
{
    driver.error(l, m);
}