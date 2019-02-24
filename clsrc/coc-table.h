#ifndef COC_TABLE_H
#define COC_TABLE_H

typedef struct {
  char *make;
  char *model;
  double coc;
} coc_table_s;

void list_all (const char *make, const char *model);
double coc_table_coc (const int idx);
  
#endif  /* COC_TABLE_H */
