#include "types.h"
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
  u8 *data;
  u16 length;
  bool used;
} compress_dictionary_item;

typedef struct compress_option_t {
  enum function_number fn;
  u32 offset;
  u8 *data;
  u32 length;
  u32 coverage;
} compress_option;

// ================================================================================ external functions

void compress(FILE *input, FILE *output);

// ================================================================================ internal functions

static compress_option check_repeat_byte(FILE *input, u32 coverage_limit);
static compress_option check_repeat_byte_long(FILE *input, u32 coverage_limit);
static compress_option check_repeat_string(FILE *input, u32 coverage_limit);
static compress_option check_repeat_string_long(FILE *input, u32 coverage_limit);
static compress_option check_mirror_string(FILE *input, u32 coverage_limit);
static compress_option check_dictionary(FILE *input, u32 coverage_limit);
static compress_option check_one_particular_byte(FILE *input, u32 coverage_limit);
static compress_option check_arithmetic_progression(FILE *input, u32 coverage_limit);
static compress_option check_geometric_progression(FILE *input, u32 coverage_limit);
static compress_option check_fibonacci_progression(FILE *input, u32 coverage_limit);
static compress_option check_shift_left(FILE *input, u32 coverage_limit);
static compress_option check_shift_right(FILE *input, u32 coverage_limit);
static compress_option check_offset_segment(FILE *input, u32 coverage_limit);
static compress_option check_jumping_segment(FILE *input, u32 coverage_limit);

static i32 options_compare(const void *a, const void *b);
static void perform_compression(FILE *input, FILE *output);

static i32 parts_compare(const void *a, const void *b);
static void create_compress_dictionary(FILE *input);
static void optimize_compress_dictionary(u16 *new_dictionary_indexes);
static void delete_compress_dictionary(void);

static void write_compress_dictionary(FILE *output);
static void write_compress_data(FILE *input, FILE *output, const u16 *new_dictionary_indexes);

// ================================================================================ internal variables

static compress_option next_comparison_option = {0};

static const u32 CD_ITEM_LENGTH_LIMIT = 4;
static compress_dictionary_item *compress_dictionary = NULL;
static u16 compress_dictionary_size = 0;

static compress_option (*const CHECK_FUNCTIONS[])(FILE *, u32) = {
  NULL,
  NULL,
  check_repeat_byte,
  check_repeat_byte_long,
  check_repeat_string,
  check_repeat_string_long,
  check_mirror_string,
  check_dictionary,
  check_one_particular_byte,
  check_arithmetic_progression,
  check_geometric_progression,
  check_fibonacci_progression,
  check_shift_left,
  check_shift_right,
  check_offset_segment,
  check_jumping_segment,
};

// ================================================================================ definitions

compress_option check_repeat_byte(FILE *input, u32 coverage_limit)
{
  if (!ftell(input)) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 16 ? 16 : coverage_limit;

  fseek(input, -1, SEEK_CUR);
  const u8 ch = getc(input);

  u32 i = 0;
  while (getc(input) == ch && i < coverage_limit) {
    i++;
  }

  if (!i) { return (compress_option){0}; }

  const compress_option co = {FN_REPEAT_BYTE, 0, malloc(1 * sizeof(u8)), 1, i};
  *co.data = ((i - 1) << 4) + FN_REPEAT_BYTE;
  return co;
}

compress_option check_repeat_byte_long(FILE *input, u32 coverage_limit)
{
  if (!ftell(input)) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 4096 ? 4096 : coverage_limit;

  fseek(input, -1, SEEK_CUR);
  const u8 ch = getc(input);

  u32 i = 0;
  while (getc(input) == ch && i < coverage_limit) {
    i++;
  }

  if (!i) { return (compress_option){0}; }

  const compress_option co = {FN_REPEAT_BYTE_LONG, 0, malloc(2 * sizeof(u8)), 2, i};
  co.data[0] = ((i - 1) << 4) + FN_REPEAT_BYTE_LONG;
  co.data[1] = (i - 1) >> 4;
  return co;
}

