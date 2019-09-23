#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct decompress_dictionary_item_t {
  uint8_t *data;
  uint16_t length;
} decompress_dictionary_item;

// ================================================================================ external functions

void decompress(FILE *input, FILE *output);

// ================================================================================ internal functions

static void skip(uint8_t lower_half, FILE *input, FILE *output);                   // 0x0 | 4[FN]  4[count] 8[bytes..]
static void skip_long(uint8_t lower_half, FILE *input, FILE *output);              // 0x1 | 4[FN] 12[count] 8[bytes..]
static void repeat_byte(uint8_t lower_half, FILE *input, FILE *output);            // 0x2 | 4[FN]  4[count]
static void repeat_byte_long(uint8_t lower_half, FILE *input, FILE *output);       // 0x3 | 4[FN] 12[count]
static void repeat_string(uint8_t lower_half, FILE *input, FILE *output);          // 0x4 | 4[FN] 4[length]
static void repeat_string_long(uint8_t lower_half, FILE *input, FILE *output);     // 0x5 | 4[FN] 4[length] 8[count]
static void mirror_string(uint8_t lower_half, FILE *input, FILE *output);          // 0x6 | 4[FN] 4[length]
static void dictionary(uint8_t lower_half, FILE *input, FILE *output);             // 0x7 | 4[FN] 12[index]
static void one_particular_byte(uint8_t lower_half, FILE *input, FILE *output);    // 0x8 | 4[FN] 4[offset]
static void arithmetic_progression(uint8_t lower_half, FILE *input, FILE *output); // 0x9 | 4[FN] 4[count] 8[factor]
static void geometric_progression(uint8_t lower_half, FILE *input, FILE *output);  // 0xA | 4[FN] 4[count] 8[factor]
static void fibonacci_progression(uint8_t lower_half, FILE *input, FILE *output);  // 0xB | 4[FN] 4[count]
static void shift_left(uint8_t lower_half, FILE *input, FILE *output);             // 0xC | 4[FN] 4[count]
static void shift_right(uint8_t lower_half, FILE *input, FILE *output);            // 0xD | 4[FN] 4[count]
static void offset_segment(uint8_t lower_half, FILE *input, FILE *output);         // 0xE | 4[FN] 4[offset] 8[count] 4[halves..]
static void jumping_segment(uint8_t lower_half, FILE *input, FILE *output);        // 0xF | 4[FN] 4[offset] 8[count] 4[halves..]

static void create_decompress_dictionary(FILE *input);
static void delete_decompress_dictionary(void);

// ================================================================================ internal variables

static decompress_dictionary_item *decompress_dictionary;
static uint16_t decompress_dictionary_size;

