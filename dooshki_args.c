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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#include "dooshki_args.h"

/* Columns at which help screen entries are shown, feel free to tweak. */
#define SHORT_START_COL     2
#define LONG_START_COL      6
#define DESC_START_COL      28
#define PAGE_WRAP_COL       78

/* The hard-coded help and version entries. */
#define HELP_SHORT_OPT      'h'
#define HELP_SHORT_OPT_STR  "h"
#define HELP_LONG_OPT       "help"
#define HELP_DESC           "Display this help screen and quit."

#define VER_SHORT_OPT       'V'
#define VER_SHORT_OPT_STR   "V"
#define VER_LONG_OPT        "version"
#define VER_DESC            "Display the program's version and quit."


/* Convenience routine for printing error messages. */
static void print_error(const struct dooshki_args *args_ctxt,
                        const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "%s: ", args_ctxt->program_name);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}

/* Print usage information, along with a possible suggestion to use --help */
static void print_usage(const struct dooshki_args *args_ctxt, char is_error)
{
    if (is_error)
        fprintf(stderr, "\n");

    printf("%s %s - %s\n",
           args_ctxt->program_name,
           args_ctxt->version,
           args_ctxt->summary);

    printf("Usage:\n"
           "    %s %s\n\n", args_ctxt->program_name, args_ctxt->usage);

    if (is_error)
        printf("See `%s --%s' for more details.\n",
               args_ctxt->program_name, HELP_LONG_OPT);
}

/* Move to a specified column on the screen. */
static void set_column(unsigned int new_col, unsigned int *column,
                       char space_needed)
{
    unsigned int current_col = *column;

    if (new_col < current_col || (space_needed && new_col == current_col))
    {
        putchar('\n');
        current_col = 0;
    }

    while (current_col < new_col) {
        putchar(' ');
        current_col += 1;
    }

    *column = current_col;
}

/*
 * Locate an entry in a whitespace-separated list.
 *
 *
 * The search starts at the index w_start, and skips over a potential block
 * of whitespaces until it reaches the first non-whitespace character.
 *
 * A whitespace in this routine is defined as any character for which isspace()
 * returns a non-zero value.
 *
 * *w_start will refer to either '\0' (if no word was found) or the first
 * character of a word.
 *
 * *w_end will refer to the first character beyond the word, either
 * a whitespace or '\0'.  If no word was found, *w_end will have the same
 * value as *w_start.
 */
static void find_word(const char *str,
                      unsigned int *w_start,
                      unsigned int *w_end)
{
    for (; str[*w_start] != '\0' && isspace(str[*w_start]); *w_start += 1);

    for (*w_end = *w_start; str[*w_end] != '\0' && !isspace(str[*w_end]);
         *w_end += 1);
}

/* Print a line-folded description for a command-line option. */
static void print_opt_desc(const char *desc, unsigned int column)
{
    unsigned int word_start = 0;
    unsigned int word_end;
    char line_first_word;

    while(desc[word_start] != '\0')
    {
        set_column(DESC_START_COL, &column, 1);
        line_first_word = 1;

        find_word(desc, &word_start, &word_end);
        if (desc[word_start] == '\0')
            break;

        do
        {
            if (!line_first_word)
            {
                putchar(' ');
                column += 1;
            }
            column += fwrite(&desc[word_start], 1, word_end - word_start,
                             stdout);
            line_first_word = 0;

            word_start = word_end;
            find_word(desc, &word_start, &word_end);

        } while (desc[word_start] != '\0' &&
                 column + 1 + (word_end - word_start) < PAGE_WRAP_COL);
    }
}

