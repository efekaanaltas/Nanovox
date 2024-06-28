#pragma once

#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <assert.h>
#include <math.h>
#include <vector>

#include "types.h"

#define U32Max ((u32)-1)
#define F32Max FLT_MAX
#define F32Min -FLT_MAX
#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))