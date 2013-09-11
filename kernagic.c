                                                                            /*
Kernagic a libre auto spacer and kerner for Unified Font Objects.
Copyright (C) 2013 Øyvind Kolås

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.       */

#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include "kernagic.h"
#include "kerner.h"

#define SPECIMEN_SIZE 60

char *kernagic_sample_text = NULL;

static char *loaded_ufo_path = NULL;
static GList *glyphs = NULL;
float  scale_factor = 0.18;
static gunichar *glyph_string = NULL;

extern KernagicMethod *kernagic_cadence,
                      *kernagic_rythm,
                      *kernagic_gray,
                      *kernagic_bounds;

KernagicMethod *methods[32] = {NULL};

static void init_methods (void)
{
  int i = 0;
  methods[i++] = kernagic_bounds;
  methods[i++] = kernagic_gray;
  methods[i++] = kernagic_cadence;
  methods[i++] = kernagic_rythm;
  methods[i] = NULL;
};


gboolean kernagic_strip_bearing = FALSE;

GList *kernagic_glyphs (void)
{
  return glyphs;
}

void init_kernagic (void)
{
  init_kerner ();
}


static int
add_glyph(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
  if (strstr (fpath, "contents.plist"))
    return 0;
  Glyph *glyph = kernagic_glyph_new (fpath);

  if (glyph)
    glyphs = g_list_prepend (glyphs, glyph);
  return 0;
}

/* simplistic recomputation of right bearings; it uses the average advance
 * taking all kerning pairs into account, and modifies all the involved
 * kerning pairs accordingly.
 */
void
recompute_right_bearings ()
{
  GList *l;
  for (l = glyphs; l; l= l->next)
    {
      Glyph *lg = l->data;
      GList *r;
      float advance_sum = 0;
      long  glyph_count = 0;
      int   new_advance;
      int   advance_diff;
      for (r = glyphs; r; r= r->next)
        {
          Glyph *rg = r->data;
          advance_sum += kernagic_kern_get (lg, rg) + kernagic_get_advance (lg);
          glyph_count ++;
        }
      new_advance = advance_sum / glyph_count;
      advance_diff = new_advance - kernagic_get_advance (lg);

      lg->left_bearing  = 0;
      lg->right_bearing = new_advance;

      for (r = glyphs; r; r= r->next)
        {
          Glyph *rg = r->data;
          float oldkern = kernagic_kern_get (lg, rg);
          kernagic_set_kerning (lg, rg, oldkern - advance_diff);
        }
    }
}

void kernagic_save_kerning_info (void)
{
  GString *str = g_string_new (
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\"\n"
"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"  <dict>\n");

  GList *left, *right;

  /* XXX: this likely is not desired for all methods!! */
  recompute_right_bearings ();

  for (left = glyphs; left; left = left->next)
  {
    Glyph *lg = left->data;
    int found = 0;

    for (right = glyphs; right; right = right->next)
      {
        Glyph *rg = right->data;
        float kerning;

        if ((kerning=kernagic_kern_get (lg, rg)) != 0.0)
          {
            if (!found)
              {
    g_string_append_printf (str, "    <key>%s</key>\n    <dict>\n", lg->name);
    found = 1;
              }
      
            g_string_append_printf (str, "      <key>%s</key><integer>%d</integer>\n", rg->name, (int)kerning);

          }
      }
    if (found)
      g_string_append (str, "    </dict>\n");
  }

  g_string_append (str, "  </dict>\n</plist>");

  char path[4095];
  GError *error = NULL;

  sprintf (path, "%s/kerning.plist", loaded_ufo_path);

  g_file_set_contents (path, str->str, -1, &error);
  if (error)
    fprintf (stderr, "EEeek %s\n", error->message);
  g_string_free (str, TRUE);

  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      rewrite_ufo_glyph (glyph);
    }
}

void render_ufo_glyph (Glyph *glyph);

