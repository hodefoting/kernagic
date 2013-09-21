                                                                            /*
This file is part of Kernagic a font spacing tool.
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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#define MAX_WORDS 500

int attempts = 500;


static int print_score = 0;

/** score_string:
 * @str              input string
 * @desired_glyphs   string containing highly desired glyphs
 *
 * Generates an ipsum with an attempt at maximizing the amount of differing
 * neighbour pairs.
 *
 * Returns the score for a string, a higher score is a better string.
 */
static int score_string (const char *str,
                         const char *desired_glyphs)
{
  /* we pick slightly larger than a power of two, to aovid aliasing of things
   * starting on multiples of 512 in the unicode set.
   */
#define ADJ_DIM   513
  gunichar *ustr;
  char adjacency_matrix[ADJ_DIM*ADJ_DIM]={0,};
  gunichar *p;

  if (!str || str[0] == 0)
    return 0;

  ustr = g_utf8_to_ucs4 (str, -1, NULL, NULL, NULL);

  if (!ustr)
    return 0;

  /* walk throguh the string ..*/
  for (p = ustr; p[1]; p++)
    {
      gunichar x = p[0]; /* .. using the current .. */
      gunichar y = p[1]; /* .. and the next characters unicode position ..*/

      if (x==' ' || y == ' ') 
        continue;  /* (bailing if one of them is a space) */

      x %= ADJ_DIM; /* with unicode positions wrapped down to our */
      y %= ADJ_DIM; /*  matrix dimensions */

      /* mark cell in matrix as visited */
      adjacency_matrix[y * ADJ_DIM + x] = 1;
    }

  /* count number of distinct pairs encountered (permitting some collisions,
   * in a bloom-filter like manner) */
  {
    int i;
    int sum = 0;

    if (desired_glyphs)
    for (i = 0; ustr[i]; i++)
      {
        int j;
        for (j = 0; desired_glyphs[j]; j++)
          if (desired_glyphs[j] == ustr[i])
            sum ++;
      }

    for (i = 0; i < ADJ_DIM * ADJ_DIM ; i ++)
      sum += adjacency_matrix[i] * 2;
      
    g_free (ustr);
    return sum;
  }
}

/* XXX: make this code utf8 */

char *ipsumat_generate (const char *dict_path,
                        const char *charset,
                        const char *desired_glyphs,
                        int         max_wordlen,
                        int         max_words)
{
  char    *p;
  int      count = 0;
  int      best_sentence[MAX_WORDS]={0,};
  int      sentence[MAX_WORDS]={0,};
  char    *words_str = NULL;
  GList   *words = NULL;
  GString *word = g_string_new ("");
  int      best_score = 0;
  int      i;
  if (!dict_path)
    dict_path = "/usr/share/dict/words";

  g_file_get_contents (dict_path, &words_str, NULL, NULL);

  if (!words_str)
    return g_strdup ("problem opening dictionary");

  if (charset == NULL)
    charset = "abcdefghijklmnopqrstuvwxyz";

  if (max_words > MAX_WORDS)
    max_words = MAX_WORDS;

  for (p = words_str; *p; p++)
    {
      switch (*p)
      {
        case '\n':
        case ' ':
          if (word->len)
          {
            int skip = 0;
            int i;
            for (i = 0; word->str[i]; i++)
              {
                int k;
                skip++;
                for (k = 0; charset[k]; k++)
                  if (charset[k]==word->str[i])
                    {
                      skip--;break;
                    }
              }
            if (word->len > max_wordlen)
              skip++;

            if (!skip)
            {
              words = g_list_prepend (words, g_strdup (word->str));
              count ++;
            }
          }
          g_string_assign (word, "");
          break;
        default:
          g_string_append_unichar (word, tolower (*p));
          break;
      }
    }
  g_free (words_str);


  for (i = 0; i < attempts; i ++)
    {
      GString *example = g_string_new ("");
      int j;
      for (j = 0; j < max_words; j ++)
        {
          int n;
          const char *str;
          n = random()%count;
 
          {
            int k;
            for (k = 0; k < j; k++)
              if (sentence[k]==n)
                {
                  /* we try once more if it collides with already picked
                   * random number,. - but this value will stick */
                  n = random()%count;
                  break;
                }
          }
          sentence[j] = n;
          
          str = g_list_nth_data (words, n);
          if (str)
          {
            if (j)
              g_string_append (example, " ");
            g_string_append (example, str);
          }
        }
      float score = score_string ((void*)example->str, desired_glyphs);
      if (score >= best_score)
      {
        for (j = 0; j < max_words; j ++)
          best_sentence[j] = sentence[j];
        best_score = score;
      }
      g_string_free (example, TRUE);
    }

  if (print_score)
    printf ("Score: %i\n", best_score);

  {
    char *ret = NULL;
    int j;
    GString *s = g_string_new ("");

    if (desired_glyphs && desired_glyphs[0])
      {
        g_string_append (s, desired_glyphs);
        g_string_append (s, " ");
      }

    for (j = 0; j < max_words; j ++)
    {
      const char *str;
      str = g_list_nth_data (words, best_sentence[j]);
      if (str)
        {
          if (j)
            g_string_append (s, " ");
          g_string_append (s, str);
        }
    }
    ret = strdup (s->str);
    g_string_free (s, TRUE);
    return ret;
  }
}

