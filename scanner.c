#include "lex.h"

#include "iter.h"
#include "scanner.h"
#include "token.h"


Scanner Scanner_make(FILE *file)
{
	Scanner scanner;
	scanner.iterator = Iter_make(file);
	scanner.next = (Token){ErrorToken, "", 0};
	return scanner;
}

void Scanner_destroy(Scanner scanner)
{
	Iter_destroy(scanner.iterator);
}

void Scanner_start(Scanner *scanner)
{
	Iter_reset(&scanner->iterator);
	scanner->next = take_token(&scanner->iterator);
}

Token Scanner_next(Scanner *scanner)
{
	Token next = scanner->next;
	scanner->next = take_token(&scanner->iterator);
	return next;
}

Token Scanner_peek(Scanner *scanner)
{
	return scanner->next;
}

void Scanner_seek_end(Scanner *scanner)
{
	while (scanner->next.type != EndToken) {
		scanner->next = take_token(&scanner->iterator);
	}
}

void Scanner_skip_nl(Scanner *scanner)
{
	while (scanner->next.type == EndToken && *scanner->next.string == '\n') {
		scanner->next = take_token(&scanner->iterator);
	}
}
