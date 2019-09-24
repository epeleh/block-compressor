#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum function_number {
  FN_SKIP,
  FN_SKIP_LONG,
  FN_REPEAT_BYTE,
  FN_REPEAT_BYTE_LONG,
  FN_REPEAT_STRING,
  FN_REPEAT_STRING_LONG,
  FN_MIRROR_STRING,
  FN_DICTIONARY,
  FN_ONE_PARTICULAR_BYTE,
  FN_ARITHMETIC_PROGRESSION,
  FN_GEOMETRIC_PROGRESSION,
  FN_FIBONACCI_PROGRESSION,
  FN_SHIFT_LEFT,
  FN_SHIFT_RIGHT,
  FN_OFFSET_SEGMENT,
  FN_JUMPING_SEGMENT,
};

typedef struct compress_dictionary_item_t {
  uint8_t *data;
  uint16_t length;
  bool used;
} compress_dictionary_item;

// ================================================================================ external functions

void compress(FILE *input, FILE *output);

// ================================================================================ internal functions

static void perform_compression(FILE *input, FILE *output);

static int32_t parts_compare(const void *a, const void *b);
static void create_compress_dictionary(FILE *input);
static void optimize_compress_dictionary(void);
static void delete_compress_dictionary(void);

static void write_compress_dictionary(FILE *output);
static void write_compress_data(FILE *input, FILE *output, uint16_t *new_dictionary_indexes);

// ================================================================================ internal variables

static const uint32_t CD_ITEM_LENGTH_LIMIT = 4;
static compress_dictionary_item *compress_dictionary = NULL;
static uint16_t compress_dictionary_size = 0;

// ================================================================================ definitions

void perform_compression(FILE *input, FILE *output)
{
  // TODO
}

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

      free(parts[--parts_count]);
    }

    if (!actual_parts_count) {
      free(parts);
      continue;
    }

    if (compress_dictionary_size + actual_parts_count > 0xFFF) {
      while (parts_count) {
        free(parts[--parts_count]);
      }
      free(parts);
      break;
    }

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

        compress_dictionary[cdi].used = false;
        free(parts[parts_i++]);
      }
    }

    free(parts);
  }
}

void optimize_compress_dictionary(void)
{
  // TODO
}

void delete_compress_dictionary(void)
{
  for (uint16_t i = 0; i < compress_dictionary_size; ++i) {
    free(compress_dictionary[i].data);
  }

  compress_dictionary_size = 0;
  free(compress_dictionary);
  compress_dictionary = NULL;
}

void write_compress_dictionary(FILE *output)
{
  fwrite(&compress_dictionary_size, sizeof(uint16_t), 1, output);
  for (int i = 0; i < compress_dictionary_size; ++i) {
    fwrite(&compress_dictionary[i].length, sizeof(uint16_t), 1, output);
    fwrite(compress_dictionary[i].data, sizeof(uint8_t), compress_dictionary[i].length, output);
  }
}

void write_compress_data(FILE *input, FILE *output, uint16_t *new_dictionary_indexes)
{
  // TODO: should change dictionary indexes
  int16_t ch;
  while ((ch = getc(input)) != EOF) {
    putc(ch, output);
  }
}

void compress(FILE *input, FILE *output)
{
  create_compress_dictionary(input);
  FILE *tmp = tmpfile();

  perform_compression(input, tmp);

  uint16_t *new_dictionary_indexes = malloc(compress_dictionary_size * sizeof(uint16_t));
  for (uint32_t new_i = 0, i = 0; i < compress_dictionary_size; ++i) {
    if (compress_dictionary[i].used) { new_dictionary_indexes[i] = new_i++; }
  }

  optimize_compress_dictionary();
  write_compress_dictionary(output);
  write_compress_data(tmp, output, new_dictionary_indexes);

  free(new_dictionary_indexes);
  fclose(tmp);
  delete_compress_dictionary();
}

// TODO: this function needs for debug only
void print_compress_dictionary()
{
  printf("cds = %d\n", compress_dictionary_size);
  for (int j = 0; j < compress_dictionary_size; ++j) {
    printf("used = %d\tlength = %d\t data = ", compress_dictionary[j].used, compress_dictionary[j].length);
    for (int l = 0; l < compress_dictionary[j].length; ++l) {
      putc(compress_dictionary[j].data[l], stdout);
    }
    puts("");
  }
}
