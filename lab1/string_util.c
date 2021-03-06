#include "string_util.h"

int string_cmp(const char * string_a, const char * string_b)
{
  /* Return 0 if not the same, idx if the same */
  for(int idx = 0; ; ++idx)
  {
    if(string_a[idx] != string_b[idx])
    {
      return 0;
    }
    if(string_a[idx] == '\0') /* implicit string_b[idx] == '\0' */
    {
      return idx;
    }
  }
}

void string_strip(char * string, char c)
{
  int idx = 0;
  /* search to end of string */
  while(string[idx] != '\0')
  {
    ++idx;
  }
  idx -= 1;

  for(; idx >= 0; --idx)
  {
    if(string[idx] == c)
    {
      string[idx] = '\0';
    }
    else
    {
      break;
    }
  }
}

void string_concat(char * a, const char * b)
{
  /* a = a + b */

  /* find end of a */
  int idx_a_end = 0;
  while(a[idx_a_end] != '\0')
  {
    ++idx_a_end;
  }

  int idx_b = 0;
  for(; b[idx_b] != '\0'; ++idx_b)
  {
    a[idx_a_end++] = b[idx_b];
  }
  a[idx_a_end] = '\0';

  return;
}

int string_length(const char * s)
{
  /* excluding null byte */
  int length = 0;
  while(s[length] != '\0')
  {
    ++length;
  }
  return length;
}

void string_reverse_sequence(const char * src, char * dst, int size)
{
  /* Including null byte */

  int dst_idx = 0;
  for(int reverse_idx = size - 2; reverse_idx >= 0; --reverse_idx)
  {
    dst[dst_idx++] = src[reverse_idx];
  }
  dst[dst_idx] = '\0';
}

void string_longlong_to_char(char * string, const long long i)
{
  char output_buffer[128];
  int string_idx = 0;
  long long process_integer = i;

  if(i == 0)
  {
    string[0] = '0';
    string[1] = '\0';
    return;
  }

  if(i < 0)
  {
    process_integer *= -1;
  }

  while(process_integer != 0)
  {
    output_buffer[string_idx] = NUM_TO_CHAR(process_integer % 10);
    process_integer /= 10;
    ++string_idx;
  }

  if(i < 0)
  {
    output_buffer[string_idx] = '-';
    ++string_idx;
  }
  output_buffer[string_idx++] = '\0';

  string_reverse_sequence(output_buffer, string, string_idx);

  return;
}

void string_float_to_char(char * string, const float f)
{
  char longlong_buffer[64];
  char float_buffer[64];

  long long f_longlong = (long long)f;
  string_longlong_to_char(longlong_buffer, f_longlong);

  float f_float = f - (float)f_longlong;
  long long f_float_long = (long long)(f_float * 10000000);
  string_longlong_to_char(float_buffer, f_float_long);
  string_strip(float_buffer, '0');

  if(f < 0)
  {
    string[0] = '-';
    string[1] = '\0';
  }
  else
  {
    string[0] = '\0';
  }
  string_concat(string, longlong_buffer);
  string_concat(string, ".");
  string_concat(string, float_buffer);
}

