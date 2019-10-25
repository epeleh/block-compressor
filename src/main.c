#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_NAME "bc"
#define MAGIC_HEADER 0xB290

extern void compress(FILE *input, FILE *output);
extern void decompress(FILE *input, FILE *output);

typedef struct command_line_options_t {
  bool stdout;
  bool decompress;
  bool force;
  bool help;
  bool keep;
  bool quiet;
  bool verbose;
  bool version;
} command_line_options;

static void print_help(void)
{
  puts(
    "Usage: " APP_NAME
    " [OPTION]... [FILE]...\n"
    "Compress or uncompress FILEs (by default, compress FILES in-place).\n"
    "\n"
    "  -c, --stdout      write on standard output, keep original files unchanged\n"
    "  -d, --decompress  decompress\n"
    "  -f, --force       force overwrite of output file\n"
    "  -h, --help        give this help\n"
    "  -k, --keep        keep (don't delete) input files\n"
    "  -q, --quiet       suppress all warnings\n"
    "  -v, --verbose     verbose mode\n"
    "  -V, --version     display version number\n");
}

static void print_version(void)
{
  puts(APP_NAME " 1.0");
}

int main(int argc, char *argv[])
{
  command_line_options options = {0};
  {
    for (size_t i = 1; i < argc; ++i) {
      if (argv[i][0] != '-') { continue; }

      if (argv[i][1] == '-') {
        if (!strcmp(argv[i] + 2, "stdout")) {
          options.stdout = true;
        } else if (!strcmp(argv[i] + 2, "decompress")) {
          options.decompress = true;
        } else if (!strcmp(argv[i] + 2, "force")) {
          options.force = true;
        } else if (!strcmp(argv[i] + 2, "help")) {
          options.help = true;
        } else if (!strcmp(argv[i] + 2, "keep")) {
          options.keep = true;
        } else if (!strcmp(argv[i] + 2, "quiet")) {
          options.quiet = true;
        } else if (!strcmp(argv[i] + 2, "verbose")) {
          options.verbose = true;
        } else if (!strcmp(argv[i] + 2, "version")) {
          options.version = true;
        } else {
          printf(APP_NAME ": invalid option '%s'\n", argv[i]);
          return 1;
        }
        continue;
      }

      for (size_t j = 1; argv[i][j]; ++j) {
        if (argv[i][j] == 'c') {
          options.stdout = true;
        } else if (argv[i][j] == 'd') {
          options.decompress = true;
        } else if (argv[i][j] == 'f') {
          options.force = true;
        } else if (argv[i][j] == 'h') {
          options.help = true;
        } else if (argv[i][j] == 'k') {
          options.keep = true;
        } else if (argv[i][j] == 'q') {
          options.quiet = true;
        } else if (argv[i][j] == 'v') {
          options.verbose = true;
        } else if (argv[i][j] == 'V') {
          options.version = true;
        } else {
          printf(APP_NAME ": invalid option '-%c'\n", argv[i][j]);
          return 1;
        }
      }
    }
  }

  if (options.help) {
    print_help();
    return 0;
  }

  if (options.version) {
    print_version();
    return 0;
  }

  for (size_t i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') { continue; }

    FILE *input = fopen(argv[i], "rb+");
    if (!input) {
      if (!options.quiet) { printf(APP_NAME ": no such file '%s'\n", argv[i]); }
      continue;
    }

    size_t input_pathname_length = strlen(argv[i]);
    size_t output_pathname_length;
    char *output_pathname;
    FILE *output;

    if (options.decompress) {
      if (input_pathname_length < strlen(APP_NAME) + 2 ||
          strcmp(argv[i] + input_pathname_length - strlen(APP_NAME) - 1, "." APP_NAME)) {
        if (!options.quiet) { printf(APP_NAME ": '%s' unknown suffix\n", argv[i]); }
        fclose(input);
        continue;
      }

      if (getc(input) != (MAGIC_HEADER >> 8) || (getc(input) & 0xF0) != (MAGIC_HEADER & 0xFF)) {
        if (!options.quiet) { printf(APP_NAME ": '%s' not in %s format\n", argv[i], APP_NAME); }
        fclose(input);
        continue;
      }

      output_pathname_length = input_pathname_length - strlen(APP_NAME) - 1;
      output_pathname = malloc((output_pathname_length + 1) * sizeof(char));
      memcpy(output_pathname, argv[i], output_pathname_length);
      output_pathname[output_pathname_length] = '\0';

      output = options.stdout ? stdout : fopen(output_pathname, "wb+");
      if (!output) {
        if (!options.quiet) { printf(APP_NAME ": '%s' can't open output stream\n", argv[i]); }
        fclose(input);
        free(output_pathname);
        continue;
      }

      rewind(input);
      decompress(input, output);
    } else {
      if (input_pathname_length > strlen(APP_NAME) + 1 &&
          !strcmp(argv[i] + input_pathname_length - strlen(APP_NAME) - 1, "." APP_NAME)) {
        if (!options.quiet) { printf(APP_NAME ": '%s' already has .%s suffix\n", argv[i], APP_NAME); }
        fclose(input);
        continue;
      }

      output_pathname_length = strlen(argv[i]) + strlen(APP_NAME) + 1;
      output_pathname = malloc((output_pathname_length + 1) * sizeof(char));
      memcpy(output_pathname, argv[i], input_pathname_length + 1);
      strncat(output_pathname, "." APP_NAME, strlen(APP_NAME) + 1);

      output = options.stdout ? stdout : fopen(output_pathname, "wb+");
      if (!output) {
        if (!options.quiet) { printf(APP_NAME ": '%s' can't open output stream\n", argv[i]); }
        fclose(input);
        free(output_pathname);
        continue;
      }

      compress(input, output);
    }

    if (options.verbose && !options.stdout) {
      fseek(input, 0, SEEK_END);
      fseek(output, 0, SEEK_END);

      double diff;
      if (ftell(input) > ftell(output)) {
        diff = (double)ftell(input) / ftell(output);
      } else {
        diff = (double)ftell(output) / ftell(input);
      }

      printf(APP_NAME ": '%s'\t%3.1f%% replaced with '%s'\n", argv[i], diff * 100, output_pathname);
    }

    fclose(input);
    fclose(output);
    free(output_pathname);

    if (!options.keep && !options.stdout) { remove(argv[i]); }
  }

  return 0;
}
