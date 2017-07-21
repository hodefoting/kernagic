/* implementation of frank bloklands reinaissance spacing scheme */

#include <string.h>
#include <stdlib.h>
#include "kernagic.h"

typedef struct Cadence
{
  int   left;
  char *utf8;
  int   right;
  int   flags;
} Cadence;

#define LEFT_EXTREME    (1<<0)
#define RIGHT_EXTREME   (1<<1)
#define RIGHT_STEM      (1<<2)
#define LEFT_STEM       (1<<3)
#define BOTH_EXTREME    (LEFT_EXTREME | RIGHT_EXTREME)
#define BOTH_STEM       (LEFT_STEM | RIGHT_STEM)

#define MAX_GLYPHS 4096

float n_basis = 24;


enum {
  S_E_CHAR = 0,
  S_IN_CHAR,
  S_IN_COMMENT,
  S_E_LEFT_MODE,
  S_IN_LEFT_MODE,
  S_E_LEFT_VAL,
  S_IN_LEFT_VAL,
  S_E_RIGHT_MODE,
  S_IN_RIGHT_MODE,
  S_E_RIGHT_VAL,
  S_IN_RIGHT_VAL,
  S_IN_BASIS,
  S_DONE
};

extern float scale_factor;

float left_most_center (Glyph *g)
{
  /* we only look at a single scan-line in the middle of the x-height */
  return g->leftmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

float right_most_center (Glyph *g)
{
  /* we only look at a single scan-line in the middle of the x-height */
  return g->rightmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

#define LEFT_EXTREME    (1<<0)
#define RIGHT_EXTREME   (1<<1)
#define RIGHT_STEM      (1<<2)
#define LEFT_STEM       (1<<3)
#define BOTH_EXTREME    (LEFT_EXTREME | RIGHT_EXTREME)
#define BOTH_STEM       (LEFT_STEM | RIGHT_STEM)