void kernagic_load_ufo (const char *font_path, gboolean strip_left_bearing)
{
  char path[4095];

  kernagic_strip_bearing = strip_left_bearing;

  if (loaded_ufo_path)
    g_free (loaded_ufo_path);
  loaded_ufo_path = g_strdup (font_path);
  if (loaded_ufo_path [strlen(loaded_ufo_path)] == '/')
    loaded_ufo_path [strlen(loaded_ufo_path)] = '\0';

  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      kernagic_glyph_free (glyph);
    }
  g_list_free (glyphs);
  glyphs = NULL;

  sprintf (path, "%s/glyphs", loaded_ufo_path);

  if (nftw(path, add_glyph, 20, 0) == -1)
    {
      fprintf (stderr, "EEEeeek! '%s' probably not a ufo dir\n", loaded_ufo_path);
    }

  scale_factor = SPECIMEN_SIZE / kernagic_x_height ();

  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      int y;
      render_ufo_glyph (glyph);

      for (y = 0; y < glyph->r_height; y ++)
      {
        int x;
        int min = 8192;
        int max = -1;
        for (x = 0; x < glyph->r_width; x ++)
          {
            if (glyph->raster[y * glyph->r_width + x] > 0)
              {
                if (x < min)
                  min = x;
                if (x > max)
                  max = x;
              }
          }
        glyph->leftmost[y] = min;
        glyph->rightmost[y] = max;
      }
    }
  init_kernagic ();
}

void kernagic_kern_clear_all (void)
{
  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      g_hash_table_remove_all (glyph->kerning);
    }
}

Glyph *kernagic_find_glyph_unicode (unsigned int unicode)
{
  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      if (glyph->unicode == unicode)
        return glyph;
    }
  return NULL;
}

/* programmatic way of finding x-height, is guaranteed to work better than
 * font metadata...
 */
float kernagic_x_height (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('x');
  if (!g)
    return 0.0;
  return (g->ink_max_y - g->ink_min_y);
}

gboolean kernagic_deal_with_glyph (gunichar unicode)
{
  int i;
  if (!glyph_string)
    return TRUE;
  for (i = 0; glyph_string[i]; i++)
    if (glyph_string[i] == unicode)
      return TRUE;
  return FALSE;
}

gboolean kernagic_deal_with_glyphs (gunichar unicode, gunichar unicode2)
{
  int i;
  if (!glyph_string)
    return TRUE;
  if (!unicode || !unicode2)
    return FALSE;
  for (i = 0; glyph_string[i]; i++)
    if (glyph_string[i] == unicode &&
        glyph_string[i+1] == unicode2)
      return TRUE;
  return FALSE;
}

void   kernagic_set_glyph_string (const char *utf8)
{
  if (glyph_string)
    g_free (glyph_string);
  glyph_string = NULL;
  if (utf8)
    glyph_string = g_utf8_to_ucs4 (utf8, -1, NULL, NULL, NULL);
}

/* XXX: support creating a new ufo, instead of overwriting the old one..
 *      could be added by making an initial copy?
 */

static int interactive = 1;

gboolean kernagic_strip_left_bearing = KERNAGIC_DEFAULT_STRIP_LEFT_BEARING;
char *kernagic_output = NULL;
char *kernagic_sample_string = NULL;
char *kernagic_output_png = NULL;

void help (void)
{
  printf ("kernagic [options] <font.ufo>\n"
          "\n"
          "Options:\n"
          "   -m <method>   specify method, one of gray, cadence and rythmic "
          "   suboptions influencing x-height gray:\n"
          "       -d <0..100>   minimum distance default = %i\n"
          "       -D <0..100>   maximum distance default = %i\n"
          "       -t <0..100>   target gray value default = %i\n"
     //     "       -l            don't strip left bearing\n"
     //     "       -L            strip left bearing (default)\n"
          "\n"
          "   -o <output.ufo>  create a copy of the input font, without it kernagic overwrites the input ufo\n"
   /*     "   -s <string>      sample string (for use with -p)\n"
          "   -p <output.png>   render sample string with settings (do not change font)\n" */
          "\n"

 //         "Examples:\n"
    //      "    Use the cadence unit method on Test.ufo, overwriting file\n"
    //      "      kernagic -c Test.ufo\n"
   /*     "    Preview default settings on string shoplift to test.png\n"
          "      kernagic Original.ufo -s 'shoplift' -p test.png\n"
          "    Run gray with a different gray target, still previewing to test.png\n"
          "      kernagic Original.ufo -s 'shoplift' -p test.png -t 0.4\n"
          "    Write a final adjusted font to Output.ufo\n" */
//          "      kernagic Original.ufo -s 'shoplift' -o Output.ufo -t 0.32 -m 10 -M 30\n"
          ,
    KERNER_DEFAULT_MIN,
    KERNER_DEFAULT_MAX,
    KERNER_DEFAULT_TARGET_GRAY
          );
  exit (0);
}

const char *ufo_path = NULL;

