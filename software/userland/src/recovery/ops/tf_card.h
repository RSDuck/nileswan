#ifndef __OPS_TF_CARD_H__
#define __OPS_TF_CARD_H__

#include <stddef.h>
#include <wonderful.h>
#include <ws.h>
#include <nilefs.h>

extern FATFS fs;

bool op_tf_card_init(bool force);
bool op_tf_card_test(void);
bool op_tf_card_benchmark_read(void);
bool op_tf_card_benchmark_write(void);

#endif
