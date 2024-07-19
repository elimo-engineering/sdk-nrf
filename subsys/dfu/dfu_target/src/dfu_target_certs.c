

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>
#include "certs.h"

LOG_MODULE_REGISTER(dfu_target_certs, CONFIG_DFU_TARGET_LOG_LEVEL);

#define CERTS_HEADER_MAGIC  0x656c696d

#define CERTS_STREAM_ID     "certs"

#define IS_ALIGNED_32(POINTER) (((uintptr_t)(const void *)(POINTER)) % 4 == 0)

static uint8_t *stream_buf;
static size_t stream_buf_len;
static size_t stream_buf_bytes;


bool dfu_target_certs_identify(const void *const buf)
{
	/* certs binary blob starts with 4 byte magic word */
	return *((const uint32_t *)buf) == CERTS_HEADER_MAGIC;
}

int dfu_target_certs_set_buf(uint8_t *buf, size_t len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	if (!IS_ALIGNED_32(buf)) {
		return -EINVAL;
	}

	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_certs_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
  ARG_UNUSED(img_num);
	const struct device *flash_dev;
	int err;

	stream_buf_bytes = 0;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	/* Erase certs DFU partition. We do it just before downloading the image in order to keep 
	a valid copy of previous credentials (even if not used at the moment). 
	In future it'll be possible to revert to previous credentials if desired.
	Therefore leave them in memory until the very last moment when new credentials are downloaded. */
	if (false == certs_erase_dfu_partition()) {
		LOG_ERR("FAIL to erase DFU certs partition!");
	}
  
	if (file_size > certs_get_size()) {
		LOG_ERR("Requested file too big to fit in certs_partition");
		return -EFBIG;
	}

	flash_dev = certs_get_flash_device();
	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Failed to get device");
		return -EFAULT;
	}

	err = dfu_target_stream_init(&(struct dfu_target_stream_init){
		.id = CERTS_STREAM_ID,
		.fdev = flash_dev,
		.buf = stream_buf,
		.len = stream_buf_len,
		.offset = certs_get_offset(),
		.size = certs_get_size(),
		.cb = NULL });
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	return 0;
}

int dfu_target_certs_offset_get(size_t *out)
{
	int err = 0;

	err = dfu_target_stream_offset_get(out);
	if (err == 0) {
		*out += stream_buf_bytes;
	}

	return err;
}

int dfu_target_certs_write(const void *const buf, size_t len)
{
	stream_buf_bytes = (stream_buf_bytes + len) % stream_buf_len;

	return dfu_target_stream_write(buf, len);
}

int dfu_target_certs_done(bool successful)
{
	int err = 0;

	err = dfu_target_stream_done(successful);
	if (err != 0) {
		LOG_ERR("dfu_target_stream_done error %d", err);
		return err;
	}

	if (successful) {
		stream_buf_bytes = 0;
	} else {
		LOG_INF("certs binary blob upgrade aborted.");
	}

	return err;
}

int dfu_target_certs_schedule_update(int img_num)
{
	/* nothing to here as credentials are loaded on power up (after reboot) */
	return 0;
}

int dfu_target_certs_reset(void)
{
	stream_buf_bytes = 0;
	return dfu_target_stream_reset();
}