compress_option check_repeat_string(FILE *input, u32 coverage_limit)
{
  coverage_limit = coverage_limit > 17 ? 17 : coverage_limit;
  coverage_limit = coverage_limit > ftell(input) ? ftell(input) : coverage_limit;
  if (coverage_limit < 2) { return (compress_option){0}; }

  u8 *const str = alloca(coverage_limit * sizeof(u8));
  fread(str, sizeof(u8), coverage_limit, input);

  fseek(input, -(i8)coverage_limit * 2, SEEK_CUR);
  while (coverage_limit) {
    i8 i = 0;
    while (getc(input) == str[i] && i < coverage_limit) {
      i++;
    }

    if (i == coverage_limit) { break; }

    fseek(input, -i, SEEK_CUR);
    coverage_limit--;
  }

  if (coverage_limit < 2) { return (compress_option){0}; }

  const compress_option co = {FN_REPEAT_STRING, 0, malloc(1 * sizeof(u8)), 1, coverage_limit};
  *co.data = ((coverage_limit - 2) << 4) + FN_REPEAT_STRING;
  return co;
}

compress_option check_repeat_string_long(FILE *input, u32 coverage_limit)
{
  if (ftell(input) < 2 || coverage_limit < 4) { return (compress_option){0}; }
  u8 length = ftell(input) > 17 ? 17 : ftell(input);

  u8 *str = alloca(length * sizeof(u8));
  fseek(input, -(i8)length, SEEK_CUR);
  fread(str, sizeof(u8), length, input);

  bool found = false;
  for (u32 i = 0; i < length - 1; ++i) {
    const i32 offset = ftell(input);

    u32 j = i;
    while (j < length) {
      if (str[j++] != getc(input)) { break; }
    }

    if (j == length) {
      found = true;
      str += i;
      length -= i;
      break;
    }

    fseek(input, offset, SEEK_SET);
  }

  if (!found) { return (compress_option){0}; }

  u32 count = 0;
  i16 ch;
  while ((ch = getc(input)) != EOF && ch == str[count % length]) {
    count++;
  }

  count /= length;
  if (count > 256) { count = 256; }
  if (!count) { return (compress_option){0}; }

  const compress_option co = {FN_REPEAT_STRING_LONG, 0, malloc(2 * sizeof(u8)), 2, length * (count + 1)};
  co.data[0] = ((length - 2) << 4) + FN_REPEAT_STRING_LONG;
  co.data[1] = count - 1;
  return co;
}

compress_option check_mirror_string(FILE *input, u32 coverage_limit)
{
  coverage_limit = coverage_limit > 17 ? 17 : coverage_limit;
  coverage_limit = coverage_limit > ftell(input) ? ftell(input) : coverage_limit;
  if (coverage_limit < 2) { return (compress_option){0}; }

  fseek(input, -(i8)coverage_limit, SEEK_CUR);
  u8 *const str = alloca(coverage_limit * sizeof(u8));
  fread(str, sizeof(u8), coverage_limit, input);

  u8 i = 0;
  while (i < coverage_limit && getc(input) == str[coverage_limit - i - 1]) {
    i++;
  }

  if (i < 2) { return (compress_option){0}; }

  const compress_option co = {FN_MIRROR_STRING, 0, malloc(1 * sizeof(u8)), 1, i};
  *co.data = ((i - 2) << 4) + FN_MIRROR_STRING;
  return co;
}

compress_option check_dictionary(FILE *input, u32 coverage_limit)
{
  return (compress_option){0};
}

compress_option check_one_particular_byte(FILE *input, u32 coverage_limit)
{
  const u8 ch = getc(input);
  if (ch % 0x11 || !coverage_limit) { return (compress_option){0}; }

  const compress_option co = {FN_ONE_PARTICULAR_BYTE, 0, malloc(1 * sizeof(u8)), 1, 1};
  *co.data = ((ch / 0x11) << 4) + FN_ONE_PARTICULAR_BYTE;
  return co;
}

compress_option check_arithmetic_progression(FILE *input, u32 coverage_limit)
{
  if (!ftell(input)) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 16 ? 16 : coverage_limit;

  fseek(input, -1, SEEK_CUR);
  u8 value = getc(input);
  const i16 factor = getc(input) - value;
  ungetc(factor + value, input);

  u8 i = 0;
  while (getc(input) == (value += factor) && i < coverage_limit) {
    i++;
  }

  if (!i) { return (compress_option){0}; }

  const compress_option co = {FN_ARITHMETIC_PROGRESSION, 0, malloc(2 * sizeof(u8)), 2, i};
  co.data[0] = ((i - 1) << 4) + FN_ARITHMETIC_PROGRESSION;
  co.data[1] = factor;
  return co;
}