static void (*const DECOMPRESS_FUNCTIONS[])(uint8_t, FILE *, FILE *) = {
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

void skip(uint8_t lower_half, FILE *input, FILE *output)
{
  lower_half++;
  while (lower_half--) {
    putc(getc(input), output);
  }
}

void skip_long(uint8_t lower_half, FILE *input, FILE *output)
{
  uint32_t i = (getc(input) << 4) + lower_half + 1;
  while (i--) {
    putc(getc(input), output);
  }
}

void repeat_byte(uint8_t lower_half, FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  uint8_t ch = getc(output);

  lower_half++;
  while (lower_half--) {
    putc(ch, output);
  }
}

void repeat_byte_long(uint8_t lower_half, FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  uint8_t ch = getc(output);

  uint32_t i = (getc(input) << 4) + lower_half + 1;
  while (i--) {
    putc(ch, output);
  }
}

void repeat_string(uint8_t lower_half, FILE *input, FILE *output)
{
  lower_half += 2;

  uint8_t *str = alloca(lower_half * sizeof(uint8_t));
  fseek(output, -lower_half, SEEK_CUR);
  fread(str, 1, lower_half, output);

  fwrite(str, 1, lower_half, output);
}

void repeat_string_long(uint8_t lower_half, FILE *input, FILE *output)
{
  lower_half += 2;

  uint8_t *str = alloca(lower_half * sizeof(uint8_t));
  fseek(output, -lower_half, SEEK_CUR);
  fread(str, 1, lower_half, output);

  uint16_t i = getc(input) + 2;
  while (i--) {
    fwrite(str, 1, lower_half, output);
  }
}

void mirror_string(uint8_t lower_half, FILE *input, FILE *output)
{
  lower_half += 2;

  uint8_t *str = alloca(lower_half * sizeof(uint8_t));
  fseek(output, -lower_half, SEEK_CUR);
  fread(str, 1, lower_half, output);

  while (lower_half) {
    putc(str[--lower_half], output);
  }
}

void dictionary(uint8_t lower_half, FILE *input, FILE *output)
{
  uint16_t i = (lower_half << 8) + getc(input);
  fwrite(decompress_dictionary[i].data, 1, decompress_dictionary[i].length, output);
}

void one_particular_byte(uint8_t lower_half, FILE *input, FILE *output)
{
  putc(lower_half * 17, output);
}

void arithmetic_progression(uint8_t lower_half, FILE *input, FILE *output)
{
  uint8_t factor = getc(input);

  fseek(output, -1, SEEK_CUR);
  uint8_t value = getc(output);

  lower_half++;
  while (lower_half--) {
    putc(value += factor, output);
  }
}

void geometric_progression(uint8_t lower_half, FILE *input, FILE *output)
{
  uint8_t factor = getc(input);

  fseek(output, -1, SEEK_CUR);
  uint8_t value = getc(output);

  lower_half++;
  while (lower_half--) {
    putc(value *= factor, output);
  }
}

void fibonacci_progression(uint8_t lower_half, FILE *input, FILE *output)
{
  fseek(output, -2, SEEK_CUR);
  uint8_t first = getc(output);
  uint8_t second = getc(output);
  uint8_t next;

  lower_half++;
  while (lower_half--) {
    next = first + second;
    first = second;
    second = next;
    putc(next, output);
  }
}

void shift_left(uint8_t lower_half, FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  uint8_t value = getc(output);

  lower_half++;
  while (lower_half--) {
    value = (value << 1) | (value >> 7);
    putc(value, output);
  }
}

void shift_right(uint8_t lower_half, FILE *input, FILE *output)
{
  fseek(output, -1, SEEK_CUR);
  uint8_t value = getc(output);

  lower_half++;
  while (lower_half--) {
    value = (value >> 1) | (value << 7);
    putc(value, output);
  }
}

void offset_segment(uint8_t lower_half, FILE *input, FILE *output)
{
  uint8_t i = getc(input) + 1;
  lower_half <<= 4;

  uint8_t ch;
  while (i--) {
    ch = getc(input);
    putc((ch >> 4) + lower_half, output);
    putc((ch & 0xF) + lower_half, output);
  }
}

void jumping_segment(uint8_t lower_half, FILE *input, FILE *output)
{
  uint8_t i = getc(input) + 1;
  lower_half <<= 4;
  lower_half += 8;

  uint8_t ch;
  while (i--) {
    ch = getc(input);

    lower_half += (ch >> 4) - 8 + ((ch >> 4) > 7);
    putc(lower_half, output);

    lower_half += (ch & 0xF) - 8 + ((ch & 0xF) > 7);
    putc(lower_half, output);
  }
}

void create_decompress_dictionary(FILE *input)
{
  decompress_dictionary_size = (getc(input) & 0x0F) << 8;
  decompress_dictionary_size += getc(input);
  decompress_dictionary = malloc(decompress_dictionary_size * sizeof(decompress_dictionary_item));

  FILE *tmp = tmpfile();
  decompress_dictionary_item item;
  for (uint16_t i = 0; i < decompress_dictionary_size; ++i) {
    item.length = (getc(input) << 8) + getc(input);

    if (item.length & 0x8000) {
      item.length &= 0x7FFF;

      rewind(tmp);
      for (size_t end_pos = ftell(input) + item.length; ftell(input) < end_pos;) {
        uint8_t ch = getc(input);
        DECOMPRESS_FUNCTIONS[ch >> 4](ch & 0x0F, input, tmp);
      }

      item.length = ftell(tmp);
      item.data = malloc(item.length * sizeof(uint8_t));

      rewind(tmp);
      fread(item.data, 1, item.length, tmp);
    } else {
      item.data = malloc(item.length * sizeof(uint8_t));
      fread(item.data, 1, item.length, input);
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

  int16_t ch;
  while ((ch = getc(input)) != EOF) {
    DECOMPRESS_FUNCTIONS[ch >> 4](ch & 0x0F, input, output);
  }

  delete_decompress_dictionary();
}
