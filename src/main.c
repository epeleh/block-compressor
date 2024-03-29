#include "types.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define APP_NAME "bczip"
#define EXT_NAME "bc"
#define MAGIC_HEADER 0xBC09

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

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
    "  -V, --version     display version number\n"
    "\n"
    "With no FILE, read standard input.");
}

static void print_version(void)
{
  puts(APP_NAME " 1.0");
}

static bool file_exist(const char *filepath)
{
  FILE *const file = fopen(filepath, "rb");
  if (!file) { return false; }
  fclose(file);
  return true;
}

static bool magic_header_valid(FILE *compressed_file)
{
  rewind(compressed_file);
  return (getc(compressed_file) << 8) + (getc(compressed_file) & 0x0F) == MAGIC_HEADER;
}

int main(int argc, char *argv[])
{
  command_line_options options = {0};
  bool no_files = true;
  {
    for (u32 i = 1; i < argc; ++i) {
      if (argv[i][0] != '-') {
        no_files = false;
        continue;
      }

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
          eprintf(APP_NAME ": invalid option '%s'\n", argv[i]);
          return 1;
        }
        continue;
      }

      for (u32 j = 1; argv[i][j]; ++j) {
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
          eprintf(APP_NAME ": invalid option '-%c'\n", argv[i][j]);
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

  if (!isatty(fileno(stdin)) && no_files) {
    FILE *const input_tmp = tmpfile();
    FILE *const output_tmp = tmpfile();

    i16 ch;
    while ((ch = getc(stdin)) != EOF) {
      putc(ch, input_tmp);
    }

    if (options.decompress) {
      if (magic_header_valid(input_tmp)) {
        decompress(input_tmp, output_tmp);
      } else {
        if (!options.quiet) { eprintf(APP_NAME ": stdin not in " APP_NAME " format\n"); }
      }
    } else {
      compress(input_tmp, output_tmp);
    }

    rewind(output_tmp);
    while ((ch = getc(output_tmp)) != EOF) {
      putc(ch, stdout);
    }

    fclose(input_tmp);
    fclose(output_tmp);
    return 0;
  }

  if (no_files) {
    print_help();
    return 0;
  }

  for (u32 i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') { continue; }

    FILE *const input = fopen(argv[i], "rb+");
    if (!input) {
      if (!options.quiet) { eprintf(APP_NAME ": no such file '%s'\n", argv[i]); }
      continue;
    }

    const u32 input_pathname_length = strlen(argv[i]);
    u32 output_pathname_length;
    char *output_pathname;
    FILE *output;

    if (options.decompress) {
      if (input_pathname_length < strlen(EXT_NAME) + 2 ||
          strcmp(argv[i] + input_pathname_length - strlen(EXT_NAME) - 1, "." EXT_NAME)) {
        if (!options.quiet) { eprintf(APP_NAME ": '%s' has unknown suffix\n", argv[i]); }
        fclose(input);
        continue;
      }

      if (!magic_header_valid(input)) {
        if (!options.quiet) { eprintf(APP_NAME ": '%s' not in " APP_NAME " format\n", argv[i]); }
        fclose(input);
        continue;
      }

      output_pathname_length = input_pathname_length - strlen(EXT_NAME) - 1;
      output_pathname = malloc((output_pathname_length + 1) * sizeof(char));
      memcpy(output_pathname, argv[i], output_pathname_length);
      output_pathname[output_pathname_length] = '\0';

      if (!options.force && !options.stdout && file_exist(output_pathname)) {
        printf(APP_NAME ": '%s' already exists; do you want to overwrite (y/N)? ", output_pathname);
        i16 ch = getchar();

        i16 c = ch;
        while (c != EOF && c != '\n') {
          c = getchar();
        }

        if (tolower(ch) != 'y') {
          puts("\tnot overwritten");
          fclose(input);
          free(output_pathname);
          continue;
        }
      }

      output = options.stdout ? tmpfile() : fopen(output_pathname, "wb+");
      if (!output) {
        if (!options.quiet) { eprintf(APP_NAME ": '%s' can't open output stream\n", argv[i]); }
        fclose(input);
        free(output_pathname);
        continue;
      }

      decompress(input, output);
    } else {
      if (input_pathname_length > strlen(EXT_NAME) + 1 &&
          !strcmp(argv[i] + input_pathname_length - strlen(EXT_NAME) - 1, "." EXT_NAME)) {
        if (!options.quiet) { eprintf(APP_NAME ": '%s' already has '." EXT_NAME "' suffix\n", argv[i]); }
        fclose(input);
        continue;
      }

      output_pathname_length = strlen(argv[i]) + strlen(EXT_NAME) + 1;
      output_pathname = malloc((output_pathname_length + 1) * sizeof(char));
      memcpy(output_pathname, argv[i], input_pathname_length + 1);
      strncat(output_pathname, "." EXT_NAME, strlen(EXT_NAME) + 1);

      if (!options.force && !options.stdout && file_exist(output_pathname)) {
        printf(APP_NAME ": '%s' already exists; do you want to overwrite (y/N)? ", output_pathname);
        i16 ch = getchar();

        i16 c = ch;
        while (c != EOF && c != '\n') {
          c = getchar();
        }

        if (tolower(ch) != 'y') {
          puts("\tnot overwritten");
          fclose(input);
          free(output_pathname);
          continue;
        }
      }

      output = options.stdout ? tmpfile() : fopen(output_pathname, "wb+");
      if (!output) {
        if (!options.quiet) { eprintf(APP_NAME ": '%s' can't open output stream\n", argv[i]); }
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
      if (options.decompress) {
        diff = (double)ftell(input) / ftell(output);
      } else {
        diff = (double)ftell(output) / ftell(input);
      }

      printf(APP_NAME ": '%s'\t%3.1f%% replaced with '%s'\n", argv[i], diff * 100, output_pathname);
    }

    if (options.stdout) {
      rewind(output);

      i16 ch;
      while ((ch = getc(output)) != EOF) {
        putc(ch, stdout);
      }
    }

    fclose(input);
    fclose(output);
    free(output_pathname);

    if (!options.keep && !options.stdout) { remove(argv[i]); }
  }

  return 0;
}