compress_option check_geometric_progression(FILE *input, u32 coverage_limit)
{
  if (!ftell(input)) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 16 ? 16 : coverage_limit;

  fseek(input, -1, SEEK_CUR);
  u8 value = getc(input);

  const u8 base = getc(input);
  if (!value || base % value) { return (compress_option){0}; }

  const i16 factor = base / value;
  ungetc(base, input);

  u8 i = 0;
  while (getc(input) == (value *= factor) && i < coverage_limit) {
    i++;
  }

  if (!i) { return (compress_option){0}; }

  const compress_option co = {FN_GEOMETRIC_PROGRESSION, 0, malloc(2 * sizeof(u8)), 2, i};
  co.data[0] = ((i - 1) << 4) + FN_GEOMETRIC_PROGRESSION;
  co.data[1] = factor;
  return co;
}

compress_option check_fibonacci_progression(FILE *input, u32 coverage_limit)
{
  if (ftell(input) < 2) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 16 ? 16 : coverage_limit;

  fseek(input, -2, SEEK_CUR);
  u8 first = getc(input);
  u8 second = getc(input);
  u8 next;

  u32 count = 0;
  while (getc(input) == (next = first + second) && count < coverage_limit) {
    first = second;
    second = next;
    count++;
  }

  if (!count) { return (compress_option){0}; }

  const compress_option co = {FN_FIBONACCI_PROGRESSION, 0, malloc(1 * sizeof(u8)), 1, count};
  *co.data = ((count - 1) << 4) + FN_FIBONACCI_PROGRESSION;
  return co;
}

compress_option check_shift_left(FILE *input, u32 coverage_limit)
{
  if (!ftell(input)) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 16 ? 16 : coverage_limit;

  fseek(input, -1, SEEK_CUR);
  u8 value = getc(input);

  u8 count = 0;
  while (getc(input) == (value = (value << 1) | (value >> 7)) && count < coverage_limit) {
    count++;
  }

  if (!count) { return (compress_option){0}; }

  const compress_option co = {FN_SHIFT_LEFT, 0, malloc(1 * sizeof(u8)), 1, count};
  *co.data = ((count - 1) << 4) + FN_SHIFT_LEFT;
  return co;
}

compress_option check_shift_right(FILE *input, u32 coverage_limit)
{
  if (!ftell(input)) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 16 ? 16 : coverage_limit;

  fseek(input, -1, SEEK_CUR);
  u8 value = getc(input);

  u8 count = 0;
  while (getc(input) == (value = (value >> 1) | (value << 7)) && count < coverage_limit) {
    count++;
  }

  if (!count) { return (compress_option){0}; }

  const compress_option co = {FN_SHIFT_RIGHT, 0, malloc(1 * sizeof(u8)), 1, count};
  *co.data = ((count - 1) << 4) + FN_SHIFT_RIGHT;
  return co;
}

compress_option check_offset_segment(FILE *input, u32 coverage_limit)
{
  if (coverage_limit < 2) { return (compress_option){0}; }
  coverage_limit = coverage_limit > 512 ? 512 : coverage_limit;

  const i32 pos = ftell(input);

  i16 count = 1;
  const u8 offset = getc(input) & 0xF0;
  while ((getc(input) & 0xF0) == offset && count < coverage_limit) {
    count++;
  }

  fseek(input, pos, SEEK_SET);

  count /= 2;
  if (count < 2) { return (compress_option){0}; }

  const compress_option co = {FN_OFFSET_SEGMENT, 0, malloc((count + 2) * sizeof(u8)), count + 2, count * 2};
  co.data[0] = offset + FN_OFFSET_SEGMENT;
  co.data[1] = count - 1;

  for (u32 i = 2; count-- > 0; i++) {
    co.data[i] = (getc(input) << 4) + (getc(input) & 0x0F);
  }

  return co;
}

compress_option check_jumping_segment(FILE *input, u32 coverage_limit)
{
  return (compress_option){0};
}

i32 options_compare(const void *a, const void *b)
{
  const compress_option av = *(compress_option *)a;
  const compress_option bv = *(compress_option *)b;
  if (bv.offset > av.offset) { next_comparison_option = bv; }
  return av.offset - bv.offset;
}