void parse_args (int argc, char **argv)
{
  int no;
  for (no = 1; no < argc; no++)
    {
      if (!strcmp (argv[no], "--help") ||
          !strcmp (argv[no], "-h"))
        help ();
      else if (!strcmp (argv[no], "-d"))
        {
#define EXPECT_ARG if (!argv[no+1]) {fprintf (stderr, "expected argument after %s\n", argv[no]);exit(-1);}

          EXPECT_ARG;
          kerner_settings.minimum_distance = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-D"))
        {
          EXPECT_ARG;
          kerner_settings.maximum_distance = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-t"))
        {
          EXPECT_ARG;
          kerner_settings.alpha_target = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-m"))
        {
          int i;
          char *method;
          int found = 0;
          EXPECT_ARG;
          method = argv[++no];
          for (i = 0; methods[i]; i++)
            {
              if (!strcmp (method, methods[i]->name))
                {
                  kerner_settings.method = methods[i];
                  found = 1;
                  break;
                }
            }
          if (!found)
            {
              fprintf (stderr, "unknown method %s\n", method);
              exit (-1);
            }
        }
      else if (!strcmp (argv[no], "-l"))
        {
          kernagic_strip_left_bearing = 0;
        }
      else if (!strcmp (argv[no], "-L"))
        {
          kernagic_strip_left_bearing = 1;
        }
      else if (!strcmp (argv[no], "-T"))
      {
        EXPECT_ARG;
        kernagic_sample_text = argv[++no];
      }
      else if (!strcmp (argv[no], "-o"))
      {
        EXPECT_ARG;
        kernagic_output = argv[++no];
        interactive = 0;

        if (!ufo_path)
          {
            fprintf (stderr, "no font file to work on specified before -o\n");
            exit (-1);
          }

        char cmd[512];
        /* XXX: does not work on windows */
        sprintf (cmd, "rm -rf %s;cp -Rva %s %s", kernagic_output, ufo_path, kernagic_output);
        fprintf (stderr, "%s\n", cmd);
        system (cmd);
        ufo_path = kernagic_output;
      }
      else if (!strcmp (argv[no], "-p"))
      {
        EXPECT_ARG;
        kernagic_output_png = argv[++no];
        interactive = 0;
      }
      else if (!strcmp (argv[no], "-s"))
      {
        EXPECT_ARG;
        kernagic_sample_string = argv[++no];
      }
      else if (argv[no][0] == '-')
        {
          fprintf (stderr, "unknown argument %s\n", argv[no]);
          exit (-1);
        }
      else
        {
          ufo_path = argv[no];
        }
    }
}

int kernagic_gtk (int argc, char **argv);
KernagicMethod *kernagic_method_no (int no)
{
  if (no < 0) no = 0;
  return methods[no];
}

int             kernagic_find_method_no (KernagicMethod *method)
{
  int i;
  for (i = 0; methods[i]; i++)
    if (methods[i] == method)
      return i;
  return 0;
}

int kernagic_active_method_no (void)
{
  return kernagic_find_method_no (kerner_settings.method);
}

int main (int argc, char **argv)
{
  init_methods ();
  parse_args (argc, argv);

  if (interactive)
    return kernagic_gtk (argc, argv);

  if (!ufo_path)
    {
      fprintf (stderr, "no font file to work on specified\n");
      exit (-1);
    }

  kernagic_load_ufo (ufo_path, kernagic_strip_left_bearing);

  kernagic_set_glyph_string (NULL);
  fprintf (stderr, "computing!\n");
  kernagic_compute (NULL);
  fprintf (stderr, "done fitting!\n");
  fprintf (stderr, "saving\n");
  kernagic_save_kerning_info ();
  fprintf (stderr, "done saving!\n");

  return 0;
}

void kernagic_compute (GtkProgressBar *progress)
{
  GList *glyphs = kernagic_glyphs ();
  long int total = g_list_length (glyphs);
  long int count = 0;
  GList *left;

  if (kerner_settings.method->init)
    kerner_settings.method->init ();

  for (left = glyphs; left; left = left->next)
  {
    Glyph *lg = left->data;
    if (progress)
    {
      float fraction = count / (float)total;
      gtk_progress_bar_set_fraction (progress, fraction);
      gtk_main_iteration_do (FALSE);
    }

    kernagic_glyph_reset (lg);
  
    if (progress || kernagic_deal_with_glyph (lg->unicode))
    {
      if (kerner_settings.method->each)
        kerner_settings.method->each (lg, progress);
    }
    count ++;
  }

  if (kerner_settings.method->done)
    kerner_settings.method->done ();
}
