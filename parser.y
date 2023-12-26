%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor

%{
  #include "Header.h"
  bool from_file = false;
  AST::file_t file;
  Eval::Context global_context;
  int dbg_counter = 1;
  
  # define YY_DECL \
    yy::parser::symbol_type yylex ()
  // ... and declare it for the parser's sake.
  YY_DECL;
%}

//%glr-parser
%token
  LPAREN        "("
  RPAREN        ")"
  LBRACKET      "["
  RBRACKET      "]"
  LCURLY        "{"
  RCURLY        "}"
  COMMA         ","
  DOT           "."
  COLON         ":"
  SEMICOLON     ";"
  ASSIGNMENT    "="
  NOT           "!"
  AND           "&"
  NAND          "!&"
  OR            "|"
  NOR           "!|"
  XOR           "^"
  IMPL          "->"
  IMPL2         "=>"
  NIMPL         "!->"
  LIMPL         "<-"
  NLIMPL        "!<-"
  EQV           "<->"
  
  //HYP           "hypothesis"
  RULE          "rule"
  AXIOM         "axiom"

  ;

  %left "<->"
  %left "->" "!->"
  %left "<-" "!<-"
  %left "^"
  %left "&" "|"
  %left "!&" "!|"
  %precedence "!"

  //%locations
%%

%token TOK_EOF;
%token <std::string> SYM;

Source  : File TOK_EOF { std::cout << "\nParsing Finished!" << std::endl; file = std::move($1);  YYACCEPT; };

%nterm<AST::file_t> File;
File  : RuleBlock       { if (from_file) { $$ = {$1}; } 
                          else { if (!Eval::eval($1, global_context)) std::cout << "Error: " << $1 << std::endl; } }
      | AxiomBlock      { if (from_file) { $$ = {$1}; } 
                          else { if (!Eval::eval($1, global_context)) std::cout << "Error: " << $1 << std::endl; } }
      | File RuleBlock  { if (from_file) { $$ = $1; $$.push_back($2); } 
                          else { if (!Eval::eval($2, global_context)) std::cout << "Error: " << $2 << std::endl; } }
      | File AxiomBlock { if (from_file) { $$ = $1; $$.push_back($2); } 
                          else { if (!Eval::eval($2, global_context)) std::cout << "Error: " << $2 << std::endl; } };

%nterm<AST::Hypothesis> Hypothesis;
%nterm<AST::Hypothesis> HypothesisBegin;
Hypothesis      : HypothesisBegin "}"   { $$ = $1; }
                ;
HypothesisBegin : SYM "=" Expr "->" LCURLY     { $$ = AST::Hypothesis($1, $3); }
                | HypothesisBegin Statement  { $$ = $1; $$.add_statement($2); }
                ;

%nterm<AST::RuleBlock> RuleBlock;
%nterm<AST::RuleBlock> RuleBlockStem;
%nterm<AST::RuleBlock> RuleBlockDeclaration;
%nterm<AST::RuleBlock> RuleBlockDeclarationStem;
RuleBlock                 : RuleBlockStem "}"                       { $$ = $1; };
RuleBlockStem             : RuleBlockDeclaration LCURLY             { $$ = $1; }
                          | RuleBlockStem Statement                 { $$ = $1; $$.add_statement($2); };
RuleBlockDeclaration      : RuleBlockDeclarationStem ")" "=>" Expr  { $$ = $1; $$.add_transformat($4); }
                          | "rule" SYM "(" ")" "=>" Expr            { $$ = AST::RuleBlock($2);; $$.add_transformat($6); }
                          | RuleBlockDeclaration "," Expr           { $$ = $1; $$.add_transformat($3); };
RuleBlockDeclarationStem  : "rule" SYM "(" Expr                     { $$ = AST::RuleBlock($2); $$.add_argument($4); }
                          | RuleBlockDeclarationStem "," Expr       { $$ = $1; $$.add_argument($3); };

