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

/*
 * Dooshki's Arguments library.
 *
 * This library is meant to be a simple getopt implementation with long
 * argument support, usable in any environment which supports ANSI C (C89).
 *
 * You can freely copy this library into the source tree of your project
 * and tweak it to your liking.  In particular, you can configure the layout
 * of the help screen by modifying the _COL constants in dooshki_args.c,
 * and modify the help and version options (short name, long name, description)
 * by changing the HELP_ and VER_ constants.
 *
 * You can find this library in https://github.com/dusxmt/dooshki-util
 */

/*
 * To use the library for command-line argument processing, fill out
 * a dooshki_args structure, and pass it to dooshki_args_parse, along
 * with references to argc and argv.
 *
 * The argc and argv variables will be modified, arguments processed by
 * the parsing routine will be removed.  This is useful as a means of either
 * detecting stray arguments, or for processing a general list of entries,
 * such as a list of files.
 *
 * Both short and long option types are supported, either with or without
 * an argument.  The argument can be a string, integer or floating point
 * number, or the user can provide their own argument processing routine
 * by specifying a callback routine.  The value of the argument is stored
 * in the opt_storage field.
 *
 * By specifying a non-NULL opt_found field, the user may detect whether
 * an option was specified on the command line or not.  This is mainly useful
 * in the case of numeric options, where the default desired behavior might
 * be different than if a numeric value was specified.
 *
 * NULL can be passed in place of either the long or short option name,
 * which will result in only the non-NULL option type being recognized.
 *
 * The bool and negbool option types can be used to turn on or off a boolean
 * variable, allowing for the support of --enable-feature and --disable-feature
 * types of options, which can be used to override each other (useful when
 * utilizing FLAGS variables to pass a set of default command-line options).
 *
 * This library performs no string copying or memory allocation, do not free()
 * any of the data collected by this library, the `const char *' values refer
 * directly to the strings provided by the execution environment (from argv).
 */

#ifndef DOOSHKI_ARGS_H
#define DOOSHKI_ARGS_H 1

enum dooshki_opt_type
{
    DOOSHKI_OPT_BOOL,     /* char, 1 = true, 0 = false   */
    DOOSHKI_OPT_NEGBOOL,  /* char, 0 = true, 1 = false   */
    DOOSHKI_OPT_STR,      /* `const char *'              */
    DOOSHKI_OPT_INT,      /* long                        */
    DOOSHKI_OPT_UINT,     /* unsigned long               */
    DOOSHKI_OPT_FLOAT,    /* double                      */
    DOOSHKI_OPT_CB,       /* user-defined, with argument */
    DOOSHKI_OPT_CB_NOARG  /* user-defined, no argument   */
};

struct dooshki_opt
{
    /* Note: Don't include the initial dashes in the option names. */
    const char *short_name;         /* -X                   */
    const char *long_name;          /* --option             */

    const char *argument_template;  /* --option=<arg_tmplt> */

    enum dooshki_opt_type type;

    void *opt_storage;
    char *opt_found;                /* set to 1 when the option is found */

    const char *description;

    /*
     * Callback interface, used by DOOSHKI_OPT_CB and DOOSHKI_OPT_CB_NOARG
     * options.
     *
     * `argument_text' contains the text of the argument passed to the option,
     * `opt_storage' is a pointer to where the result should be stored,
     * `opt_prefix' and `opt_name' are the dashes and the name of the option,
     * which are useful when constructing error messages, and `callback_data'
     * refers to any data which the user might want to make accessible to
     * the routine.
     *
     * `argument_text' is NULL for options of the DOOSHKI_OPT_CB_NOARG type.
     *
     * Return value of 1 signifies success, 0 signifies failure.
     */
    char (*callback)(const char *argument_text,
                     void       *opt_storage,
                     const char *opt_prefix,
                     const char *opt_name,
                     void       *callback_data);
    void *callback_data;
};

struct dooshki_args
{
    const char *program_name;   /* name of the executable */
    const char *version;        /* version string */
    const char *usage;          /* usage string */
    const char *summary;        /* short summary of the program */
    const char *description;    /* long description of the program */

    const struct dooshki_opt *opt_desc; /* array, last member is all NULL */
};

enum dooshki_args_ret
{
    DOOSHKI_ARGS_PARSE_OK,
    DOOSHKI_ARGS_HELP_SHOWN,
    DOOSHKI_ARGS_VER_SHOWN,
    DOOSHKI_ARGS_PARSE_ERROR
};

/*
 * Process command-line arguments.
 *
 *
 * The processed arguments are removed from argc and argv.  Check the value
 * of argc after the call to check for the presence of stray arguments, or
 * potentially a list of entries.
 *
 * All options are examined, to provide an exhaustive error listing for the
 * user, instead of stopping at the first error in the option.  Unknown options
 * are considered to be errors.
 *
 * Short options are recognized as letters after a dash, eg. -a is the short
 * option 'a', -abcd are the short options 'a', 'b', 'c' and 'd'.
 *
 * A long option is recognized as a string beginning with a double dash,
 * eg. --help is an example of a long option.
 *
 * Both types of options may be configured to require an argument.  In the
 * case of short options, this has to be specified in a separate argument word.
 * In the case of long options, an argument may be specified in a separate
 * word, or after an equals sign.
 *
 * For example, `-f file.txt', `--file file.txt' and `--file=file.txt'
 * are allowed, but `-ffile.txt' or `-f=file.txt' are not allowed.
 *
 * This is because of the way short argument handling works.  You may put
 * several short options which depend on an argument next to one other,
 * and they will consume the options in the order that they come.  For example,
 * if -f and -d accept arguments but -v doesn't, `-fvd file.txt database.dat' 
 * would be the same as `-f file.txt -v -d database.dat'.
 */
enum dooshki_args_ret dooshki_args_parse(int *argc, char ***argv,
                                         const struct dooshki_args *args_ctxt);

/*
 * Print program usage.
 *
 *
 * This routine in intended to be called when an error with the command line
 * arguments is discovered after the parsing has successfully completed.
 *
 * This routine prints a newline onto stderr (to separate the already printed
 * error messages from the usage text) and displays the program's name,
 * version, summary and usage information on stdout, suggesting to the user
 * to perhaps read the help screen.
 */
void dooshki_args_err_usage(const struct dooshki_args *args_ctxt);

#endif /* DOOSHKI_ARGS_H */
