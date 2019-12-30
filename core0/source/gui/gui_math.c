/*
 * gui_math.c
 *
 *  Created on: 2019Äê12ÔÂ29ÈÕ
 *      Author: jace
 */


#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "math.h"
#include "fsl_powerquad.h"

float gui_sqrt(float __x)
{
    PQ_SqrtF32(&__x, &__x);
    return __x;
}

float gui_cos(float __x)
{
    PQ_CosF32(&__x, &__x);
    return __x;

}

float gui_sin(float __x)
{
    PQ_SinF32(&__x, &__x);
    return __x;
}

float gui_div(float __x,float __y)
{
    PQ_DivF32(&__x, &__y, &__x);
    return __x;
}
