#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "errors.h"

typedef struct Args
{
    char *output;
    char *linker_script;
    char **inputs;
    unsigned num_inputs;
    char show_version   : 1;
    char show_help  : 1;
} Args;

typedef struct ArgsParserResult
{
    Args args;
    int status;
} ArgsParserResult;

typedef enum NextToken
{
    TOKEN_FLAG,
    TOKEN_OUTPUT,
    TOKEN_SCRIPT,
    TOKEN_INPUT
} NextToken;

static void print_help(const char *exec, FILE *fp);
static ArgsParserResult parse_args(int argc, const char *const *const argv);

static const char *const DEFAULT_OUTPUT = "vcpu816.bin";

int main(int argc, const char *const *const argv)
{
    int exit_code = 0;

    ArgsParserResult args_res = parse_args(argc, argv);
    exit_code = args_res.status;

    if(!exit_code)
    {
        Args args = args_res.args;

        if(args.show_version)
        {
            printf("%s (vcpu816-al-link) version " VERSION_STR "\n"
                "MIT License\n"
                "\n"
                "Copyright (c) 2022 abirvalarg\n"
                "\n"
                "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
                "of this software and associated documentation files (the \"Software\"), to deal\n"
                "in the Software without restriction, including without limitation the rights\n"
                "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
                "copies of the Software, and to permit persons to whom the Software is\n"
                "furnished to do so, subject to the following conditions:\n"
                "\n"
                "The above copyright notice and this permission notice shall be included in all\n"
                "copies or substantial portions of the Software.\n"
                "\n"
                "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
                "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
                "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
                "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
                "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
                "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
                "SOFTWARE.\n",
                argv[0]
            );
        }
        else if(args.show_help)
            print_help(argv[0], stdout);
        else if(args.num_inputs == 0)
        {
            fputs("No input files provided\n", stderr);
            exit_code = 1;
        }

        if(args.output)
            free(args.output);
        if(args.linker_script)
            free(args.linker_script);
        for(unsigned i = 0; i < args.num_inputs; i++)
            free(args.inputs[i]);
        if(args.inputs)
            free(args.inputs);
    }
    else
        print_help(argv[0], stderr);

    return exit_code;
}

static void print_help(const char *exec, FILE *fp)
{
    fprintf(fp, "Usage:\n%s [flags] <inputs>\n"
        "Flags:\n"
        "-h --help\tShow this message\n"
        "-v --version\tShow version and license\n"
        "-o --output\tSpecify output file\n"
        "-T --script\tSpecify linker script\n"
        "--\t\tStop processing flags and treat everything after it as an input file\n",
        exec
    );
}

static ArgsParserResult parse_args(int argc, const char *const *const argv)
{
    char *output = malloc(strlen(DEFAULT_OUTPUT) + 1);
    if(!output)
        out_of_memory();
    strcpy(output, DEFAULT_OUTPUT);
    char *linker_script = 0;
    char **inputs = malloc(sizeof(char*));
    if(!inputs)
        out_of_memory();
    unsigned num_inputs = 0,
        inputs_capacity = 1;
    char show_version = 0;
    char show_help = 0;
    NextToken next_token = TOKEN_FLAG;
    int status = 0;

    for(unsigned i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        switch(next_token)
        {
        case TOKEN_FLAG:
            if(arg[0] == '-')
            {
                if(!strcmp(arg, "-h") || !strcmp(arg, "--help"))
                    show_help = 1;
                else if(!strcmp(arg, "-v") || !strcmp(arg, "--version"))
                    show_version = 1;
                else if(!strcmp(arg, "-o") || !strcmp(arg, "--output"))
                    next_token = TOKEN_OUTPUT;
                else if(!strcmp(arg, "-T") || !strcmp(arg, "--script"))
                    next_token = TOKEN_SCRIPT;
                else if(!strcmp(arg, "--"))
                    next_token = TOKEN_INPUT;
                else
                {
                    status = 1;
                    free(output);
                    output = 0;
                    if(linker_script)
                        free(linker_script);
                    linker_script = 0;
                    for(unsigned i = 0; i < num_inputs; i++)
                        free(inputs[i]);
                    free(inputs);
                    inputs = 0;
                    break;
                }
            }
            else
            {
                if(num_inputs == inputs_capacity)
                {
                    unsigned new_capacity = inputs_capacity * 2;
                    char **new_inputs = malloc(sizeof(char*) * new_capacity);
                    if(!new_inputs)
                        out_of_memory();
                    memcpy(new_inputs, inputs, sizeof(char*) * num_inputs);
                    free(inputs);
                    inputs = new_inputs;
                    inputs_capacity = new_capacity;
                }
                inputs[num_inputs] = malloc(strlen(arg) + 1);
                if(!inputs[num_inputs])
                    out_of_memory();
                strcpy(inputs[num_inputs++], arg);
            }
            break;
        
        case TOKEN_OUTPUT:
            output = realloc(output, strlen(arg) + 1);
            if(!output)
                out_of_memory();
            strcpy(output, arg);
            next_token = TOKEN_FLAG;
            break;
        
        case TOKEN_SCRIPT:
            linker_script = realloc(linker_script, strlen(arg) + 1);
            if(!linker_script)
                out_of_memory();
            strcpy(linker_script, arg);
            next_token = TOKEN_FLAG;
            break;
        
        case TOKEN_INPUT:
            if(num_inputs == inputs_capacity)
            {
                unsigned new_capacity = inputs_capacity * 2;
                char **new_inputs = malloc(sizeof(char*) * new_capacity);
                if(!new_inputs)
                    out_of_memory();
                memcpy(new_inputs, inputs, sizeof(char*) * num_inputs);
                free(inputs);
                inputs = new_inputs;
                inputs_capacity = new_capacity;
            }
            inputs[num_inputs] = malloc(strlen(arg) + 1);
            if(!inputs[num_inputs])
                out_of_memory();
            strcpy(inputs[num_inputs++], arg);
            break;
        }
        if(status)
            break;
    }

    return (ArgsParserResult){
        .status = status,
        .args = {
            .output = output,
            .linker_script = linker_script,
            .inputs = inputs,
            .num_inputs = num_inputs,
            .show_version = show_version,
            .show_help = show_help
        }
    };
}
