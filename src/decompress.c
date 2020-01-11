#include "types.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct decompress_dictionary_item_t {
  u8 *data;
  u16 length;
} decompress_dictionary_item;

// ================================================================================ external functions

void decompress(FILE *input, FILE *output);

// ================================================================================ internal functions

static void skip(FILE *input, FILE *output);                   // 0x0 | - 4[FN]  4[count] 8[bytes..]
static void skip_long(FILE *input, FILE *output);              // 0x1 | - 4[FN] 12[count] 8[bytes..]
static void repeat_byte(FILE *input, FILE *output);            // 0x2 | + 4[FN]  4[count]
static void repeat_byte_long(FILE *input, FILE *output);       // 0x3 | + 4[FN] 12[count]
static void repeat_string(FILE *input, FILE *output);          // 0x4 | + 4[FN] 4[length]
static void repeat_string_long(FILE *input, FILE *output);     // 0x5 | + 4[FN] 4[length] 8[count]
static void mirror_string(FILE *input, FILE *output);          // 0x6 | + 4[FN] 4[length]
static void dictionary(FILE *input, FILE *output);             // 0x7 | - 4[FN] 12[index]
static void one_particular_byte(FILE *input, FILE *output);    // 0x8 | - 4[FN] 4[offset]
static void arithmetic_progression(FILE *input, FILE *output); // 0x9 | + 4[FN] 4[count] 8[factor]
static void geometric_progression(FILE *input, FILE *output);  // 0xA | + 4[FN] 4[count] 8[factor]
static void fibonacci_progression(FILE *input, FILE *output);  // 0xB | + 4[FN] 4[count]
static void shift_left(FILE *input, FILE *output);             // 0xC | + 4[FN] 4[count]
static void shift_right(FILE *input, FILE *output);            // 0xD | + 4[FN] 4[count]
static void offset_segment(FILE *input, FILE *output);         // 0xE | - 4[FN] 4[offset] 8[count] 4[halves..]
static void jumping_segment(FILE *input, FILE *output);        // 0xF | - 4[FN] 4[offset] 8[count] 4[halves..]

static void create_decompress_dictionary(FILE *input);
static void delete_decompress_dictionary(void);

// ================================================================================ internal variables

static decompress_dictionary_item *decompress_dictionary;
static u16 decompress_dictionary_size;

static void (*const DECOMPRESS_FUNCTIONS[])(FILE *, FILE *) = {
  skip,
  skip_long,
  repeat_byte,
  repeat_byte_long,
  repeat_string,
  repeat_string_long,
  mirror_string,
  dictionary,
  one_particular_byte,
  arithmetic_progression,
  geometric_progression,
  fibonacci_progression,
  shift_left,
  shift_right,
  offset_segment,
  jumping_segment,
};

// ================================================================================ definitions

void skip(FILE *input, FILE *output)
{
  for (i8 i = getc(input) >> 4; i >= 0; --i) {
    putc(getc(input), output);
  }
}

void skip_long(FILE *input, FILE *output)
{
  i16 i;
  fread(&i, sizeof(i16), 1, input);
  i >>= 4;

  while (i-- >= 0) {
    putc(getc(input), output);
  }
}

void repeat_byte(FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  const u8 ch = getc(output);

  for (i8 i = getc(input) >> 4; i >= 0; --i) {
    putc(ch, output);
  }
}

void repeat_byte_long(FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  const u8 ch = getc(output);

  i16 i;
  fread(&i, sizeof(i16), 1, input);
  i >>= 4;

  while (i-- >= 0) {
    putc(ch, output);
  }
}

void repeat_string(FILE *input, FILE *output)
{
  const u8 length = (getc(input) >> 4) + 2;
  u8 *const str = alloca(length * sizeof(u8));

  fseek(output, -length, SEEK_CUR);
  fread(str, sizeof(u8), length, output);

  fwrite(str, sizeof(u8), length, output);
}

void repeat_string_long(FILE *input, FILE *output)
{
  const u8 length = (getc(input) >> 4) + 2;
  u8 *const str = alloca(length * sizeof(u8));

  fseek(output, -length, SEEK_CUR);
  fread(str, sizeof(u8), length, output);

  for (u16 i = getc(input) + 2; i; --i) {
    fwrite(str, sizeof(u8), length, output);
  }
}

