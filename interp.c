#include <stdio.h>
#include <unistd.h>

#include "opts.h"
#include "scanner.h"
#include "parse.h"
#include "node.h"
#include "infer.h"
#include "types.h"
#include "eval.h"
#include "arena.h"


#define TMP_ARENA_PAGE_SIZE 4096

int main(int argc, char **argv)
{
	if (!parse_args(argc, argv)) {
		return 1;
	}
	int tty = isatty(0);
	Scanner scanner = Scanner_make(stdin);
	Context ctx = Context_make();
	// TODO: maybe make those parts of the context?
	TypeEnv *tenv = TYPEENV_EMPTY;
	Arena tmp = Arena_make(TMP_ARENA_PAGE_SIZE);
	Arena longtmp = Arena_make(TMP_ARENA_PAGE_SIZE);
	while (!Scanner_eof(scanner)) {
		Arena_reset(&tmp);
		if (tty) {
			fprintf(stderr, "> ");
		}
		Node *ast = parse(&scanner, &longtmp);
		if (!ast) {
			continue;
		}
		Type *type = NULL;
		if (typed) {
			type = infer(ast, &tenv, &tmp);
			if (!type) {
				continue;
			}
		}
		if (debug) {
			Node_println(ast);
		}
		Object *result = eval(ast, &ctx);
		if (!result) {
			continue;
		}
		Object_print(result);
		if (type) {
			printf(" :: ");
			Type_print(type);
		}
		printf("\n");
	}
	Scanner_destroy(scanner);
	Context_destroy(ctx);
	TypeEnv_drop(tenv);
	Arena_destroy(tmp);
	Arena_destroy(longtmp);
	return 0;
}