/* the main used - when executable is named/symlinked ipsum */
int ipsumat (int argc, char **argv)
{
  int argno;
  int         iterations = 1;
  int         max_wordlen = 7;
  int         wordcount = 23;
  const char *glyphs = "abcdefghijklmnopqrstuvwxyz";
  const char *desired_glyphs = "";
  const char *dict_path = NULL;

  /* XXX: should initialize seed from time? */
  struct timeval time;
  gettimeofday (&time, NULL);
  srandom (time.tv_sec);

  for (argno = 1; argno < argc; argno++)
    {
      if (!strcmp (argv[argno], "--help") ||
          !strcmp (argv[argno], "-h"))
        {
          fprintf (stdout, "\n"
              "ipsumat [options]\n"
              " -m <maxwordlength, default = 7>\n"
              " -w <number of words, default = 23>\n"
              " -i <number of iterations>\n"
              " -s <seed for random generator - defaults to time of day>\n"
              " -g <string of permitted glyphs>\n"
              " -d <string of highly desired glyphs>\n"
              " -D <path to dictionary file>\n"
              " -p print score\n"
              "\n");
          return 0;
        }
      else if (!strcmp (argv[argno], "-w"))
      {
#define EXPECT_ARG if (!argv[argno+1]) {fprintf (stderr, "expected argument        after %s\n", argv[argno]);exit(-1);}
        EXPECT_ARG;
        wordcount = atoi (argv[++argno]);
      }
      else if (!strcmp (argv[argno], "-m"))
      {
        EXPECT_ARG;
        max_wordlen = atoi (argv[++argno]);
      }
      else if (!strcmp (argv[argno], "-g"))
      {
        EXPECT_ARG;
        glyphs = argv[++argno];
      }
      else if (!strcmp (argv[argno], "-s"))
      {
        EXPECT_ARG;
        srandom (atoi(argv[++argno]));
      }
      else if (!strcmp (argv[argno], "-D"))
      {
        EXPECT_ARG;
        dict_path = argv[++argno];
      }
      else if (!strcmp (argv[argno], "-d"))
      {
        EXPECT_ARG;
        desired_glyphs = argv[++argno];
      }
      else if (!strcmp (argv[argno], "-p"))
      {
        print_score = 1;
      }
      else if (!strcmp (argv[argno], "-i"))
      {
        EXPECT_ARG;
        iterations = atoi (argv[++argno]);
      }
      else if (!strcmp (argv[argno], "-a"))
      {
        EXPECT_ARG;
        attempts = atoi (argv[++argno]);
      }
    }

  int i;
  for (i = 0; i < iterations; i ++)
  printf ("%s\n",
      ipsumat_generate (dict_path,
                        glyphs,
                        desired_glyphs,
                        max_wordlen,
                        wordcount));
  return 0;
}
