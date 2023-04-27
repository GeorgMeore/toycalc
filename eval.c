#include "eval.h"

#include <math.h>
#include <stdio.h>

#include "node.h"


static int eval_sum(Node *expr, double *result)
{
	double left, right;
	if (!eval(expr->as.pair.left, &left)) {
		return 0;
	}
	if (!eval(expr->as.pair.right, &right)) {
		return 0;
	}
	*result = left + right;
	return 1;
}

static int eval_product(Node *expr, double *result)
{
	double left, right;
	if (!eval(expr->as.pair.left, &left)) {
		return 0;
	}
	if (!eval(expr->as.pair.right, &right)) {
		return 0;
	}
	*result = left * right;
	return 1;
}

static int eval_expt(Node *expr, double *result)
{
	double left, right;
	if (!eval(expr->as.pair.left, &left)) {
		return 0;
	}
	if (!eval(expr->as.pair.right, &right)) {
		return 0;
	}
	*result = pow(left, right);
	return 1;
}

static int eval_cmp(Node *expr, double *result)
{
	double left, right;
	if (!eval(expr->as.pair.left, &left)) {
		return 0;
	}
	if (!eval(expr->as.pair.right, &right)) {
		return 0;
	}
	*result = left > right;
	return 1;
}

static int eval_and(Node *expr, double *result)
{
	double left;
	if (!eval(expr->as.pair.left, &left)) {
		return 0;
	}
	if (left != 0) {
		return eval(expr->as.pair.right, result);
	}
	*result = 0;
	return 1;
}

static int eval_or(Node *expr, double *result)
{
	double left;
	if (!eval(expr->as.pair.left, &left)) {
		return 0;
	}
	if (left != 0) {
		*result = left;
		return 1;
	}
	return eval(expr->as.pair.right, result);
}

static int eval_if(Node *expr, double *result)
{
	double cond;
	if (!eval(expr->as.ifelse.cond, &cond)) {
		return 0;
	}
	if (cond != 0) {
		return eval(expr->as.ifelse.true, result);
	}
	return eval(expr->as.ifelse.false, result);
}

int eval(Node *expr, double *result)
{
	switch (expr->type) {
		case NUMBER_NODE:
			*result = expr->as.number;
			return 1;
		case SUM_NODE:
			return eval_sum(expr, result);
		case PRODUCT_NODE:
			return eval_product(expr, result);
		case EXPT_NODE:
			return eval_expt(expr, result);
		case CMP_NODE:
			return eval_cmp(expr, result);
		case AND_NODE:
			return eval_and(expr, result);
		case OR_NODE:
			return eval_or(expr, result);
		case IF_NODE:
			return eval_if(expr, result);
		default:
			fprintf(stderr, "evaluation error: not implemented\n");
			return 0;
	}
}
