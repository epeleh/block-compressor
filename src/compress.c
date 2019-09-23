#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct compress_dictionary_item_t {
  uint8_t *data;
  uint16_t length;
  uint32_t use_count;
} compress_dictionary_item;

// ================================================================================ external functions

void compress(FILE *input, FILE *output);

// ================================================================================ internal functions

static int32_t parts_compare(const void *a, const void *b);
static void create_compress_dictionary(FILE *input);
static uint16_t *optimize_compress_dictionary(void);
static void delete_compress_dictionary(void);

// ================================================================================ internal variables

static const uint32_t CD_ITEM_LENGTH_LIMIT = 4;
static compress_dictionary_item *compress_dictionary = NULL;
static uint16_t compress_dictionary_size = 0;

// ================================================================================ definitions

int32_t parts_compare(const void *a, const void *b)
{
  const int16_t *av = *(int16_t **)a;
  const int16_t *bv = *(int16_t **)b;

  uint32_t min_length = 0;
  while (av[min_length] != EOF && bv[min_length] != EOF) {
    min_length++;
  }

  for (uint32_t i = 0; i < min_length; ++i) {
    if (av[i] == bv[i]) { continue; }
    return av[i] - bv[i];
  }

  return 0;
}

void create_compress_dictionary(FILE *input)
{
  for (uint16_t i = 0; i < 256; ++i) {
    uint32_t parts_count = 0;
    {
      rewind(input);
      int16_t ch;

      while ((ch = getc(input)) != EOF) {
        if (ch == i) { parts_count++; }
      }
    }

    if (parts_count < 2) { continue; }

    int16_t **parts = calloc(parts_count, sizeof(int16_t *));
    {
      rewind(input);
      int16_t ch;

      uint32_t last_ch_offset = 0;
      while ((ch = getc(input)) != EOF) {
        if (ch == i) { break; }
        last_ch_offset++;
      }

      uint32_t parts_i = 0;
      do {
        ch = getc(input);
        if (ch != i && ch != EOF) { continue; }
        if (ch != EOF) { fseek(input, -1, SEEK_CUR); }

        int32_t length = ftell(input) - last_ch_offset;
        parts[parts_i] = malloc((length + 1) * sizeof(int16_t));
        parts[parts_i][length] = EOF;

        fseek(input, -length, SEEK_CUR);
        for (int j = 0; j < length; ++j) {
          parts[parts_i][j] = getc(input);
        }

        parts_i++;
        last_ch_offset = ftell(input);
        fseek(input, 1, SEEK_CUR);
      } while (ch != EOF);
    }

    qsort(parts, parts_count, sizeof(int16_t *), parts_compare);

    for (uint32_t parts_i = 1; parts_i < parts_count; ++parts_i) {
      uint32_t length = 0;
      while (parts[parts_i - 1][length] == parts[parts_i][length] && parts[parts_i][length] != EOF) {
        length++;
      }

      if (length < CD_ITEM_LENGTH_LIMIT || length > 0xFFFF) {
        free(parts[parts_i - 1]);
        parts[parts_i - 1] = NULL;
        continue;
      }

      if (parts[parts_i - 1][length] != EOF) {
        parts[parts_i - 1] = realloc(parts[parts_i - 1], (length + 1) * sizeof(int16_t));
        parts[parts_i - 1][length] = EOF;
      }
    }

    uint32_t actual_parts_count = 0;
    {
      uint32_t parts_i = 0;
      while (parts[parts_i] == NULL) {
        parts_i++;
      }

      uint32_t last_parts_i = parts_i++;
      while (parts_i < parts_count) {
        actual_parts_count++;

        while (parts[parts_i] == NULL) {
          parts_i++;
        }

        for (uint32_t j = 0; parts[last_parts_i][j] == parts[parts_i][j]; ++j) {
          if (parts[parts_i][j] != EOF) { continue; }
          actual_parts_count--;
          free(parts[last_parts_i]);
          parts[last_parts_i] = NULL;
          break;
        }

        last_parts_i = parts_i++;
      }
    }

    free(parts[parts_count - 1]);

    if (!actual_parts_count) { continue; }
    if (compress_dictionary_size + actual_parts_count > 0xFFF) { break; }

    {
      uint16_t cdi = compress_dictionary_size;
      compress_dictionary_size += actual_parts_count;
      compress_dictionary = realloc(compress_dictionary, compress_dictionary_size * sizeof(compress_dictionary_item));

      for (uint32_t parts_i = 0; actual_parts_count--; ++cdi) {
        while (parts[parts_i] == NULL) {
          parts_i++;
        }

        compress_dictionary[cdi].length = 0;
        while (parts[parts_i][compress_dictionary[cdi].length] != EOF) {
          compress_dictionary[cdi].length++;
        }

        compress_dictionary[cdi].data = malloc(compress_dictionary[cdi].length * sizeof(uint8_t));
        for (uint32_t j = 0; j < compress_dictionary[cdi].length; ++j) {
          compress_dictionary[cdi].data[j] = parts[parts_i][j];
        }

        compress_dictionary[cdi].use_count = 0;
        free(parts[parts_i++]);
      }
    }
  }
}

uint16_t *optimize_compress_dictionary(void)
{
  return 0;
}

void delete_compress_dictionary(void)
{
}

void compress(FILE *input, FILE *output)
{
  create_compress_dictionary(input);

  // ============================================ DEBUG
  printf("cds = %d\n", compress_dictionary_size);
  for (int j = 0; j < compress_dictionary_size; ++j) {
    printf("use_cout = %d\tlength = %d\t data = ", compress_dictionary[j].use_count, compress_dictionary[j].length);
    for (int l = 0; l < compress_dictionary[j].length; ++l) {
      putc(compress_dictionary[j].data[l], stdout);
    }
    puts("");
  }
  // ============================================

  // FILE *tmp = tmpfile();
  // super_func(input, tmp);

  // uint16_t *new_dictionary_indexes = optimize_compress_dictionary();

  // write_compress_dictionary(output);
  // write_compress_data(tmp, output, new_dictionary_indexes);

  // free(new_dictionary_indexes);
  // fclose(tmp);

  delete_compress_dictionary();
}