/* Print information about a command-line option. */
static void print_option(const char *short_name,
                         const char *long_name,
                         const char *argument_template,
                         const char *description)
{
    unsigned int column = 0;

    if (short_name != NULL)
    {
        set_column(SHORT_START_COL, &column, 0);
        column += printf("-%c", short_name[0]);
    }
    if (long_name != NULL)
    {
        if (short_name != NULL)
        {
            putchar(',');
            column += 1;
        }
        set_column(LONG_START_COL, &column, 1);
        column += printf("--%s", long_name);
    }
    if (argument_template != NULL)
    {
        putchar((long_name != NULL)? '=' : ' ');
        column += 1;
        column += printf("<%s>", argument_template);
    }
    if (description != NULL)
        print_opt_desc(description, column);

    putchar('\n');
}

/* Print the help screen. */
static void print_help(const struct dooshki_args *args_ctxt)
{
    unsigned int opt_iter;

    print_usage(args_ctxt, 0);
    printf("%s\nOptions:\n", args_ctxt->description);

    for (opt_iter = 0;
         (args_ctxt->opt_desc[opt_iter].short_name != NULL ||
          args_ctxt->opt_desc[opt_iter].long_name  != NULL);
         opt_iter++)
    {
        print_option(args_ctxt->opt_desc[opt_iter].short_name,
                     args_ctxt->opt_desc[opt_iter].long_name,
                     args_ctxt->opt_desc[opt_iter].argument_template,
                     args_ctxt->opt_desc[opt_iter].description);
    }

    print_option(VER_SHORT_OPT_STR, VER_LONG_OPT, NULL, VER_DESC);
    print_option(HELP_SHORT_OPT_STR, HELP_LONG_OPT, NULL, HELP_DESC);
}

/* Print the program name and version. */
static void print_version(const struct dooshki_args *args_ctxt)
{
    printf("%s %s\n", args_ctxt->program_name, args_ctxt->version);
}


/* Collect a string argument. */
static char process_str_arg(const struct dooshki_opt *option,
                            const char *argument)
{
    const char **dest = option->opt_storage;
    *dest = argument;
    return 1;
}

/* Collect an unsigned integer argument. */
static char process_uint_arg(const struct dooshki_args *args_ctxt,
                             const struct dooshki_opt  *option,
                             const char  *opt_prefix,
                             const char  *opt_name,
                             const char  *argument)
{
    unsigned long *dest = option->opt_storage;
    char *test_ptr;

    errno = 0;
    *dest = strtoul(argument, &test_ptr, 10);
    if (*test_ptr != '\0' || argument[0] == '-')
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is not "
                    "a valid unsigned integer.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    if (*dest == ULONG_MAX && errno == ERANGE)
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is too large.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    return 1;
}

/* Collect a signed integer argument. */
static char process_int_arg(const struct dooshki_args *args_ctxt,
                            const struct dooshki_opt  *option,
                            const char  *opt_prefix,
                            const char  *opt_name,
                            const char  *argument)
{
    long *dest = option->opt_storage;
    char *test_ptr;

    errno = 0;
    *dest = strtol(argument, &test_ptr, 10);
    if (*test_ptr != '\0')
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is not "
                    "a valid integer.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    if (*dest == LONG_MAX && errno == ERANGE)
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is too large.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    if (*dest == LONG_MIN && errno == ERANGE)
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is too small.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    return 1;
}

/* Collect a floating point argument. */
static char process_float_arg(const struct dooshki_args *args_ctxt,
                              const struct dooshki_opt  *option,
                              const char  *opt_prefix,
                              const char  *opt_name,
                              const char  *argument)
{
    double *dest = option->opt_storage;
    char *test_ptr;

    errno = 0;
    *dest = strtod(argument, &test_ptr);
    if (*test_ptr != '\0')
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is not "
                    "a valid floating point number.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    if (*dest == HUGE_VAL && errno == ERANGE)
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is too large.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    if (*dest == -HUGE_VAL && errno == ERANGE)
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s is too small.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    if (*dest == 0 && errno == ERANGE)
    {
        print_error(args_ctxt,
                    "Argument `%s' passed to option %s%s would cause "
                    "an underflow.",
                    argument, opt_prefix, opt_name);
        return 0;
    }
    return 1;
}

