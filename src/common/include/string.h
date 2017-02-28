#ifndef _STRING_H_
#define _STRING_H_

char *get_token(char **str);
int streq(const char *s1, const char *s2);
int strlen(const char *s);
int strcpy(char *dst, const char *src);
int strtoi(char *str, int strict, byte *err);

#endif
