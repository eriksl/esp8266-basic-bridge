#ifndef i2c_h
#define i2c_h

#include "util.h"

#include <stdint.h>

enum
{
	i2c_busses = 5 // 0 -> 1, 1 -> 2, ... and non-multiplexed bus = 0
};

typedef enum
{
	i2c_state_invalid = 0,
	i2c_state_idle,
	i2c_state_header_send,
	i2c_state_start_send,
	i2c_state_address_send,
	i2c_state_address_ack_receive,
	i2c_state_address_ack_received,
	i2c_state_data_send_data,
	i2c_state_data_send_ack_receive,
	i2c_state_data_send_ack_received,
	i2c_state_data_receive_data,
	i2c_state_data_receive_ack_send,
	i2c_state_stop_send,
	i2c_state_error,
	i2c_state_size = i2c_state_error
} i2c_state_t;

assert_size(i2c_state_t, 4);

typedef enum
{
	i2c_error_ok = 0,
	i2c_error_no_init,
	i2c_error_invalid_state_not_idle,
	i2c_error_invalid_state_idle,
	i2c_error_invalid_state_not_send_start,
	i2c_error_invalid_state_not_send_header,
	i2c_error_invalid_state_not_send_address_or_data,
	i2c_error_invalid_state_not_receive_ack,
	i2c_error_invalid_state_not_send_ack,
	i2c_error_bus_lock,
	i2c_error_sda_stuck,
	i2c_error_address_nak,
	i2c_error_data_nak,
	i2c_error_device_error_1,
	i2c_error_device_error_2,
	i2c_error_device_error_3,
	i2c_error_device_error_4,
	i2c_error_device_error_5,
	i2c_error_invalid_bus,
	i2c_error_out_of_range,
	i2c_error_disabled,
	i2c_error_secondary,
	i2c_error_in_use,
	i2c_error_in_use_on_bus_0,
	i2c_error_overflow,
	i2c_error_error,
	i2c_error_size = i2c_error_error
} i2c_error_t;

assert_size(i2c_error_t, 4);

typedef struct attr_packed
{
	unsigned int multiplexer:1;
	unsigned int buses:7;
} i2c_info_t;

assert_size(i2c_info_t, 1);

void i2c_error_format_string(string_t *dst, i2c_error_t error);

#define i2c_log(__prefix__,__error__) \
do { \
	static roflash const char prefix_flash[] = __prefix__; \
	string_new(, string_error, 128); \
	string_append_cstr_flash(&string_error, prefix_flash); \
	i2c_error_format_string(&string_error, __error__); \
	string_append_cstr(&string_error, "\n"); \
	log_from_flash(string_to_cstr(&string_error)); \
} while(0)

void		i2c_init(unsigned int sda_index, unsigned int scl_index);
void		i2c_speed_delay(unsigned int speed_delay);
i2c_error_t	i2c_select_bus(unsigned int bus);
void		i2c_get_info(i2c_info_t *);

i2c_error_t	i2c_send(int address, int length, const uint8_t *bytes);
i2c_error_t	i2c_receive(int address, int length, uint8_t *bytes);
i2c_error_t	i2c_send_receive(int address, int sendlength, const uint8_t *sendbytes, int receivelength, uint8_t *receivebytes);

i2c_error_t	i2c_send1(int address, int byte0);
i2c_error_t	i2c_send2(int address, int byte0, int byte1);
i2c_error_t	i2c_send3(int address, int byte0, int byte1, int byte2);
i2c_error_t	i2c_send4(int address, int byte0, int byte1, int byte2, int byte3);

i2c_error_t	i2c_send1_receive(int address, int byte0, int receivelength, uint8_t *receivebytes);

i2c_error_t i2c_reset(void);

#endif
