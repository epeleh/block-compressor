#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern void decompress(FILE *input, FILE *output);

void print_usage(void)
{
  puts("Usage: bc");
}

int main(int argc, char *argv[])
{
  bool help_option = argc < 2;
  for (size_t i = 1; i < argc && !help_option; ++i) {
    help_option = !strcmp(argv[i], "--help");
  }

  if (help_option) {
    print_usage();
    return 0;
  }

  char buffer[] = {
    0x00, 0x02,
    0x80, 6, 0x03, 'F', 'u', 'c', 'k', 0x42,
    0x00, 4, 'Y', 'o', 'u', '!',
    0x70, 0, 0x70, 1, 0x46, 0x50, 6};
  FILE *input = fmemopen(buffer, 2 + 8 + 6 + 7, "rb");

  // FILE *input = fopen(argv[1], "rb");
  FILE *output = fopen(strcat(argv[1], ".bc"), "wb+");

  decompress(input, output);

  fclose(input);
  fclose(output);
  return 0;
}
