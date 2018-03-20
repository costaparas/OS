#include "bar.h"

#ifndef ITEM_H
#define ITEM_H

typedef struct barorder *item;
typedef item key;

#define key(it) ((it))
#define del(it) ((it) = (it))
#define copy(x) ((x))
#define eq(x,y) ((x) == (y))
#define lt(x,y) ((x) < (y))
#define le(x,y) ((x) <= (y))
#define gt(x,y) ((x) > (y))
#define ge(x,y) ((x) >= (y))

#endif
