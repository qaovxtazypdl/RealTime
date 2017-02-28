#include <string.h>

/* Returns 0 if strings are equal, 1 otherwise. */
int streq(const char *s1, const char *s2) {
	while((*s1 == *s2) && *s1 && *s2) {
		s1++;
		s2++;
	}

	return (*s1 != '\0' || *s2 != '\0');
}

int strlen(const char *s) {
	int i = 0;

	if(s == NULL)
		return 0;

	for(i = 0;*s++ != '\0';i++);
	return i;
}

/* Returns the length of the string (excluding the null character) */
int strcpy(char *dst, const char *src) {
	int sz = 0;
	while((*dst++ = *src++))
		sz++;
	return sz;
}

/* Converts the given string to an integer, if strict is set then expect	    */
/* the string to consist eclusively of numeric characters (otherwise error is set). */
  
int strtoi(char *str, int strict, byte *err) {
  int res = 0;
  *err = 0;
  
  while(*str) {
    if(*str >= '0' && *str <= '9') {
      res *= 10;
      res += *str & 0xf;
    } else if (strict) {
      *err = 1;
      return 0;
    }
    
    str++;
  }
  
  return res;
}

/* Chop the first token (the first non-whitespace block) off of the given string and return it. Returns NULL when 
 * the string has been exhausted. */
char *get_token(char **str) {
  char *token;

  while(**str && **str == ' ') (*str)++; /* Strip leading whitespace */
  token = *str;
  if(*token == '\0') {
	  *str = NULL;
	  return NULL;
  }
  while(**str && **str != ' ') (*str)++;
  
  if(**str != '\0') {
    **str = '\0';
    (*str)++;
  }
  
  return token;
}
