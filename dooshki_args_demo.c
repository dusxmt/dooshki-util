/*
 * Copyright (c) 2020 Marek Benc <dusxmt@gmx.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dooshki_args.h"

#define PROG_NAME    "dooshki_args_demo"
#define PROG_VERSION "0.1"
#define PROG_USAGE   "[OPTIONS] [FILE1 [FILE2 [...]]]"
#define PROG_SUMMARY "Dooshki's demo for his CLI arguments library"

#define PROG_DESCRIPTION \
                         \
"This program serves as a simple example of Dooshki's command line argument\n" \
"parsing library.\n" \
"\n" \
"The library is designed to be easily integrated into any small project\n" \
"written in the C programming language.  It has a small code footprint,\n" \
"and works in any environment which supports ANSI C89.\n" \
"\n" \
"This program tests all of the supported command line option types, including\n" \
"a custom type (by the use of a callback function), and displays the collected\n" \
"information.\n"


static const char *program_name = PROG_NAME;

/*
 * Case insensitive string comparison (returns 1 on match, 0 on mismatch).
 *
 * Since strcasecmp() is a POSIX extension, and this demo targets ANSI C89,
 * an alternative is provided.
 */
char streq_ci(const char *in_a, const char *in_b)
{
    size_t iter;

    for (iter = 0; in_a[iter] != '\0' && in_b[iter] != '\0' &&
                   tolower(in_a[iter]) == tolower(in_b[iter]); iter++);

    return (in_a[iter] == in_b[iter] && in_a[iter] == '\0')? 1 : 0;
}

/* Example of a custom type and argument processing function for it. */
enum projectile_quality
{
    QUALITY_GOOD,
    QUALITY_BAD,
    QUALITY_UGLY
};

char quality_arg_decode(const char *argument_text, void *opt_storage,
                        const char *opt_prefix, const char *opt_name,
                        void *callback_data)
{
    enum projectile_quality *dest = opt_storage;

    /* This routine doesn't need any callback context data. */
    (void)callback_data;

    if (streq_ci(argument_text, "good"))
    {
        *dest = QUALITY_GOOD;
        return 1;
    }
    else if (streq_ci(argument_text, "bad"))
    {
        *dest = QUALITY_BAD;
        return 1;
    }
    else if (streq_ci(argument_text, "ugly"))
    {
        *dest = QUALITY_UGLY;
        return 1;
    }
    else
    {
        fprintf(stderr,
                "%s: Argument `%s' passed to option %s%s is not a valid "
                "quality specifier (allowed values: good, bad, ugly).\n",
                program_name, argument_text, opt_prefix, opt_name);

        return 0;
    }
}

/* Values retrieved from the command line. */
static char automatic = 0;
static char automatic_opt = 0;
static char manual_opt = 0;

const char *label = NULL;

static long direction = 0;
static char direction_set = 0;

static unsigned long velocity = 0;
static char velocity_set = 0;

static double rating = 0.0;
static char rating_set = 0;

static enum projectile_quality quality = QUALITY_GOOD;
static char quality_set = 0;


/*
 * Option definitions.
 *
 * Various fields are left empty for the sake of demonstration.  Of course,
 * for a pleasant appearence, you should instead aim for consistency.
 */
static const struct dooshki_opt cli_options[] =
{
    { "a", "automatic", NULL, DOOSHKI_OPT_BOOL, &automatic, &automatic_opt,
      "Perform the requested action automatically.", NULL, NULL },

    { "m", NULL, NULL, DOOSHKI_OPT_NEGBOOL, &automatic, &manual_opt,
      "Perform the requested action manually.  This option has an intentionally"
      " long description, as to show the line-wrapping support.", NULL, NULL },

    { "l", "label", "NAME", DOOSHKI_OPT_STR, &label, NULL,
      "Label to display.", NULL, NULL },

    { "r", "rating", "RATING", DOOSHKI_OPT_FLOAT, &rating, &rating_set,
      NULL, NULL, NULL },

    { NULL, "direction", "DIR", DOOSHKI_OPT_INT, &direction, &direction_set,
      "Projectile direction.", NULL, NULL },

    { "v", NULL, "VEL", DOOSHKI_OPT_UINT, &velocity, &velocity_set,
      "Projectile velocity.", NULL, NULL },

    { "q", "quality", "GOOD|BAD|UGLY", DOOSHKI_OPT_CB, &quality, &quality_set,
      "Quality of the projectiles to be used.", quality_arg_decode, NULL },

    { NULL }
};

/* Arguments context, contains information needed for --help and --version. */
static struct dooshki_args cli_args_context =
{
    PROG_NAME,
    PROG_VERSION,
    PROG_USAGE,
    PROG_SUMMARY,
    PROG_DESCRIPTION,

    cli_options
};

#if 0
static void list_argv(int argc, char **argv)
{
    int iter;

    printf("argv[%d] = { ", argc);

    for(iter = 0; iter < argc; iter++)
    {
        if (iter > 0)
            printf(", ");

        if (argv[iter] != NULL)
            printf("\"%s\"", argv[iter]);

        else
            printf("NULL");
        
    }
    printf(" };\n");
}
#endif

int main(int argc, char **argv)
{
    enum dooshki_args_ret arg_parse_ret;

    /* list_argv(argc, argv); */
    arg_parse_ret = dooshki_args_parse(&argc, &argv, &cli_args_context);
    /* list_argv(argc, argv); */

    switch(arg_parse_ret)
    {
        case DOOSHKI_ARGS_PARSE_OK:
            break;

        case DOOSHKI_ARGS_HELP_SHOWN:
            return 0;
            break;

        case DOOSHKI_ARGS_VER_SHOWN:
            return 0;
            break;

        case DOOSHKI_ARGS_PARSE_ERROR:
            return 1;
            break;

        default:
            fprintf(stderr,
                    "%s: Unexpected return value %u from dooshki_args_parse().\n",
                    program_name, (unsigned int)arg_parse_ret);
            return 1;
    }

    printf("The following information was retrieved from the command line:\n");

    printf("    Label:          %s\n", (label != NULL)? label : "unspecified");
    printf("    Operation type: %s (manual opt: %s, automatic opt: %s)\n",
           automatic?      "automatic" : "manual",
           automatic_opt?  "yes" : "no",
           manual_opt?     "yes" : "no");

    printf("    Direction:      ");
    if (direction_set)
        printf("%ld\n", direction);
    else
        printf("%s\n", "unspecified");

    printf("    Velocity:       ");
    if (velocity_set)
        printf("%lu\n", velocity);
    else
        printf("%s\n", "unspecified");

    printf("    Rating:         ");
    if (rating_set)
        printf("%.6g\n", rating);
    else
        printf("%s\n", "unspecified");

    printf("    Quality:        ");
    if (quality_set)
    {
        const char *quality_str;
        switch (quality)
        {
            case QUALITY_GOOD:
                quality_str = "good";
                break;

            case QUALITY_BAD:
                quality_str = "bad";
                break;

            case QUALITY_UGLY:
                quality_str = "ugly";
                break;

            default:
                quality_str = "unknown";
                break;
        }
        printf("%s\n", quality_str);
    }
    else
        printf("%s\n", "unspecified");

    printf("\n"
           "If this program did anything, "
           "it would process the following files:\n");

    if (argc > 1)
    {
        int iter;

        for (iter = 1; iter < argc; iter++)
            printf("    \"%s\"\n", argv[iter]);
    }
    else
        printf("    none\n");

    return 0;
}