void perform_compression(FILE *input, FILE *output)
{
  fseek(input, 0, SEEK_END);
  const u32 input_length = ftell(input);
  double profit_limit = input_length > 8 ? (double)input_length / 8 : input_length;
  rewind(input);

  u32 options_capacity = 256;
  compress_option *options = malloc(options_capacity * sizeof(compress_option));
  u32 options_size = 0;

  while (profit_limit >= 1) {
    const u32 current_options_size = options_size;

    i16 ch;
    while ((ch = getc(input)) != EOF) {
      ungetc(ch, input);

      const i32 offset = ftell(input);
      u32 coverage_limit = input_length - offset;

      {
        next_comparison_option.fn = 0;
        const compress_option key_option = {.offset = offset};
        const compress_option *const found_option =
          bsearch(&key_option, options, current_options_size, sizeof(compress_option), options_compare);

        if (found_option) {
          fseek(input, found_option->coverage, SEEK_CUR);
          continue;
        }

        if (next_comparison_option.fn) {
          coverage_limit = next_comparison_option.offset - offset;
        }
      }

      compress_option best_co = {.length = 1};
      for (u8 i = 2; i < 0x10; ++i) {
        fseek(input, offset, SEEK_SET);

        const compress_option co = CHECK_FUNCTIONS[i](input, coverage_limit);
        if (!co.fn) { continue; }

        const double co_profit = (double)co.coverage / co.length;
        if (co_profit < profit_limit) {
          free(co.data);
          continue;
        }

        const double best_co_profit = (double)best_co.coverage / best_co.length;
        if (co_profit > best_co_profit) {
          free(best_co.data);
          best_co = co;
          continue;
        }

        free(co.data);
      }

      if (best_co.fn) {
        best_co.offset = offset;
        options[options_size++] = best_co;
        if (options_size >= options_capacity) {
          options_capacity *= 2;
          options = realloc(options, options_capacity * sizeof(compress_option));
        }
      }

      fseek(input, offset + (best_co.fn ? best_co.coverage : 1), SEEK_SET);
    }

    qsort(options, options_size, sizeof(compress_option), options_compare);
    rewind(input);
    profit_limit /= 2;
  }

  fseek(input, 0, SEEK_END);
  options[options_size++] = (compress_option){.offset = ftell(input)};
  rewind(input);

  for (u32 i = 0; i < options_size; ++i) {
    u16 skip_length = options[i].offset - ftell(input);

    if (skip_length) {
      if (skip_length > 16) {
        const u16 skip_length_buff = ((skip_length - 1) << 4) + FN_SKIP_LONG;
        fwrite(&skip_length_buff, sizeof(u16), 1, output);
      } else {
        putc(((skip_length - 1) << 4) + FN_SKIP, output);
      }

      while (skip_length--) {
        putc(getc(input), output);
      }
    }

    if (!options[i].fn) { continue; }

    // TODO: lines below just for debug
    printf("options[%d].fn = %d\n", i, options[i].fn);
    printf("options[%d].offset = %d\n", i, options[i].offset);
    printf("options[%d].length = %d\n", i, options[i].length);
    printf("options[%d].coverage = %d\n", i, options[i].coverage);
    printf("options[%d].data = %p\n", i, options[i].data);
    for (u32 j = 0; j < options[i].length; ++j) {
      printf("options[%d].data[%d] = %x\n", i, j, options[i].data[j]);
    }
    printf("\n");

    fwrite(options[i].data, sizeof(u8), options[i].length, output);
    free(options[i].data);
    fseek(input, options[i].coverage, SEEK_CUR);
  }

  free(options);
}

// ================================================================================ TODO: Refactor code below

i32 parts_compare(const void *a, const void *b)
{
  i16 *av = *(i16 **)a;
  i16 *bv = *(i16 **)b;

  u32 min_length = 0;
  while (av[min_length] != EOF && bv[min_length] != EOF) {
    min_length++;
  }

  for (u32 i = 0; i < min_length; ++i) {
    if (av[i] == bv[i]) { continue; }
    return av[i] - bv[i];
  }

  return 0;
}

