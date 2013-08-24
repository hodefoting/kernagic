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

static char *loaded_ufo_path = NULL;

static GList *glyphs = NULL;
float  scale_factor = 0.18;
static gunichar *glyph_string = NULL;

gboolean kernagic_strip_bearing = FALSE;

void init_kernagic (void)
{
  init_kerner ();
}

void render_ufo_glyph (Glyph *glyph);

void glyph_free (Glyph *glyph)
{
  if (glyph->path)
    g_free (glyph->path);
  if (glyph->name)
    g_free (glyph->name);
  if (glyph->xml)
    g_free (glyph->xml);
  if (glyph->raster)
    g_free (glyph->raster);
  if (glyph->kerning)
    g_hash_table_destroy (glyph->kerning);
  g_free (glyph);
}

Glyph *nsd_glyph_new (const char *path)
{
  Glyph *glyph = g_malloc0 (sizeof (Glyph));
  g_file_get_contents (path, &glyph->xml, NULL, NULL);

  if (glyph->xml)
    {
      load_ufo_glyph (glyph);
    }
  else
    {
      g_free (glyph);
      glyph = NULL;
    }

  if (glyph)
    {
      /* skipping some glyphs */
      if (!glyph->name ||
          glyph->unicode == ' ')
        {
          g_free (glyph);
          glyph = NULL;
        }
    }

  if (glyph)
    {
      glyph->kerning = g_hash_table_new (g_direct_hash, g_direct_equal);
      glyph->path = g_strdup (path);
    }
  return glyph;
}

void nsd_glyph_free (Glyph *glyph)
{
  if (glyph->xml)
    g_free (glyph->xml);
  if (glyph->raster)
    free (glyph->raster);
  if (glyph->name)
    g_free (glyph->name);
  g_free (glyph);
}

static int
add_glyph(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
  if (strstr (fpath, "contents.plist"))
    return 0;
  Glyph *glyph = nsd_glyph_new (fpath);

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
          advance_sum += kernagic_kern_get (lg, rg) + lg->advance;
          glyph_count ++;
        }
      new_advance = advance_sum / glyph_count;
      advance_diff = new_advance - lg->advance;
      lg->advance = new_advance;
      for (r = glyphs; r; r= r->next)
        {
          Glyph *rg = r->data;
          float oldkern = kernagic_kern_get (lg, rg);
          kernagic_kern_set (lg, rg, oldkern - advance_diff);
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
      glyph_free (glyph);
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
      render_ufo_glyph (glyph);
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

void kernagic_kern_set (Glyph *a, Glyph *b, float kerning)
{
  int intkern = kerning * 10;
  if (!a || !b)
    return;

  g_hash_table_insert (a->kerning, b, GINT_TO_POINTER (intkern));
}

float kernagic_kern_get (Glyph *a, Glyph *b)
{
  int intkern;
  float kern;
  if (!a || !b)
    return 0;

  gpointer pvalue = g_hash_table_lookup (a->kerning, b);

  intkern = GPOINTER_TO_INT (pvalue);
  kern = intkern / 10.0;
  return kern;
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
  return (g->max_y - g->min_y);
}

static gboolean deal_with_glyph (gunichar unicode)
{
  int i;
  if (!glyph_string)
    return TRUE;
  for (i = 0; glyph_string[i]; i++)
    if (glyph_string[i] == unicode)
      return TRUE;
  return FALSE;
}

static gboolean deal_with_glyphs (gunichar unicode, gunichar unicode2)
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

void kernagic_combinatoric_each_left (Glyph *lg, GtkProgressBar *progress)
{
  GList *right;
  for (right = glyphs; right; right = right->next)
    {
      Glyph *rg = right->data;
      if (progress || deal_with_glyphs (lg->unicode, rg->unicode))
        {
          float kerned_advance = kerner_kern (&kerner_settings, lg, rg);
          kernagic_kern_set (lg, rg, kerned_advance - lg->advance);
        }
    }
}

void kernagic_cadence_each_left (Glyph *lg, GtkProgressBar *progress)
{
  printf ("%s\n", lg->name);
}


void kernagic_compute (GtkProgressBar *progress)
{
  long int total = g_list_length (glyphs);
  long int count = 0;
  GList *left;

  for (left = glyphs; left; left = left->next)
  {
    Glyph *lg = left->data;
    if (progress)
    {
      float fraction = count / (float)total;
      gtk_progress_bar_set_fraction (progress, fraction);
      gtk_main_iteration_do (FALSE);
    }
  
    if (progress || deal_with_glyph (lg->unicode))
    {
      switch (kerner_settings.mode)
      {
        case KERNAGIC_CADENCE:
          kernagic_cadence_each_left (lg, progress);
          break;
        case KERNAGIC_RYTHM:
          fprintf (stderr, "missing linear iterator\n");
          break;
        case KERNAGIC_GRAY:
        default:
          kernagic_combinatoric_each_left (lg, progress);
          break;
      }
    }
    count ++;
  }
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
//          "   -c use cadence units, try this for fonts with traditional proportions\n"
//          "   -g use gray level     (default)\n"
//          "      suboptions influencing gray:\n"
          "       -m <0..100>   minimum distance default = %i\n"
          "       -M <0..100>   maximum distance default = %i\n"
          "       -t <0..100>   target gray value default = %i\n"
     //     "       -l            don't strip left bearing\n"
     //     "       -L            strip left bearing (default)\n"
          "\n"
          "   -o <output.ufo>  create a copy of the input font, withoutit kernagic overwrites the input ufo\n"
   /*     "   -s <string>      sample string (for use with -p)\n"
          "   -p <output.png>   render sample string with settings (do not change font)\n" */
          "\n"

          "Examples:\n"
    //      "    Use the cadence unit method on Test.ufo, overwriting file\n"
    //      "      kernagic -c Test.ufo\n"
   /*     "    Preview default settings on string shoplift to test.png\n"
          "      kernagic Original.ufo -s 'shoplift' -p test.png\n"
          "    Run gray with a different gray target, still previewing to test.png\n"
          "      kernagic Original.ufo -s 'shoplift' -p test.png -t 0.4\n"
          "    Write a final adjusted font to Output.ufo\n" */
          "      kernagic Original.ufo -s 'shoplift' -o Output.ufo -t 0.32 -m 10 -M 30\n",
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
      else if (!strcmp (argv[no], "-m"))
        {
#define EXPECT_ARG if (!argv[no+1]) {fprintf (stderr, "expected argument after %s\n", argv[no]);exit(-1);}

          EXPECT_ARG;
          kerner_settings.minimum_distance = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-M"))
        {
          EXPECT_ARG;
          kerner_settings.maximum_distance = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-t"))
        {
          EXPECT_ARG;
          kerner_settings.alpha_target = atof (argv[++no]);
        }
      else if (!strcmp (argv[no], "-g"))
        {
          kerner_settings.mode = 0;
        }
      else if (!strcmp (argv[no], "-c"))
        {
          kerner_settings.mode = 1;
        }
      else if (!strcmp (argv[no], "-l"))
        {
          kernagic_strip_left_bearing = 0;
        }
      else if (!strcmp (argv[no], "-L"))
        {
          kernagic_strip_left_bearing = 1;
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

int main (int argc, char **argv)
{
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
