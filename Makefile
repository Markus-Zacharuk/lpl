CPP = lex.yy.cpp parser.tab.cpp Eval.cpp AST.cpp

lpl: lex.yy.cpp parser.tab.cpp
	   clang++ $(CPP) -std=c++23 -O3 -o lpl

debug: lex.yy.cpp parser.tab.cpp
	   clang++ $(CPP) -ggdb -std=c++23 -Og -o lpldbg

parser.tab.cpp: parser.y
	   bison -d --verbose --graph -Wall parser.y -o parser.tab.cpp

lex.yy.cpp: lexer.l
	   flex lexer.l
	   cp lex.yy.c lex.yy.cpp
	   rm lex.yy.c

graph: parser.tab.cpp
	   xdot parser.dot

conflicts:
	   cat parser.output

clean:
	   rm -rf lex.yy.c lex.yy.cpp lex.yy.cc parser.tab.cpp parser.tab.c parser.tab.cc parser.tab.h parser.tab.hpp parser.tab.hh parser.output lpldbg lpl location.hh
