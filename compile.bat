gcc -c  ctree.c heap.c nmetab.c prnttree.c symtab.c token.c tree.c dsm_extension.c treestk.c lexer.c gram.c && g++ -c CodeGenerator.cpp
g++ -o compiler ctree.o heap.o nmetab.o prnttree.o symtab.o token.o tree.o dsm_extension.o treestk.o lexer.o gram.o CodeGenerator.o
del *.o
PAUSE

