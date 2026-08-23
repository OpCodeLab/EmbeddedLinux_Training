#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage(const char *prog_name) {
    fprintf(stderr,
        "Usage: %s [-v] [-o output_file] -n name [-h]\n"
        "  -v            verbose mode\n"
        "  -o <file>     specify output file\n"
        "  -n <name>     specify a required name\n"
        "  -h            show this help message\n",
        prog_name);
}

int main(int argc, char *argv[]) 
{
    int verbose_flag = 0;
    char *output_file = NULL;
    char *name = NULL;
    int opt;

    /* The leading ':' lets getopt distinguish missing-argument (':')
       from unknown-option ('?') errors. Options followed by ':' 
       require an argument. */
    while ((opt = getopt(argc, argv, ":vo:n:h")) != -1) {
        switch (opt) {
            case 'v':
                verbose_flag = 1;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'n':
                name = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case ':':
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            case '?':
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            default:
                /* Should never get here */
                abort();
        }
    }

    if (name == NULL) {
        fprintf(stderr, "Error: -n <name> is required.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("verbose = %d\n", verbose_flag);
    printf("output_file = %s\n", output_file ? output_file : "(none)");
    printf("name = %s\n", name);

    /* Handle any remaining non-option arguments */
    if (optind < argc) {
        printf("Non-option arguments: ");
        for (int i = optind; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}