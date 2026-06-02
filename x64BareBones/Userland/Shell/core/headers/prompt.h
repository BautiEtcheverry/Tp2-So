#ifndef PROMPT_H
#define PROMPT_H
#include <stdint.h>

void     prompt(void);
void     prompt_set_fg(uint32_t rgb);   // lo llama textColor
uint32_t prompt_get_fg(void);           

#endif