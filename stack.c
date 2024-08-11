#include "stack.h"

#include <stdlib.h>

#include "object.h"


Stack *Stack_new()
{
	Stack *self = malloc(sizeof(*self));
	self->size = 0;
	self->capacity = INITIAL_STACK_CAPACITY;
	self->objects = calloc(INITIAL_STACK_CAPACITY, sizeof(*self->objects));
	return self;
}

void Stack_drop(Stack *self)
{
	free(self->objects);
	free(self);
}

void Stack_push(Stack *self, Object *value)
{
	if (self->size >= self->capacity) {
		self->capacity *= 2;
		self->objects = reallocarray(self->objects, self->capacity, sizeof(*self->objects));
	}
	self->objects[self->size][0] = value;
	self->objects[self->size][1] = NULL;
	self->size += 1;
}

void Stack_pin(Stack *self, Object *value)
{
	self->objects[self->size-1][1] = value;
}

Object *Stack_pop(Stack *self)
{
	if (!self->size) {
		return NULL;
	}
	self->size -= 1;
	return self->objects[self->size][0];
}

void Stack_clear(Stack *self)
{
	self->size = 0;
}

void Stack_for_each(const Stack *self, void (*fn)(void *, Object *), void *param)
{
	for (int i = 0; i < self->size; i++) {
		fn(param, self->objects[i][0]);
		if (self->objects[i][1]) {
			fn(param, self->objects[i][1]);
		}
	}
}
