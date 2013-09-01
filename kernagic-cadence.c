#include "kernagic.h"
#include "kerner.h"

typedef struct Cadence
{
  int left;
  char *utf8;
  int right;
  int flags;
} Cadence;

#define LEFT_EXTREME    (1<<0)
#define RIGHT_EXTREME   (1<<1)
#define RIGHT_STEM      (1<<2)
#define LEFT_STEM       (1<<3)
#define BOTH_EXTREME    (LEFT_EXTREME | RIGHT_EXTREME)
#define BOTH_STEM       (LEFT_STEM | RIGHT_STEM)

/* cadence unit table; from lettermodel.org august 2013, manually typed
 * from image by Øyvind Kolås */
Cadence cadence[]={
{1,"A",1,BOTH_EXTREME},
{8,"B",3,LEFT_STEM | RIGHT_EXTREME},
{3,"C",2,BOTH_EXTREME},
{8,"D",3,LEFT_STEM | RIGHT_EXTREME},
{8,"E",2,LEFT_STEM | RIGHT_EXTREME},
{8,"F",2,LEFT_STEM | RIGHT_EXTREME},
{3,"G",6,LEFT_EXTREME | RIGHT_STEM},
{8,"H",8,BOTH_STEM},
{6,"J",6,BOTH_STEM}, /* MIGHT BE TOO LOW FOR LEFT STEM */
{8,"K",1,LEFT_STEM | RIGHT_EXTREME},
{8,"L",2,LEFT_STEM | RIGHT_EXTREME},
{8,"M",8,BOTH_STEM},
{8,"N",8,BOTH_STEM},
{3,"O",3,BOTH_EXTREME},
{8,"P",2,LEFT_STEM | RIGHT_EXTREME},
{3,"Q",3,BOTH_EXTREME},
{8,"R",1,LEFT_STEM | RIGHT_EXTREME},
{3,"S",3,BOTH_EXTREME},
{1,"T",1,BOTH_EXTREME},
{6,"U",6,BOTH_STEM},
{1,"V",1,BOTH_EXTREME},
{1,"W",1,BOTH_EXTREME},
{1,"X",1,BOTH_EXTREME},
{1,"Y",1,BOTH_EXTREME},
{2,"Z",2,BOTH_EXTREME},
{2,"a",6,BOTH_EXTREME| RIGHT_STEM},
{5,"b",2,LEFT_STEM | RIGHT_EXTREME},
{2,"c",1,BOTH_EXTREME},
{2,"d",6,BOTH_EXTREME | RIGHT_STEM},
{2,"e",1,BOTH_EXTREME},
{6,"f",1,LEFT_STEM | RIGHT_EXTREME},
{2,"g",1,BOTH_EXTREME},
{6,"h",6,BOTH_STEM},
{6,"i",6,BOTH_STEM},
{6,"j",5,BOTH_STEM},
{6,"k",1,BOTH_STEM}, /* stem tricky ? */
{6,"l",6,BOTH_STEM},
{6,"m",6,BOTH_STEM},
{6,"n",6,BOTH_STEM},
{2,"o",2,BOTH_EXTREME},
{6,"p",2,LEFT_STEM | RIGHT_EXTREME},
{2,"q",5,LEFT_EXTREME| RIGHT_STEM},
{6,"r",1,LEFT_STEM | RIGHT_EXTREME},
{3,"s",3,BOTH_EXTREME},
{5,"t",1,LEFT_STEM | RIGHT_EXTREME},
{5,"u",5,BOTH_STEM},
{1,"v",1,BOTH_EXTREME},
{1,"w",1,BOTH_EXTREME},
{1,"x",1,BOTH_EXTREME},
{1,"y",1,BOTH_EXTREME},
{2,"z",2,BOTH_EXTREME},
{3,".",3,BOTH_EXTREME},
{3,",",3,BOTH_EXTREME}};

extern float scale_factor;

float left_most_center (Glyph *g)
{
  return g->leftmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

float right_most_center (Glyph *g)
{
  return g->rightmost[(int)(kernagic_x_height() * 1.5 * scale_factor)] / scale_factor;
}

void kernagic_cadence_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  printf ("n-width: %f\n", g->ink_max_x - g->ink_min_x);
  printf ("n-width: %f %f\n", g->ink_max_x, g->ink_min_x);
  printf ("       : %f %f\n", left_most_center(g), right_most_center(g));
  //return (g->max_y - g->min_y);
}

void kernagic_cadence_each (Glyph *lg, GtkProgressBar *progress)
{
  printf ("%s\n", lg->name);
  kernagic_set_left_bearing (lg, 100);
  kernagic_set_right_bearing (lg, 100);
}



