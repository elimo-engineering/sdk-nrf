
#ifndef DFU_TARGET_CERTS_H__
#define DFU_TARGET_CERTS_H__

#include <stddef.h>
#include <dfu/dfu_target.h>

#ifdef __cplusplus
extern "C" {
#endif


int dfu_target_certs_set_buf(uint8_t *buf, size_t len);


bool dfu_target_certs_identify(const void *const buf);


int dfu_target_certs_init(size_t file_size, int img_num, dfu_target_callback_t cb);


int dfu_target_certs_offset_get(size_t *offset);


int dfu_target_certs_write(const void *const buf, size_t len);


int dfu_target_certs_done(bool successful);


int dfu_target_certs_schedule_update(int img_num);


int dfu_target_certs_reset(void);


#ifdef __cplusplus
}
#endif

#endif 

