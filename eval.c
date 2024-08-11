#include "eval.h"

#include <math.h>
#include <string.h>

#include "opts.h"
#include "node.h"
#include "env.h"
#include "stack.h"
#include "gc.h"
#include "context.h"
#include "error.h"


#define ERROR_PREFIX "evaluation error"

static Object *eval_dispatch(const Node *expr, Context *ctx, Object *env);
static Object *actual_value(const Node *expr, Context *ctx, Object *env);

static inline Object *eval_expect(const Node *expr, Context *ctx, Object *env, ObjectType type)
{
	Object *obj = actual_value(expr, ctx, env);
	if (!obj) {
		return NULL;
	}
	if (obj->type != type) {
		error("type mismatch");
		return NULL;
	}
	return obj;
}

static Object *eval_lookup(const Node *id, Object *env)
{
	Object *value = Env_get(EnvObj_env(env), IdNode_value(id));
	if (!value) {
		errorf("unbound variable: %s", IdNode_value(id));
		return NULL;
	}
	return value;
}

static Object *eval_neg(const Node *neg, Context *ctx, Object *env)
{
	Object *value = eval_expect(NegNode_value(neg), ctx, env, NUM_OBJECT);
	if (!value) {
		return NULL;
	}
	return GC_alloc_number(ctx->gc, NumObj_num(value) * -1);
}

static Object *eval_pair(const Node *pair, Context *ctx, Object *env)
{
	int op = PairNode_op(pair);
	Context_stack_push(ctx, env);
	Object *leftv = eval_expect(PairNode_left(pair), ctx, env, NUM_OBJECT);
	if (!leftv) {
		return NULL;
	}
	Context_stack_pop(ctx);
	Context_stack_push(ctx, leftv);
	Object *rightv = eval_expect(PairNode_right(pair), ctx, env, NUM_OBJECT);
	if (!rightv) {
		return NULL;
	}
	Context_stack_pop(ctx);
	switch (op) {
		case '^':
			return GC_alloc_number(ctx->gc, pow(NumObj_num(leftv), NumObj_num(rightv)));
		case '*':
			return GC_alloc_number(ctx->gc, NumObj_num(leftv) * NumObj_num(rightv));
		case '/':
			return GC_alloc_number(ctx->gc, NumObj_num(leftv) / NumObj_num(rightv));
		case '%':
			return GC_alloc_number(ctx->gc, fmod(NumObj_num(leftv), NumObj_num(rightv)));
		case '+':
			return GC_alloc_number(ctx->gc, NumObj_num(leftv) + NumObj_num(rightv));
		case '-':
			return GC_alloc_number(ctx->gc, NumObj_num(leftv) - NumObj_num(rightv));
		case '>':
			return GC_alloc_number(ctx->gc, NumObj_num(leftv) > NumObj_num(rightv));
		case '<':
			return GC_alloc_number(ctx->gc, NumObj_num(leftv) < NumObj_num(rightv));
		case '=':
			return GC_alloc_number(ctx->gc, NumObj_num(leftv) == NumObj_num(rightv));
		default:
			errorf("unknown binary operation: '%c'", op);
			return NULL;
	}
}

static Object *eval_or(const Node *or, Context *ctx, Object *env)
{
	Object *leftv = eval_expect(PairNode_left(or), ctx, env, NUM_OBJECT);
	if (!leftv) {
		return NULL;
	}
	if (NumObj_num(leftv)) {
		return leftv;
	}
	Object *rightv = eval_expect(PairNode_right(or), ctx, env, NUM_OBJECT);
	if (!rightv) {
		return NULL;
	}
	return rightv;
}

static Object *eval_and(const Node *and, Context *ctx, Object *env)
{
	Object *leftv = eval_expect(PairNode_left(and), ctx, env, NUM_OBJECT);
	if (!leftv) {
		return NULL;
	}
	if (!NumObj_num(leftv)) {
		return leftv;
	}
	Object *rightv = eval_expect(PairNode_right(and), ctx, env, NUM_OBJECT);
	if (!rightv) {
		return NULL;
	}
	return rightv;
}

