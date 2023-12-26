%{
#include "Header.h"
#include "parser.tab.hpp"

// https://en.cppreference.com/w/cpp/language/escape
// Code of this function based on https://stackoverflow.com/questions/5612182/convert-string-with-explicit-escape-sequence-into-relative-character
std::string unescape(const std::string &str) {
  std::string result;
  std::string::const_iterator it = str.begin();
  while (it != str.end()) {
    char c = *it++;
    if (c == '\\' && it != str.end()) {
      switch (*it++) {
        case '\'':
          c = '\'';
          break;
        case '\"':
          c = '\"';
          break;
        case '\?':
          c = '\?';
          break;
        case '\\':
          c = '\\';
          break;
        case 'a':
          c = '\a';
          break;
        case 'b':
          c = '\b';
          break;
        case 'f':
          c = '\f';
          break;
        case 'n':
          c = '\n';
          break;
        case 'r':
          c = '\r';
          break;
        case 't':
          c = '\t';
          break;
        case 'v':
          c = '\v';
          break;
        default:
          // invalid escape sequence
          continue;
      }
    }
    result += c;
  }
  return result;
}

// Tell Flex the lexer's prototype ...
# define YY_DECL \
  yy::parser::symbol_type yylex ()
// ... and declare it for the parser's sake.
YY_DECL;
%}

%option noyywrap

%%



\(                          { return yy::parser::make_LPAREN(); }
\)                          { return yy::parser::make_RPAREN(); }
\[                          { return yy::parser::make_LBRACKET(); }
\]                          { return yy::parser::make_RBRACKET(); }
\{                          { return yy::parser::make_LCURLY(); }
\}                          { return yy::parser::make_RCURLY(); }
\,                          { return yy::parser::make_COMMA(); }
\.                          { return yy::parser::make_DOT(); }
\:                          { return yy::parser::make_COLON(); }
\;                          { return yy::parser::make_SEMICOLON(); }
"//".*                      { /* Comment */ }
\=                          { return yy::parser::make_ASSIGNMENT(); }
\&                          { return yy::parser::make_AND(); }
"!&"                        { return yy::parser::make_NAND(); }
"|"                         { return yy::parser::make_OR(); }
"!|"                        { return yy::parser::make_NOR(); }
\^                          { return yy::parser::make_XOR(); }
"->"                        { return yy::parser::make_IMPL(); }
"=>"                        { return yy::parser::make_IMPL2(); }
"!->"                       { return yy::parser::make_NIMPL(); }
"<-"                        { return yy::parser::make_LIMPL(); }
"!<-"                       { return yy::parser::make_NLIMPL(); }
"<->"                       { return yy::parser::make_EQV(); }
\!                          { return yy::parser::make_NOT(); }

"rule"                      { return yy::parser::make_RULE(); }
"axiom"                     { return yy::parser::make_AXIOM(); }

\n                          { if (!from_file) { std::cout << "> "; } ++line; }
[ \t\v\f]                   { ;  }
[a-zA-Z_0-9]*               { return yy::parser::make_SYM( std::string(yytext) ); }
<<EOF>>                     { return yy::parser::make_TOK_EOF(); }
%%