void create_compress_dictionary(FILE *input)
{
  for (u16 i = 0; i < 256; ++i) {
    u32 parts_count = 0;
    {
      rewind(input);
      i16 ch;

      while ((ch = getc(input)) != EOF) {
        if (ch == i) { parts_count++; }
      }
    }

    if (parts_count < 2) { continue; }

    i16 **parts = calloc(parts_count, sizeof(i16 *));
    {
      rewind(input);
      i16 ch;

      u32 last_ch_offset = 0;
      while ((ch = getc(input)) != EOF) {
        if (ch == i) { break; }
        last_ch_offset++;
      }

      u32 parts_i = 0;
      do {
        ch = getc(input);
        if (ch != i && ch != EOF) { continue; }
        if (ch != EOF) { ungetc(ch, input); }

        i32 length = ftell(input) - last_ch_offset;
        parts[parts_i] = malloc((length + 1) * sizeof(i16));
        parts[parts_i][length] = EOF;

        fseek(input, -length, SEEK_CUR);
        for (u32 j = 0; j < length; ++j) {
          parts[parts_i][j] = getc(input);
        }

        parts_i++;
        last_ch_offset = ftell(input);
        fseek(input, 1, SEEK_CUR);
      } while (ch != EOF);
    }

    qsort(parts, parts_count, sizeof(i16 *), parts_compare);

    for (u32 parts_i = 1; parts_i < parts_count; ++parts_i) {
      u32 length = 0;
      while (parts[parts_i - 1][length] == parts[parts_i][length] && parts[parts_i][length] != EOF) {
        length++;
      }

      if (length < CD_ITEM_LENGTH_LIMIT || length > 0xFFFF) {
        free(parts[parts_i - 1]);
        parts[parts_i - 1] = NULL;
        continue;
      }

      if (parts[parts_i - 1][length] != EOF) {
        parts[parts_i - 1] = realloc(parts[parts_i - 1], (length + 1) * sizeof(i16));
        parts[parts_i - 1][length] = EOF;
      }
    }

    u32 actual_parts_count = 0;
    {
      u32 parts_i = 0;
      while (parts[parts_i] == NULL) {
        parts_i++;
      }

      u32 last_parts_i = parts_i++;
      while (parts_i < parts_count) {
        actual_parts_count++;

        while (parts[parts_i] == NULL) {
          parts_i++;
        }

        for (u32 j = 0; parts[last_parts_i][j] == parts[parts_i][j]; ++j) {
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
      u16 cdi = compress_dictionary_size;
      compress_dictionary_size += actual_parts_count;
      compress_dictionary = realloc(compress_dictionary, compress_dictionary_size * sizeof(compress_dictionary_item));

      for (u32 parts_i = 0; actual_parts_count--; ++cdi) {
        while (parts[parts_i] == NULL) {
          parts_i++;
        }

        compress_dictionary[cdi].length = 0;
        while (parts[parts_i][compress_dictionary[cdi].length] != EOF) {
          compress_dictionary[cdi].length++;
        }

        compress_dictionary[cdi].data = malloc(compress_dictionary[cdi].length * sizeof(u8));
        for (u32 j = 0; j < compress_dictionary[cdi].length; ++j) {
          compress_dictionary[cdi].data[j] = parts[parts_i][j];
        }

        compress_dictionary[cdi].used = false;
        free(parts[parts_i++]);
      }
    }

    free(parts);
  }
}

void optimize_compress_dictionary(u16 *new_dictionary_indexes)
{
  // TODO:
  // Compress compress_dictionary data
  // Fill new_dictionary_indexes
  // Reallocate compress_dictionary
  // Change compress_dictionary_size
}

void delete_compress_dictionary(void)
{
  for (u16 i = 0; i < compress_dictionary_size; ++i) {
    free(compress_dictionary[i].data);
  }

  compress_dictionary_size = 0;
  free(compress_dictionary);
  compress_dictionary = NULL;
}

void write_compress_dictionary(FILE *output)
{
  delete_compress_dictionary(); // TODO: remove this line

  putc(0xB2, output); // write first MAGIC_HEADER part
  u16 cds_with_second_magic_header_part = (compress_dictionary_size << 4) + 0x9;
  fwrite(&cds_with_second_magic_header_part, sizeof(u16), 1, output);
  for (u32 i = 0; i < compress_dictionary_size; ++i) {
    fwrite(&compress_dictionary[i].length, sizeof(u16), 1, output);
    fwrite(compress_dictionary[i].data, sizeof(u8), compress_dictionary[i].length, output);
  }
}

void write_compress_data(FILE *input, FILE *output, const u16 *new_dictionary_indexes)
{
  rewind(input);

  // TODO: should change dictionary indexes
  i16 ch;
  while ((ch = getc(input)) != EOF) {
    putc(ch, output);
  }
}

void compress(FILE *input, FILE *output)
{
  create_compress_dictionary(input);
  FILE *const tmp = tmpfile();

  perform_compression(input, tmp);

  {
    u16 *const new_dictionary_indexes = malloc(compress_dictionary_size * sizeof(u16));

    optimize_compress_dictionary(new_dictionary_indexes);
    write_compress_dictionary(output);
    write_compress_data(tmp, output, new_dictionary_indexes);

    free(new_dictionary_indexes);
  }

  fclose(tmp);
  delete_compress_dictionary();
}