static Object *eval_let(const Node *let, Context *ctx, Object *env)
{
	Object *value = eval_dispatch(LetNode_value(let), ctx, env);
	if (!value) {
		return NULL;
	}
	Env_add(EnvObj_env(env), LetNode_name_value(let), value);
	return value;
}

static Node *eval_if(Context *ctx, Object **env, const Node *expr)
{
	Object *condv = eval_expect(IfNode_cond(expr), ctx, *env, NUM_OBJECT);
	if (!condv) {
		return NULL;
	}
	if (NumObj_num(condv)) {
		return IfNode_true(expr);
	} else {
		return IfNode_false(expr);
	}
}

static Node *eval_application(Context *ctx, Object **env, const Node *expr)
{
	Context_stack_push(ctx, *env);
	Object *fnv = eval_expect(PairNode_left(expr), ctx, *env, FN_OBJECT);
	if (!fnv) {
		return NULL;
	}
	Object *argv = NULL;
	if (lazy) {
		argv = GC_alloc_thunk(ctx->gc, *env, PairNode_right(expr));
	} else {
		Context_stack_push(ctx, fnv);
		argv = eval_dispatch(PairNode_right(expr), ctx, *env);
		if (!argv) {
			return NULL;
		}
		Context_stack_pop(ctx);
	}
	Context_stack_pop(ctx);
	*env = GC_alloc_env(ctx->gc, FnObj_env(fnv));
	Env_add(EnvObj_env(*env), FnObj_arg(fnv), argv);
	// NOTE: we "pin" the function to the current stack top to
	// keep it alive while it's body is being evaluated.
	Context_stack_pin(ctx, fnv);
	return FnObj_body(fnv);
}

static Object *eval_dispatch(const Node *expr, Context *ctx, Object *env)
{
	for (;;) {
		GC_collect(ctx->gc, env, ctx->stack);
		switch (expr->type) {
			case NUMBER_NODE:
				return GC_alloc_number(ctx->gc, NumNode_value(expr));
			case FN_NODE:
				return GC_alloc_fn(ctx->gc, env, FnNode_body(expr), FnNode_param_value(expr));
			case ID_NODE:
				return eval_lookup(expr, env);
			case NEG_NODE:
				return eval_neg(expr, ctx, env);
			case EXPT_NODE:
			case PRODUCT_NODE:
			case SUM_NODE:
			case CMP_NODE:
				return eval_pair(expr, ctx, env);
			case AND_NODE:
				return eval_and(expr, ctx, env);
			case OR_NODE:
				return eval_or(expr, ctx, env);
			case IF_NODE:
				expr = eval_if(ctx, &env, expr);
				if (!expr) {
					return NULL;
				}
				break;
			case APPLICATION_NODE:
				expr = eval_application(ctx, &env, expr);
				if (!expr) {
					return NULL;
				}
				break;
			case LET_NODE:
				return eval_let(expr, ctx, env);
		}
	}
}

static Object *force(Object *obj, Context *ctx)
{
	if (!obj || obj->type != THUNK_OBJECT) {
		return obj;
	}
	if (ThunkObj_value(obj)) {
		return ThunkObj_value(obj);
	}
	Context_stack_push(ctx, obj);
	Object *value = actual_value(ThunkObj_body(obj), ctx, ThunkObj_env(obj));
	ThunkObj_set_value(obj, value);
	if (!value) {
		return NULL;
	}
	Context_stack_pop(ctx);
	return value;
}

static Object *actual_value(const Node *expr, Context *ctx, Object *env)
{
	return force(eval_dispatch(expr, ctx, env), ctx);
}

Object *eval(const Node *expr, Context *ctx)
{
	Stack_clear(Context_stack(ctx));
	Context_stack_push(ctx, ctx->root);
	if (lazy) {
		return actual_value(expr, ctx, ctx->root);
	} else {
		return eval_dispatch(expr, ctx, ctx->root);
	}
}
