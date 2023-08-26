#include "object.h"

#include <stdio.h>


void Object_println(Object *obj)
{
	switch (obj->type) {
	case NUM_OBJECT:
		printf("%lf\n", obj->as.num);
		return;
	case FN_OBJECT:
		printf("<fn>\n");
		return;
	case ENV_OBJECT:
		printf("<env>\n");
		return;
	}
}