/* Process the argument of an option, returns 1 on success, 0 on failure. */
static char process_opt_arg(const struct dooshki_args *args_ctxt,
                            const struct dooshki_opt  *option,
                            char         opt_is_long,
                            const char  *argument)
{
    const char *opt_prefix;
    const char *opt_name;
    char retval = 1;

    if (opt_is_long)
    {
        opt_prefix = "--";
        opt_name   = option->long_name;
    }
    else
    {
        opt_prefix = "-";
        opt_name   = option->short_name;
    }

    switch (option->type)
    {
        case DOOSHKI_OPT_STR:
            if (! process_str_arg(option, argument))
                retval = 0;
            break;

        case DOOSHKI_OPT_INT:
            if (! process_int_arg(args_ctxt, option, opt_prefix, opt_name,
                                  argument))
                retval = 0;
            break;

        case DOOSHKI_OPT_UINT:
            if (! process_uint_arg(args_ctxt, option, opt_prefix, opt_name,
                                   argument))
                retval = 0;
            break;

        case DOOSHKI_OPT_FLOAT:
            if (! process_float_arg(args_ctxt, option, opt_prefix, opt_name,
                                    argument))
                retval = 0;
            break;

        case DOOSHKI_OPT_CB:
            if (! option->callback(argument, option->opt_storage, opt_prefix,
                                   opt_name, option->callback_data))
                retval = 0;
            break;

        default:
            print_error(args_ctxt,
                        "Bug: Unknown argument type %u for option %s%s",
                         (unsigned int)option->type, opt_prefix, opt_name);
            retval = 0;
    }
    return retval;
}

static void process_long_opt(int *argc, char ***argv, unsigned int opt_argi,
                             const struct dooshki_args *args_ctxt,
                             char *show_help,
                             char *show_version,
                             char *errors_found)
{
    unsigned int iter;
    unsigned int opt_len;
    char opt_recognized;

    const char *option = (*argv)[opt_argi];
    const char *argument = NULL;

    if (strcmp(option, "--" HELP_LONG_OPT) == 0)
    {
        if (! *show_version)
            *show_help = 1;
        return;
    }

    if (strcmp(option, "--" VER_LONG_OPT) == 0)
    {
        if (! *show_help)
            *show_version = 1;
        return;
    }

    for (iter = 2; option[iter] != '\0' && option[iter] != '='; iter++);
    opt_len = iter - 2;

    if (option[iter] == '=')
        argument = &option[iter+1];

    for (iter = 0, opt_recognized = 0;
         (args_ctxt->opt_desc[iter].short_name != NULL ||
          args_ctxt->opt_desc[iter].long_name  != NULL) && !opt_recognized;
         iter++)
    {
        if (args_ctxt->opt_desc[iter].long_name == NULL)
            continue;

        if (strncmp(option + 2, args_ctxt->opt_desc[iter].long_name,
                    opt_len) == 0)
        {
            opt_recognized = 1;

            if (args_ctxt->opt_desc[iter].opt_found != NULL)
                *(args_ctxt->opt_desc[iter].opt_found) = 1;

            if (args_ctxt->opt_desc[iter].type == DOOSHKI_OPT_BOOL)
            {
                char *bool_ptr;

                if (argument != NULL)
                {
                    print_error(args_ctxt,
                                "Argument `%s' not expected for option --%s",
                                argument, args_ctxt->opt_desc[iter].long_name);
                    *errors_found = 1;
                    return;
                }

                bool_ptr = args_ctxt->opt_desc[iter].opt_storage;
                *bool_ptr = 1;
            }
            else if (args_ctxt->opt_desc[iter].type == DOOSHKI_OPT_NEGBOOL)
            {
                char *bool_ptr;

                if (argument != NULL)
                {
                    print_error(args_ctxt,
                                "Argument `%s' not expected for option --%s",
                                argument, args_ctxt->opt_desc[iter].long_name);
                    *errors_found = 1;
                    return;
                }

                bool_ptr = args_ctxt->opt_desc[iter].opt_storage;
                *bool_ptr = 0;
            }
            else if (args_ctxt->opt_desc[iter].type == DOOSHKI_OPT_CB_NOARG)
            {
                if (argument != NULL)
                {
                    print_error(args_ctxt,
                                "Argument `%s' not expected for option --%s",
                                argument, args_ctxt->opt_desc[iter].long_name);
                    *errors_found = 1;
                    return;
                }

                if (! args_ctxt->opt_desc[iter].callback(NULL,
                                                         args_ctxt->opt_desc[iter].opt_storage,
                                                         "--",
                                                         args_ctxt->opt_desc[iter].long_name,
                                                         args_ctxt->opt_desc[iter].callback_data))
                    *errors_found = 1;
            }
            else
            {
                if (argument == NULL)
                {
                    int arg_iter;

                    for (arg_iter = opt_argi + 1; arg_iter < *argc; arg_iter++)
                    {
                        if ((*argv)[arg_iter] != NULL)
                        {
                            if (strcmp((*argv)[arg_iter], "--") == 0)
                                break;
                            else
                            {
                                argument = (*argv)[arg_iter];
                                (*argv)[arg_iter] = NULL;
                                break;
                            }
                        }
                    }
                    if (argument == NULL)
                    {
                        print_error(args_ctxt,
                                    "Missing argument for option --%s",
                                    args_ctxt->opt_desc[iter].long_name);
                        *errors_found = 1;
                        return;
                    }
                }
                if (! process_opt_arg(args_ctxt, &args_ctxt->opt_desc[iter],
                                      1, argument))
                    *errors_found = 1;
            }
        }
    }
    if (!opt_recognized)
    {
        print_error(args_ctxt, "Unrecognized option %s", option);
        *errors_found = 1;
    }
}