%nterm<AST::AxiomBlock> AxiomBlock;
%nterm<AST::AxiomBlock> AxiomBlockStem2;
%nterm<AST::AxiomBlock> AxiomBlockStem;
AxiomBlock     : AxiomBlockStem2 ";"           { $$ = $1; };
AxiomBlockStem2 : AxiomBlockStem ")" "=>" Expr { $$ = $1; $$.add_transformat($4); }
                | AxiomBlockStem2 "," Expr     { $$ = $1; $$.add_transformat($3); };
AxiomBlockStem : "axiom" SYM "(" Expr          { $$ = AST::AxiomBlock($2); $$.add_argument($4); }
          | AxiomBlockStem "," Expr            { $$ = $1; $$.add_argument($3); };

%nterm<AST::statement_t> Statement;
Statement : SYM "=" Expr ":" RuleInvocation ";" { $$ = AST::Definition($1, $3, $5); }
          | Hypothesis                          { $$ = $1; };

%nterm<AST::RuleInvocation> RuleInvocation;
%nterm<AST::RuleInvocation> RuleInvocationWithArguments;
RuleInvocation  : SYM "(" ")"                     { $$ = AST::RuleInvocation($1); }
                | RuleInvocationWithArguments ")" { $$ = $1; }
                ;
RuleInvocationWithArguments : SYM "(" SYM                           { $$ = AST::RuleInvocation($1, $3); }
                            | RuleInvocationWithArguments "," SYM   { $$ = $1; $$.add($3); }
                            ;

%nterm <AST::lformula_t> Op;
Op  : Expr "<->" Expr   {$$ = AST::BinOp(AST::BinOpID::Equivalence, $1, $3);}
    | Expr "->" Expr    {$$ = AST::BinOp(AST::BinOpID::Implication, $1, $3);}
    | Expr "!->" Expr   {$$ = AST::BinOp(AST::BinOpID::NImplication, $1, $3);}
    | Expr "<-" Expr    {$$ = AST::BinOp(AST::BinOpID::ReverseImplication, $1, $3);}
    | Expr "!<-" Expr   {$$ = AST::BinOp(AST::BinOpID::NReverseImplication, $1, $3);}
    | Expr "^" Expr     {$$ = AST::BinOp(AST::BinOpID::Xor, $1, $3);}
    | Expr "|" Expr     {$$ = AST::BinOp(AST::BinOpID::Or, $1, $3);}
    | Expr "!|" Expr    {$$ = AST::BinOp(AST::BinOpID::Nor, $1, $3);}
    | Expr "&" Expr     {$$ = AST::BinOp(AST::BinOpID::And, $1, $3);}
    | Expr "!&" Expr    {$$ = AST::BinOp(AST::BinOpID::Nand, $1, $3);}
    | "!" Expr          {$$ = AST::UnOp(AST::UnOpID::Not, $2);}
    ;


%nterm <AST::lformula_t> Expr;
Expr    : SYM           { $$ = AST::lformula_t($1); }
        | "(" Expr ")"  { $$ = $2; }
//        | "(" Op ")"    { $$ = AST::lformula_t($2); }
        |  Op           { $$ = $1; }
        ;


%%
namespace yy
{
  // Report an error to the user.
  auto parser::error (const std::string& msg) -> void
  {
    std::cerr <<"line " << line << ": " << msg << ": " << yytext << '\n';
  }
}
unsigned int line = 0;

int main(int argc, char** argv) {

	if ( argc > 1 ) {
		yyin = fopen( argv[1], "r" );
    // https://stackoverflow.com/questions/42876210/c-fopen-opening-directories
    //std::cout << yyin << std::endl;
    if (yyin == NULL) {
      std::cout << "File '" << argv[1] << "' not found.";
      return -1;
    }
		from_file = true;
	} else {
		yyin = stdin;
		printf("> ");
	}
	int line = 0;
	yy::parser parse;
	int counter = 0;
	while (true) {
		parse();
    if (from_file) {
      if (!Eval::eval(file, global_context)) {
        std::cout << "Error while evaluating file.\n";
      } else { std::cout << "Evaluation suceessful!\n"; }
    }
    break;
	}
    /*
    if (from_file) {
    	fclose(yyin);
    	restart = false;
    	break;
    }
    */
	return 0;
}