void mirror_string(FILE *input, FILE *output)
{
  u8 length = (getc(input) >> 4) + 2;
  u8 *const str = alloca(length * sizeof(u8));

  fseek(output, -length, SEEK_CUR);
  fread(str, sizeof(u8), length, output);

  while (length) {
    putc(str[--length], output);
  }
}

void dictionary(FILE *input, FILE *output)
{
  u16 i;
  fread(&i, sizeof(u16), 1, input);
  i >>= 4;

  fwrite(decompress_dictionary[i].data, sizeof(u8), decompress_dictionary[i].length, output);
}

void one_particular_byte(FILE *input, FILE *output)
{
  putc((getc(input) >> 4) * 0x11, output);
}

void arithmetic_progression(FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  u8 value = getc(output);

  u8 i = (getc(input) >> 4) + 1;
  const u8 factor = getc(input);

  while (i--) {
    putc(value += factor, output);
  }
}

void geometric_progression(FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  u8 value = getc(output);

  u8 i = (getc(input) >> 4) + 1;
  const u8 factor = getc(input);

  while (i--) {
    putc(value *= factor, output);
  }
}

void fibonacci_progression(FILE *input, FILE *output)
{
  fseek(output, -2, SEEK_CUR);
  u8 first = getc(output);
  u8 second = getc(output);
  u8 next;

  for (i8 i = (getc(input) + 1) >> 4; i >= 0; --i) {
    next = first + second;
    first = second;
    second = next;
    putc(next, output);
  }
}

void shift_left(FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  u8 value = getc(output);

  for (i8 i = getc(input) >> 4; i >= 0; --i) {
    value = (value << 1) | (value >> 7);
    putc(value, output);
  }
}

void shift_right(FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  u8 value = getc(output);

  for (i8 i = getc(input) >> 4; i >= 0; --i) {
    value = (value >> 1) | (value << 7);
    putc(value, output);
  }
}

void offset_segment(FILE *input, FILE *output)
{
  const u8 offset = getc(input) & 0xF0;
  for (i16 i = getc(input); i >= 0; --i) {
    const u8 ch = getc(input);
    putc((ch >> 4) + offset, output);
    putc((ch & 0x0F) + offset, output);
  }
}

void jumping_segment(FILE *input, FILE *output)
{
  u8 value = (getc(input) & 0xF0) + 8;
  for (i16 i = getc(input); i >= 0; --i) {
    const u8 ch = getc(input);

    value += (ch >> 4) - 8 + ((ch >> 4) > 7);
    putc(value, output);

    value += (ch & 0x0F) - 8 + ((ch & 0x0F) > 7);
    putc(value, output);
  }
}

void create_decompress_dictionary(FILE *input)
{
  fseek(input, 1, SEEK_SET);
  fread(&decompress_dictionary_size, sizeof(u16), 1, input);
  decompress_dictionary_size >>= 4;

  decompress_dictionary = calloc(decompress_dictionary_size, sizeof(decompress_dictionary_item));

  FILE *const tmp = tmpfile();
  decompress_dictionary_item item;
  for (u16 i = 0; i < decompress_dictionary_size; ++i) {
    fread(&item.length, sizeof(u16), 1, input);

    if (item.length & 0x8000) {
      item.length &= 0x7FFF;

      rewind(tmp);
      for (u32 end = ftell(input) + item.length; ftell(input) < end;) {
        const u8 ch = getc(input);
        ungetc(ch, input);
        DECOMPRESS_FUNCTIONS[ch & 0x0F](input, tmp);
      }

      item.length = ftell(tmp);
      item.data = malloc(item.length * sizeof(u8));

      rewind(tmp);
      fread(item.data, sizeof(u8), item.length, tmp);
    } else {
      item.data = malloc(item.length * sizeof(u8));
      fread(item.data, sizeof(u8), item.length, input);
    }

    decompress_dictionary[i] = item;
  }

  fclose(tmp);
}

void delete_decompress_dictionary(void)
{
  while (decompress_dictionary_size) {
    free(decompress_dictionary[--decompress_dictionary_size].data);
  }

  free(decompress_dictionary);
}

void decompress(FILE *input, FILE *output)
{
  create_decompress_dictionary(input);

  i16 ch;
  while ((ch = getc(input)) != EOF) {
    ungetc(ch, input);
    DECOMPRESS_FUNCTIONS[ch & 0x0F](input, output);
  }

  delete_decompress_dictionary();
}