static void process_short_opts(int *argc, char ***argv, unsigned int opt_argi,
                               const struct dooshki_args *args_ctxt,
                               char *show_help,
                               char *show_version,
                               char *errors_found)
{
    unsigned int in_iter;
    unsigned int opt_iter;
    char opt_recognized;

    const char *options = (*argv)[opt_argi];

    for (in_iter = 1; options[in_iter] != '\0'; in_iter++)
    {
        if (options[in_iter] == HELP_SHORT_OPT)
        {
            if (! *show_version)
                *show_help = 1;

            continue;
        }
        if (options[in_iter] == VER_SHORT_OPT)
        {
            if (! *show_help)
                *show_version = 1;

            continue;
        }
        for (opt_iter = 0, opt_recognized = 0;
             (args_ctxt->opt_desc[opt_iter].short_name != NULL ||
              args_ctxt->opt_desc[opt_iter].long_name  != NULL) &&
             !opt_recognized; opt_iter++)
        {
            if (args_ctxt->opt_desc[opt_iter].short_name == NULL)
                continue;

            if (options[in_iter] == (args_ctxt->opt_desc[opt_iter].short_name[0]))
            {
                opt_recognized = 1;

                if (args_ctxt->opt_desc[opt_iter].opt_found != NULL)
                    *(args_ctxt->opt_desc[opt_iter].opt_found) = 1;

                if (args_ctxt->opt_desc[opt_iter].type == DOOSHKI_OPT_BOOL)
                {
                    char *bool_ptr;

                    bool_ptr = args_ctxt->opt_desc[opt_iter].opt_storage;
                    *bool_ptr = 1;
                }
                else if (args_ctxt->opt_desc[opt_iter].type == DOOSHKI_OPT_NEGBOOL)
                {
                    char *bool_ptr;

                    bool_ptr = args_ctxt->opt_desc[opt_iter].opt_storage;
                    *bool_ptr = 0;
                }
                else if (args_ctxt->opt_desc[opt_iter].type == DOOSHKI_OPT_CB_NOARG)
                {

                    if (! args_ctxt->opt_desc[opt_iter].callback(NULL,
                                                                 args_ctxt->opt_desc[opt_iter].opt_storage,
                                                                 "-",
                                                                 args_ctxt->opt_desc[opt_iter].short_name,
                                                                 args_ctxt->opt_desc[opt_iter].callback_data))
                        *errors_found = 1;
                }
                else
                {
                    const char *argument;
                    int arg_iter;

                    for (arg_iter = opt_argi + 1, argument = NULL;
                         arg_iter < *argc && argument == NULL;
                         arg_iter++)
                    {
                        if ((*argv)[arg_iter] != NULL)
                        {
                            if (strcmp((*argv)[arg_iter], "--") == 0)
                                break;
                            else
                            {
                                argument = (*argv)[arg_iter];
                                (*argv)[arg_iter] = NULL;
                            }
                        }
                    }
                    if (argument == NULL)
                    {
                        print_error(args_ctxt,
                                    "Missing argument for option -%s",
                                    args_ctxt->opt_desc[opt_iter].short_name);
                        *errors_found = 1;
                    }
                    else if (! process_opt_arg(args_ctxt,
                                               &args_ctxt->opt_desc[opt_iter],
                                               0, argument))
                        *errors_found = 1;
                }
            }
        }
        if (!opt_recognized)
        {
            print_error(args_ctxt,
                        "Unrecognized option -%c", options[in_iter]);
            *errors_found = 1;
        }
    }
}

/* Remove NULL entries from argc/argv. */
static void deflate_args_list(int *argc, char ***argv)
{
    int fill_iter;
    int pull_iter;

    for (fill_iter = 1; fill_iter < *argc; fill_iter++)
    {
        if ((*argv)[fill_iter] == NULL)
        {
            for (pull_iter = fill_iter + 1; pull_iter < *argc; pull_iter++)
            {
                if ((*argv)[pull_iter] != NULL)
                {
                    (*argv)[fill_iter] = (*argv)[pull_iter];
                    (*argv)[pull_iter] = NULL;
                    break;
                }
            }
        }

        if ((*argv)[fill_iter] == NULL)
            break;
    }
    *argc = fill_iter;
}

enum dooshki_args_ret dooshki_args_parse(int *argc, char ***argv,
                                         const struct dooshki_args *args_ctxt)
{
    int arg_iter;

    char stopper_reached = 0;
    char show_help       = 0;
    char show_version    = 0;
    char errors_found    = 0;


    for (arg_iter = 1; arg_iter < *argc && !stopper_reached; arg_iter++)
    {
        if ((*argv)[arg_iter] != NULL && ((*argv)[arg_iter])[0] == '-')
        {
            if (((*argv)[arg_iter])[1] == '-')
            {
                if (((*argv)[arg_iter])[2] == '\0')
                    stopper_reached = 1;

                else
                    process_long_opt(argc, argv, arg_iter, args_ctxt,
                                     &show_help, &show_version, &errors_found);
            }
            else
                process_short_opts(argc, argv, arg_iter, args_ctxt,
                                   &show_help, &show_version, &errors_found);

            (*argv)[arg_iter] = NULL;
        }
    }
    deflate_args_list(argc, argv);

    if (show_help)
    {
        if (errors_found)
            fputc('\n', stderr);

        print_help(args_ctxt);
        return DOOSHKI_ARGS_HELP_SHOWN;
    }

    if (show_version)
    {
        if (errors_found)
            fputc('\n', stderr);

        print_version(args_ctxt);
        return DOOSHKI_ARGS_VER_SHOWN;
    }

    if (errors_found)
    {
        print_usage(args_ctxt, 1);
        return DOOSHKI_ARGS_PARSE_ERROR;
    }

    return DOOSHKI_ARGS_PARSE_OK;
}

void dooshki_args_err_usage(const struct dooshki_args *args_ctxt)
{
    print_usage(args_ctxt, 1);
}
