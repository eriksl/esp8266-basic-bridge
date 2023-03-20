#include "io_gpio.h"
#include "io_aux.h"
#include "io_mcp.h"
#include "io_pcf.h"
#include "io_ledpixel.h"
#include "display_cfa634.h"
#include "io.h"
#include "i2c.h"
#include "config.h"
#include "util.h"
#include "sys_time.h"
#include "sequencer.h"
#include "dispatch.h"
#include "remote_trigger.h"
#include "spi.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
	uint32_t value;
	const char name[16];
} io_flag_name_t;

assert_size(io_flag_name_t, 20);

roflash static const io_flag_name_t io_flag_names[] =
{
	{	io_flag_autostart,		"autostart"		},
	{	io_flag_repeat,			"repeat"		},
	{	io_flag_pullup,			"pullup"		},
	{	io_flag_reset_on_read,	"reset-on-read"	},
	{	io_flag_extended,		"extended"		},
	{	io_flag_grb,			"grb"			},
	{	io_flag_linear,			"linear",		},
	{	io_flag_fill8,			"fill8",		},
	{	io_flag_invert,			"invert",		},
	{	io_flag_none,			""				},
};

io_config_pin_entry_t io_config[io_id_size][max_pins_per_io];

roflash static const io_info_t io_info =
{
	{
		io_id_gpio, /* = 0 */
		0x00,
		0,
		16,
		caps_input_digital |
			caps_counter |
			caps_output_digital |
			caps_output_pwm1 |
			caps_output_pwm2 |
			caps_i2c |
			caps_uart |
			caps_ledpixel |
			caps_pullup |
			caps_rotary_encoder |
			caps_spi,
		"Internal GPIO",
		io_gpio_init,
		(void *)0, // postinit
		io_gpio_pin_max_value,
		(void *)0, // periodic slow
		(void *)0, // periodic fast
		io_gpio_init_pin_mode,
		io_gpio_get_pin_info,
		io_gpio_read_pin,
		io_gpio_write_pin,
		io_gpio_set_mask,
	},
	{
		io_id_aux,/* = 1 */
		0x01,
		0,
		2,
		caps_input_digital |
			caps_counter |
			caps_output_digital |
			caps_input_analog,
		"Auxilliary GPIO (RTC+ADC)",
		io_aux_init,
		(void *)0, // postinit
		io_aux_pin_max_value,
		(void *)0, // periodic slow
		io_aux_periodic_fast,
		io_aux_init_pin_mode,
		io_aux_get_pin_info,
		io_aux_read_pin,
		io_aux_write_pin,
		(void *)0, // set_mask
	},
	{
		io_id_mcp_20, /* = 2 */
		0x20,
		io_mcp_instance_20,
		16,
		caps_input_digital |
			caps_counter |
			caps_output_digital |
			caps_pullup |
			caps_rotary_encoder,
		"MCP23017 I2C I/O expander #1",
		io_mcp_init,
		(void *)0, // postinit
		io_mcp_pin_max_value,
		(void *)0, // periodic slow
		io_mcp_periodic_fast,
		io_mcp_init_pin_mode,
		io_mcp_get_pin_info,
		io_mcp_read_pin,
		io_mcp_write_pin,
		io_mcp_set_mask,
	},
	{
		io_id_mcp_21, /* = 3 */
		0x21,
		io_mcp_instance_21,
		16,
		caps_input_digital |
			caps_counter |
			caps_output_digital |
			caps_pullup |
			caps_rotary_encoder,
		"MCP23017 I2C I/O expander #2",
		io_mcp_init,
		(void *)0, // postinit
		io_mcp_pin_max_value,
		(void *)0, // periodic slow
		io_mcp_periodic_fast,
		io_mcp_init_pin_mode,
		io_mcp_get_pin_info,
		io_mcp_read_pin,
		io_mcp_write_pin,
		io_mcp_set_mask,
	},
	{
		io_id_mcp_22, /* = 4 */
		0x22,
		io_mcp_instance_22,
		16,
		caps_input_digital |
			caps_counter |
			caps_output_digital |
			caps_pullup |
			caps_rotary_encoder,
		"MCP23017 I2C I/O expander #3",
		io_mcp_init,
		(void *)0, // postinit
		io_mcp_pin_max_value,
		(void *)0, // periodic slow,
		io_mcp_periodic_fast,
		io_mcp_init_pin_mode,
		io_mcp_get_pin_info,
		io_mcp_read_pin,
		io_mcp_write_pin,
		io_mcp_set_mask,
	},
	{
		io_id_pcf_3a, /* = 5 */
		0x3a,
		io_pcf_instance_3a,
		8,
		caps_input_digital |
			caps_counter |
			caps_output_digital |
			caps_rotary_encoder,
		"PCF8574A I2C I/O expander",
		io_pcf_init,
		(void *)0, // postinit
		io_pcf_pin_max_value,
		(void *)0, // periodic slow
		io_pcf_periodic_fast,
		io_pcf_init_pin_mode,
		(void *)0, // get pin info
		io_pcf_read_pin,
		io_pcf_write_pin,
		io_pcf_set_mask,
	},
	{
		io_id_ledpixel, /* = 6 */
		0x00,
		0,
		16,
		caps_output_pwm1,
		"led string",
		io_ledpixel_init,
		io_ledpixel_post_init,
		io_ledpixel_pin_max_value,
		(void *)0, // periodic slow
		(void *)0, // periodic fast
		io_ledpixel_init_pin_mode,
		(void *)0, // get pin info
		io_ledpixel_read_pin,
		io_ledpixel_write_pin,
		(void *)0, // set_mask // FIXME this can be implemented, but may not be very useful
	},
	{
		io_id_pcf_26, /* = 7 */
		0x26,
		io_pcf_instance_26,
		8,
		caps_input_digital |
			caps_counter |
			caps_output_digital |
			caps_rotary_encoder,
		"PCF8574A I2C I/O expander",
		io_pcf_init,
		(void *)0, // postinit
		io_pcf_pin_max_value,
		(void *)0, // periodic slow
		io_pcf_periodic_fast,
		io_pcf_init_pin_mode,
		(void *)0, // get pin info
		io_pcf_read_pin,
		io_pcf_write_pin,
		io_pcf_set_mask,
	},
};

static io_data_t io_data;

typedef struct
{
	attr_flash_align	uint32_t	mode;
	attr_flash_align	const char	*short_name;
	attr_flash_align	const char	*long_name;
} io_mode_trait_t;

assert_size(io_mode_trait_t, 12);

roflash static const io_mode_trait_t io_mode_traits[io_pin_size] =
{
	{ io_pin_disabled,			"disabled",		"disabled"				},
	{ io_pin_input_digital,		"dinput",		"digital input"			},
	{ io_pin_counter,			"counter",		"counter"				},
	{ io_pin_output_digital,	"doutput",		"digital output"		},
	{ io_pin_timer,				"timer",		"timer (blink)"			},
	{ io_pin_input_analog,		"ainput",		"analog input (adc)"	},
	{ io_pin_output_pwm1,		"pwm",			"primary pwm output"	},
	{ io_pin_i2c,				"i2c",			"i2c"					},
	{ io_pin_uart,				"uart",			"uart"					},
	{ io_pin_lcd,				"lcd",			"lcd"					},
	{ io_pin_trigger,			"trigger",		"trigger"				},
	{ io_pin_ledpixel,			"ledpixel",		"ledpixel control"		},
	{ io_pin_cfa634,			"cfa634",		"crystalfontz cfa634"	},
	{ io_pin_output_pwm2,		"pwm2",			"secondary pwm output"	},
	{ io_pin_rotary_encoder,	"renc",			"rotary encoder input"	},
	{ io_pin_spi,				"spi",			"spi"					},
};

static io_pin_mode_t io_mode_from_string(const string_t *src)
{
	unsigned int ix;
	const io_mode_trait_t *entry;

	for(ix = 0; ix < io_pin_size; ix++)
	{
		entry = &io_mode_traits[ix];

		if(string_match_cstr(src, entry->short_name))
			return(entry->mode);
	}

	return(io_pin_error);
}

static void io_string_from_mode(string_t *name, io_pin_mode_t mode, int pad)
{
	unsigned int ix;
	const io_mode_trait_t *entry;

	for(ix = 0; ix < io_pin_size; ix++)
	{
		entry = &io_mode_traits[ix];

		if(entry->mode == mode)
		{
			if(pad > 0)
				string_format(name, "%20s", entry->long_name);
			else
				if(pad < 0)
					string_format(name, "%-20s", entry->long_name);
				else
					string_format(name, "%s", entry->long_name);

			return;
		}
	}

	string_append(name, "error");
}

static void io_strings_from_modes(string_t *dst)
{
	unsigned int ix;
	const io_mode_trait_t *entry;

	for(ix = 0; ix < io_pin_size; ix++)
	{
		entry = &io_mode_traits[ix];

		if(ix != 0)
			string_append(dst, "/");

		string_append_cstr(dst, entry->short_name);
	}
}

typedef struct
{
	const io_pin_ll_mode_t	mode;
	const char				*name;
} io_ll_mode_trait_t;

roflash static const io_ll_mode_trait_t io_ll_mode_traits[io_pin_ll_size] =
{
	{ io_pin_ll_disabled,			"disabled"			},
	{ io_pin_ll_input_digital,		"digital input"		},
	{ io_pin_ll_counter,			"counter"			},
	{ io_pin_ll_output_digital,		"digital output"	},
	{ io_pin_ll_input_analog,		"analog input"		},
	{ io_pin_ll_output_pwm1,		"pwm output"		},
	{ io_pin_ll_i2c,				"i2c"				},
	{ io_pin_ll_uart,				"uart"				},
	{ io_pin_ll_output_pwm2,		"pwm2 output"		},
	{ io_pin_ll_spi,				"spi"				},
	{ io_pin_ll_i2s,				"i2s"				},
};

void io_string_from_ll_mode(string_t *name, io_pin_ll_mode_t mode, int pad)
{
	unsigned int ix;
	const io_ll_mode_trait_t *entry;

	for(ix = 0; ix < io_pin_size; ix++)
	{
		entry = &io_ll_mode_traits[ix];

		if(entry->mode == mode)
		{
			if(pad > 0)
				string_format(name, "%14s", entry->name);
			else
				if(pad < 0)
					string_format(name, "%-14s", entry->name);
				else
					string_format(name, "%s", entry->name);
			return;
		}
	}

	string_append(name, "error");
}

static io_i2c_t io_i2c_pin_from_string(const string_t *pin)
{
	if(string_match_cstr(pin, "sda"))
		return(io_i2c_sda);
	else if(string_match_cstr(pin, "scl"))
		return(io_i2c_scl);
	else
		return(io_i2c_error);
}

static void io_string_from_i2c_type(string_t *name, io_i2c_t type)
{
	switch(type)
	{
		case(io_i2c_sda): { string_append(name, "sda"); break; }
		case(io_i2c_scl): { string_append(name, "scl"); break; }
		default: { string_append(name, "error"); break; }
	}
}

typedef struct
{
	io_renc_pin_t	mode;
	const char		*name;
} io_renc_pin_trait_t;

static io_renc_pin_trait_t io_renc_pin_traits[io_renc_size] =
{
	{ io_renc_unset,	"unset"	},
	{ io_renc_1a,		"1a"	},
	{ io_renc_1b,		"1b"	},
	{ io_renc_2a,		"2a"	},
	{ io_renc_2b,		"2b"	},
};

static io_renc_pin_t io_renc_pin_from_string(const string_t *src)
{
	unsigned int ix;
	const io_renc_pin_trait_t *entry;

	for(ix = 0; ix < io_renc_size; ix++)
	{
		entry = &io_renc_pin_traits[ix];

		if(string_match_cstr(src, entry->name))
			return(entry->mode);
	}

	return(io_renc_error);
}

static void io_string_from_renc_pin(string_t *name, io_renc_pin_t mode)
{
	unsigned int ix;
	const io_renc_pin_trait_t *entry;

	for(ix = 0; ix < io_renc_size; ix++)
	{
		entry = &io_renc_pin_traits[ix];

		if(entry->mode == mode)
		{
			string_format(name, "%s", entry->name);
			return;
		}
	}

	string_append(name, "error");
}

typedef struct
{
	const io_lcd_mode_t	mode;
	const char			*name;
} io_lcd_mode_trait_t;

roflash static const io_lcd_mode_trait_t io_lcd_mode_traits[io_lcd_size] =
{
	{ io_lcd_rs,	"rs"	},
	{ io_lcd_rw,	"rw"	},
	{ io_lcd_e,		"e"		},
	{ io_lcd_d0,	"d0"	},
	{ io_lcd_d1,	"d1"	},
	{ io_lcd_d2,	"d2"	},
	{ io_lcd_d3,	"d3"	},
	{ io_lcd_d4,	"d4"	},
	{ io_lcd_d5,	"d5"	},
	{ io_lcd_d6,	"d6"	},
	{ io_lcd_d7,	"d7"	},
	{ io_lcd_bl,	"bl"	}
};

static io_lcd_mode_t io_lcd_mode_from_string(const string_t *src)
{
	unsigned int ix;
	const io_lcd_mode_trait_t *entry;

	for(ix = 0; ix < io_lcd_size; ix++)
	{
		entry = &io_lcd_mode_traits[ix];

		if(string_match_cstr(src, entry->name))
			return(entry->mode);
	}

	return(io_lcd_error);
}

static void io_string_from_lcd_mode(string_t *name, io_lcd_mode_t mode)
{
	unsigned int ix;
	const io_lcd_mode_trait_t *entry;

	for(ix = 0; ix < io_lcd_size; ix++)
	{
		entry = &io_lcd_mode_traits[ix];

		if(entry->mode == mode)
		{
			string_format(name, "%s", entry->name);
			return;
		}
	}

	string_append(name, "error");
}

typedef struct
{
	const io_trigger_t	id;
	const char			*name;
} io_trigger_action_t;

roflash static const io_trigger_action_t io_trigger_action[io_trigger_size] =
{
	{ io_trigger_none,		"none"		},
	{ io_trigger_off,		"off"		},
	{ io_trigger_on,		"on"		},
	{ io_trigger_down,		"down"		},
	{ io_trigger_up,		"up"		},
	{ io_trigger_toggle,	"toggle"	},
	{ io_trigger_stop,		"stop"		},
	{ io_trigger_start,		"start"		},
};

static io_trigger_t string_to_trigger_action(const string_t *src)
{
	unsigned int ix;
	const io_trigger_action_t *entry;

	for(ix = 0; ix < io_trigger_size; ix++)
	{
		entry = &io_trigger_action[ix];

		if(string_match_cstr(src, entry->name))
			return(entry->id);
	}

	return(io_trigger_error);
}

static void trigger_actions_to_string(string_t *dst)

{
	unsigned int ix;
	const io_trigger_action_t *entry;

	for(ix = 0; ix < io_trigger_size; ix++)
	{
		entry = &io_trigger_action[ix];

		if(ix > 0)
			string_append(dst, ", ");

		string_format(dst, "%s", entry->name);
	}
}

static void trigger_action_to_string(string_t *name, io_trigger_t id)
{
	unsigned int ix;
	const io_trigger_action_t *entry;

	for(ix = 0; ix < io_trigger_size; ix++)
	{
		entry = &io_trigger_action[ix];

		if(entry->id == id)
		{
			string_format(name, "%s", entry->name);
			return;
		}
	}

	string_append(name, "error");
}

static void trigger_usage(string_t *dst)
{
	string_append(dst, "usage: io-trigger <io> <pin> <action>\n");
	string_append(dst, "    action: ");
	trigger_actions_to_string(dst);
}

static void iomode_trigger_usage(string_t *dst, const char *info)
{
	string_append(dst, "usage: io-mode <io> <pin> trigger <action1> <io1> <pin1> [<action2> <io2> <pin2>]\n");
	string_append(dst, "    action: ");
	trigger_actions_to_string(dst);
	string_format(dst, "\nerror in <%s>\n", info);
}

static bool io_string_to_flags(const string_t *flag, io_config_pin_entry_t *pin_config, bool resetset)
{
	const io_flag_name_t *entry;

	for(entry = io_flag_names; entry->value != io_flag_none; entry++)
	{
		if(string_match_cstr_flash(flag, entry->name))
		{
			if(resetset)
				pin_config->flags |= entry->value;
			else
				pin_config->flags &= ~entry->value;

			return(true);
		}
	}

	return(false);
}

static void io_flags_to_string(string_t *dst, const io_config_pin_entry_t *pin_config)
{
	const io_flag_name_t *entry;
	bool first = true;

	for(entry = io_flag_names; entry->value != io_flag_none; entry++)
		if(pin_config->flags & entry->value)
		{
			if(first)
				first = false;
			else
				string_append_char(dst, ' ');

			string_append_cstr_flash(dst, entry->name);
		}
}

static unsigned int io_pin_max_value_x(const io_info_entry_t *info, io_data_pin_entry_t *pin_data, const io_config_pin_entry_t *pin_config, int pin)
{
	if((pin_config->mode == io_pin_disabled) || (pin_config->mode == io_pin_error))
		return(0);

	if(!info->pin_max_value_fn)
		return(0);

	return(info->pin_max_value_fn(info, pin_data, pin_config, pin));
}

static io_error_t io_read_pin_x(string_t *errormsg, const io_info_entry_t *info, io_data_pin_entry_t *pin_data, const io_config_pin_entry_t *pin_config, int pin, unsigned int *value)
{
	io_error_t error;

	switch(pin_config->mode)
	{
		case(io_pin_disabled):
		case(io_pin_error):
		case(io_pin_cfa634):
		case(io_pin_spi):
		{
			if(errormsg)
				string_append(errormsg, "cannot read from this pin");

			return(io_error);
		}

		case(io_pin_counter):
		{
			*value = pin_data->value;
			break;
		}

		default:
		{
			if(!info->read_pin_fn)
			{
				if(errormsg)
					string_append(errormsg, "cannot read from this pin");

				return(io_error);
			}

			if((error = info->read_pin_fn(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
				return(error);

			break;
		}
	}

	return(io_ok);
}

static io_error_t io_write_pin_x(string_t *errormsg, const io_info_entry_t *info, io_data_pin_entry_t *pin_data, io_config_pin_entry_t *pin_config, int pin, uint32_t value)
{
	io_error_t error;

	switch(pin_config->mode)
	{
		case(io_pin_disabled):
		case(io_pin_error):
		case(io_pin_cfa634):
		case(io_pin_spi):
		{
			if(errormsg)
				string_append(errormsg, "cannot write to this pin");

			return(io_error);
		}

		case(io_pin_counter):
		{
			pin_data->value = value;
			break;
		}

		case(io_pin_ledpixel):
		{
			if(io_ledpixel_pinmask(value) != io_ok)
			{
				if(errormsg)
					string_append(errormsg, "cannot access this ledpixel pin");

				return(io_error);
			}

			break;
		}

		default:
		{
			if(!info->write_pin_fn)
			{
				if(errormsg)
					string_append(errormsg, "cannot write to this pin");

				return(io_error);
			}

			if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
				return(error);

			break;
		}
	}

	return(io_ok);
}

static io_error_t io_set_mask_x(string_t *errormsg, const io_info_entry_t *info, unsigned int mask, unsigned int pins)
{
	if(!info->set_mask_fn)
	{
		if(errormsg)
			string_append(errormsg, "set mask operation not supported on this io");

		return(io_error);
	}

	return(info->set_mask_fn(errormsg, info, mask, pins));
}

static io_error_t io_trigger_pin_x(string_t *errormsg, const io_info_entry_t *info, io_data_pin_entry_t *pin_data, io_config_pin_entry_t *pin_config, int pin, io_trigger_t trigger_type)
{
	io_error_t error;
	unsigned int old_value, trigger;
	unsigned int value = 0;

	switch(pin_config->mode)
	{
		case(io_pin_output_digital):
		case(io_pin_lcd):
		{
			switch(trigger_type)
			{
				case(io_trigger_off):
				case(io_trigger_stop):
				case(io_trigger_down):
				{
					if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, 0)) != io_ok)
						return(error);

					break;
				}

				case(io_trigger_on):
				case(io_trigger_start):
				case(io_trigger_up):
				{
					if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, 1)) != io_ok)
						return(error);

					break;
				}

				default:
				{
					if(errormsg)
						string_append(errormsg, "invalid trigger type");

					return(io_error);
				}
			}

			break;
		}

		case(io_pin_counter):
		{
			switch(trigger_type)
			{
				case(io_trigger_down):
				{
					if((error = info->read_pin_fn(errormsg, info, pin_data, pin_config, pin, &value)) != io_ok)
						return(error);

					if(value > 0)
						value--;

					if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
						return(error);

					break;
				}

				case(io_trigger_up):
				{
					if((error = info->read_pin_fn(errormsg, info, pin_data, pin_config, pin, &value)) != io_ok)
						return(error);

					value++;

					if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
						return(error);

					break;
				}

				default:
				{
					if(errormsg)
						string_append(errormsg, "invalid trigger type");

					return(io_error);
				}
			}

			break;
		}

		case(io_pin_timer):
		{
			switch(trigger_type)
			{
				case(io_trigger_off):
				case(io_trigger_stop):
				{
					value = pin_config->direction == io_dir_up ? 1 : 0;

					if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
						return(error);

					pin_data->speed = 0;
					pin_data->direction = io_dir_none;

					break;
				}

				case(io_trigger_on):
				case(io_trigger_start):
				{
					value = pin_config->direction == io_dir_up ? 0 : 1;

					if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
						return(error);

					pin_data->speed = pin_config->speed;
					pin_data->direction = pin_config->direction;

					break;
				}

				default:
				{
					if(errormsg)
						string_append(errormsg, "invalid trigger type");

					return(io_error);
				}
			}

			break;
		}

		case(io_pin_output_pwm1):
		case(io_pin_output_pwm2):
		{
			if((error = info->read_pin_fn(errormsg, info, pin_data, pin_config, pin, &value)) != io_ok)
				return(error);

			if(trigger_type == io_trigger_toggle)
			{
				if(value == 0)
					trigger_type = io_trigger_on;
				else
					trigger_type = io_trigger_off;
			}

			switch(trigger_type)
			{
				case(io_trigger_off):
				{
					pin_data->direction = io_dir_none;
					pin_data->speed = 0;

					if(value > 0)
						pin_data->saved_value = value;

					value = 0;

					break;
				}

				case(io_trigger_on):
				{
					if(value == 0)
					{
						if(pin_data->saved_value > 0)
							value = pin_data->saved_value;
						else
							value = pin_config->shared.output_pwm.upper_bound;

						pin_data->saved_value = 0;
					}

					break;
				}

				case(io_trigger_stop):
				{
					pin_data->direction = io_dir_none;
					pin_data->speed = 0;

					break;
				}

				case(io_trigger_start):
				{
					if(pin_data->direction == io_dir_none)
					{
						pin_data->direction = io_dir_up;
						pin_data->speed = pin_config->speed;
						value = pin_data->saved_value;
						pin_data->saved_value = 0;
					}

					break;
				}

				case(io_trigger_down):
				{
					old_value = value;

					if(pin_config->flags & io_flag_linear)
					{
						if(value >= pin_config->speed)
							value -= pin_config->speed;
					}
					else
					{
						value /= (pin_config->speed / 10000.0) + 1;

						if((old_value == value) && (value > 0))
							value--;
					}

					if(value <= pin_config->shared.output_pwm.lower_bound)
					{
						if((pin_config->flags & io_flag_repeat) && (pin_data->direction == io_dir_down))
							pin_data->direction = io_dir_up;
						else
						{
							value = 0;
							pin_data->direction = io_dir_none;
						}
					}

					break;
				}

				case(io_trigger_up):
				{
					if(value < pin_config->shared.output_pwm.lower_bound)
						value = pin_config->shared.output_pwm.lower_bound;
					else
					{
						old_value = value;

						if(pin_config->flags & io_flag_linear)
							value += pin_config->speed;
						else
						{
							value *= (pin_config->speed / 10000.0) + 1;

							if(old_value == value)
								value++;
						}
					}

					if(value >= pin_config->shared.output_pwm.upper_bound)
					{
						value = pin_config->shared.output_pwm.upper_bound;

						if(pin_data->direction == io_dir_up)
							pin_data->direction = io_dir_down;
					}

					break;
				}

				default:
				{
					if(errormsg)
						string_append(errormsg, "invalid trigger type");

					return(io_error);
				}
			}

			if((error = info->write_pin_fn(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
				return(error);

			break;
		}

		case(io_pin_trigger):
		{
			for(trigger = 0; trigger < 2; trigger++)
			{
				if((pin_config->shared.trigger[trigger].io.io >= 0) &&
						(pin_config->shared.trigger[trigger].io.pin >= 0) &&
						(pin_config->shared.trigger[trigger].action != io_trigger_off))
				{
					if(io_trigger_pin(errormsg,
							pin_config->shared.trigger[trigger].io.io,
							pin_config->shared.trigger[trigger].io.pin,
							pin_config->shared.trigger[trigger].action) != io_ok)
						return(io_error);
				}
			}

			break;
		}

		default:
		{
			if(errormsg)
				string_append(errormsg, "cannot trigger this pin");

			return(io_error);
		}

	}

	return(io_ok);
}

unsigned int io_pin_max_value(unsigned int io, unsigned int pin)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;

	if(io >= io_id_size)
		return(0);

	info = &io_info[io];
	data = &io_data[io];

	if(!data->detected)
		return(io_error);

	if(pin >= info->pins)
		return(0);

	pin_config = &io_config[io][pin];
	pin_data = &data->pin[pin];

	return(io_pin_max_value_x(info, pin_data, pin_config, pin));
}

io_error_t io_read_pin(string_t *error_msg, unsigned int io, unsigned int pin, unsigned int *value)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;
	io_error_t error;

	if(io >= io_id_size)
	{
		if(error_msg)
			string_append(error_msg, "io out of range\n");
		return(io_error);
	}

	info = &io_info[io];
	data = &io_data[io];

	if(!data->detected)
	{
		if(error_msg)
			string_append(error_msg, "io not available\n");
		return(io_error);
	}

	if(pin >= info->pins)
	{
		if(error_msg)
			string_append(error_msg, "pin out of range\n");
		return(io_error);
	}

	pin_config = &io_config[io][pin];
	pin_data = &data->pin[pin];

	if(((error = io_read_pin_x(error_msg, info, pin_data, pin_config, pin, value)) != io_ok) && error_msg)
		string_append(error_msg, "\n");
	else
		if((pin_config->flags & io_flag_reset_on_read) &&
				((pin_config->mode == io_pin_counter) || (pin_config->mode == io_pin_rotary_encoder)))
			error = io_write_pin_x(error_msg, info, pin_data, pin_config, pin, 0);

	return(error);
}

io_error_t io_write_pin(string_t *error, unsigned int io, unsigned int pin, unsigned int value)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;

	if(io >= io_id_size)
	{
		if(error)
			string_append(error, "io out of range\n");
		return(io_error);
	}

	info = &io_info[io];
	data = &io_data[io];

	if(!data->detected)
	{
		if(error)
			string_append(error, "io not available\n");
		return(io_error);
	}

	if(pin >= info->pins)
	{
		if(error)
			string_append(error, "pin out of range\n");
		return(io_error);
	}

	pin_config = &io_config[io][pin];
	pin_data = &data->pin[pin];

	return(io_write_pin_x(error, info, pin_data, pin_config, pin, value));
}

io_error_t io_set_mask(string_t *error, int io, unsigned int mask, unsigned int pins)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;

	if(io >= io_id_size)
	{
		if(error)
			string_append(error, "io out of range\n");
		return(io_error);
	}

	info = &io_info[io];
	data = &io_data[io];

	if(!data->detected)
	{
		if(error)
			string_append(error, "io not available\n");
		return(io_error);
	}

	return(io_set_mask_x(error, info, mask, pins));
}

io_error_t io_trigger_pin(string_t *error, unsigned int io, unsigned int pin, io_trigger_t trigger_type)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;

	if(io >= io_id_size)
	{
		if(error)
			string_append(error, "io out of range\n");
		return(io_error);
	}

	info = &io_info[io];
	data = &io_data[io];

	if(!data->detected)
	{
		if(error)
			string_append(error, "io not available\n");
		return(io_error);
	}

	if(pin >= info->pins)
	{
		if(error)
			string_append(error, "pin out of range\n");
		return(io_error);
	}

	pin_config = &io_config[io][pin];
	pin_data = &data->pin[pin];

	return(io_trigger_pin_x(error, info, pin_data, pin_config, pin, trigger_type));
}

io_error_t io_traits(string_t *errormsg, unsigned int io, unsigned int pin, io_pin_mode_t *pinmode, unsigned int *lower_bound, unsigned int *upper_bound, int *step, unsigned int *value)
{
	io_error_t error;
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;

	if(io >= io_id_size)
	{
		if(errormsg)
			string_append(errormsg, "io out of range\n");
		return(io_error);
	}

	info = &io_info[io];
	data = &io_data[io];

	if(pin >= info->pins)
	{
		if(errormsg)
			string_append(errormsg, "pin out of range\n");
		return(io_error);
	}

	pin_config = &io_config[io][pin];
	pin_data = &data->pin[pin];

	*pinmode = pin_config->mode;

	switch(pin_config->mode)
	{
		case(io_pin_disabled):
		case(io_pin_error):
		{
			if(errormsg)
				string_append(errormsg, "pin disabled");
			return(io_error);
		}

		case(io_pin_output_pwm1):
		case(io_pin_output_pwm2):
		{
			*lower_bound	= pin_config->shared.output_pwm.lower_bound;
			*upper_bound	= pin_config->shared.output_pwm.upper_bound;
			*step			= pin_config->speed;

			if((error = io_read_pin_x(errormsg, info, pin_data, pin_config, pin, value)) != io_ok)
				return(error);

			break;
		}

		default:
		{
			if(errormsg)
				string_append(errormsg, "no info for this type");
			return(io_error);
		}
	}

	return(io_ok);
}

void io_init(void)
{
	string_new(, error, 64);
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;
	io_pin_flag_to_int_t flags;
	unsigned int io, pin;
	unsigned int mode, llmode;
	int i2c_sda = -1;
	int i2c_scl = -1;
	unsigned int i2c_set_speed_delay;
	int trigger;
	int trigger_io, trigger_pin, trigger_type;
	unsigned int spi_pin;
	uint64_t start = time_get_us();

	for(io = 0; io < io_id_size; io++)
	{
		info = &io_info[io];
		data = &io_data[io];

		spi_pin = 0;

		for(pin = 0; pin < info->pins; pin++)
		{
			pin_data = &data->pin[pin];
			pin_data->direction = io_dir_none;
			pin_data->speed = 0;
			pin_data->value = 0;
			pin_data->saved_value = 0;
			pin_data->previous = 0;

			pin_config = &io_config[io][pin];

			mode = io_pin_disabled;
			llmode = io_pin_ll_disabled;

			if(!config_get_uint("io.%u.%u.mode", &mode, io, pin))
				mode = io_pin_disabled;

			if(!config_get_uint("io.%u.%u.llmode", &llmode, io, pin))
				llmode = io_pin_ll_disabled;

			if(!config_get_uint("io.%u.%u.flags", &flags.intvalue, io, pin))
				flags.intvalue = 0;

			if((mode == io_pin_disabled) || (llmode == io_pin_ll_disabled))
			{
				if((io == 0) && (pin == 1))		// emergency mode if no config present, make sure serial output works
				{
					mode = io_pin_uart;
					llmode = io_pin_ll_uart;
				}
				else
				{
					mode = io_pin_disabled;
					llmode = io_pin_ll_disabled;
				}
			}

			pin_config->mode = mode;
			pin_config->llmode = llmode;
			pin_config->flags = flags.io_pin_flags;

			switch(mode)
			{
				case(io_pin_rotary_encoder):
				{
					unsigned int pin_type;
					int remote_index;

					if(!config_get_uint("io.%u.%u.renc.pintype", &pin_type, io, pin))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					if((pin_type == io_renc_1b) || (pin_type == io_renc_2b))
					{
						unsigned int partner_pin;
						io_renc_pin_t partner_pin_type = (pin_type == io_renc_1b) ? io_renc_1a : io_renc_2a;

						for(partner_pin = 0; partner_pin < pin; partner_pin++)
							if((io_config[io][partner_pin].mode == io_pin_rotary_encoder) &&
									(io_config[io][partner_pin].shared.renc.pin_type == partner_pin_type))
								break;

						if(partner_pin < pin)
						{
							io_config[io][partner_pin].shared.renc.partner = pin;
							pin_config->shared.renc.partner = partner_pin;
						}
					}

					if(config_get_int("io.%u.%u.renc.trigger_pin.io", &trigger_io, io, pin) &&
						config_get_int("io.%u.%u.renc.trigger_pin.pin", &trigger_pin, io, pin))
					{
						pin_config->shared.renc.trigger_pin.io = trigger_io;
						pin_config->shared.renc.trigger_pin.pin = trigger_pin;

						if(config_get_int("io.%u.%u.renc.remote", &remote_index, io, pin))
							pin_config->shared.renc.trigger_pin.remote = remote_index;
						else
							pin_config->shared.renc.trigger_pin.remote = -1;
					}
					else
					{
						pin_config->shared.renc.trigger_pin.io = -1;
						pin_config->shared.renc.trigger_pin.pin = -1;
						pin_config->shared.renc.trigger_pin.remote = -1;
					}

					pin_config->shared.renc.pin_type = pin_type;

					break;
				}

				case(io_pin_trigger):
				{
					if(!(info->caps & caps_counter))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					for(trigger = 0; trigger < max_triggers_per_pin; trigger++)
					{
						pin_config->shared.trigger[trigger].io.io = -1;
						pin_config->shared.trigger[trigger].io.pin = -1;
						pin_config->shared.trigger[trigger].action = io_trigger_none;
					}

					if(config_get_int("io.%u.%u.trigger.io", &trigger_io, io, pin) &&
						config_get_int("io.%u.%u.trigger.pin", &trigger_pin, io, pin) &&
						config_get_int("io.%u.%u.trigger.type", &trigger_type, io, pin))
					{
						pin_config->shared.trigger[0].io.io = trigger_io;
						pin_config->shared.trigger[0].io.pin = trigger_pin;
						pin_config->shared.trigger[0].action = trigger_type;
					}

					if(config_get_int("io.%u.%u.trigger.0.io", &trigger_io, io, pin) &&
						config_get_int("io.%u.%u.trigger.0.pin", &trigger_pin, io, pin) &&
						config_get_int("io.%u.%u.trigger.0.type", &trigger_type, io, pin))
					{
						pin_config->shared.trigger[0].io.io = trigger_io;
						pin_config->shared.trigger[0].io.pin = trigger_pin;
						pin_config->shared.trigger[0].action = trigger_type;
					}

					if(config_get_int("io.%u.%u.trigger.1.io", &trigger_io, io, pin) &&
						config_get_int("io.%u.%u.trigger.1.pin", &trigger_pin, io, pin) &&
						config_get_int("io.%u.%u.trigger.1.type", &trigger_type, io, pin))
					{
						pin_config->shared.trigger[1].io.io = trigger_io;
						pin_config->shared.trigger[1].io.pin = trigger_pin;
						pin_config->shared.trigger[1].action = trigger_type;
					}

					break;
				}

				case(io_pin_timer):
				{
					unsigned int direction, speed;

					if(!(info->caps & caps_output_digital))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					if(!config_get_uint("io.%u.%u.timer.delay", &speed, io, pin))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					if(!config_get_uint("io.%u.%u.timer.direction", &direction, io, pin))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					pin_config->speed = speed;
					pin_config->direction = direction;

					break;
				}

				case(io_pin_output_pwm1):
				{
					unsigned int speed, lower_bound, upper_bound;

					if(!(info->caps & caps_output_pwm1))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					if(!config_get_uint("io.%u.%u.outputa.speed", &speed, io, pin))
						speed = 0;

					if(!config_get_uint("io.%u.%u.outputa.lower", &lower_bound, io, pin))
						lower_bound = 0;

					if(!config_get_uint("io.%u.%u.outputa.upper", &upper_bound, io, pin))
						upper_bound = ~0;

					pin_config->shared.output_pwm.lower_bound = lower_bound;
					pin_config->shared.output_pwm.upper_bound = upper_bound;
					pin_config->speed = speed;

					llmode = io_pin_ll_output_pwm1;

					break;
				}

				case(io_pin_output_pwm2):
				{
					unsigned int speed, lower_bound, upper_bound;

					if(!(info->caps & caps_output_pwm2))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					if(!config_get_uint("io.%u.%u.outputa.speed", &speed, io, pin))
						speed = 0;

					if(!config_get_uint("io.%u.%u.outputa.lower", &lower_bound, io, pin))
						lower_bound = 0;

					if(!config_get_uint("io.%u.%u.outputa.upper", &upper_bound, io, pin))
						upper_bound = ~0;

					pin_config->shared.output_pwm.lower_bound = lower_bound;
					pin_config->shared.output_pwm.upper_bound = upper_bound;
					pin_config->speed = speed;

					llmode = io_pin_ll_output_pwm2;

					break;
				}

				case(io_pin_i2c):
				{
					unsigned int pin_mode;

					if(!(info->caps & caps_i2c))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					if(!config_get_uint("io.%u.%u.i2c.pinmode", &pin_mode, io, pin))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					pin_config->shared.i2c.pin_mode = pin_mode;

					break;
				}

				case(io_pin_lcd):
				{
					unsigned int pin_mode;

					if(!config_get_uint("io.%u.%u.lcd.pin", &pin_mode, io, pin))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					pin_config->shared.lcd.pin_use = pin_mode;

					break;
				}

				case(io_pin_ledpixel):
				{
					if(!(info->caps & caps_ledpixel) || (io_ledpixel_mode(io, pin) == ledpixel_invalid))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					break;
				}

				case(io_pin_cfa634):
				{
					if(!(info->caps & caps_uart))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					break;
				}

				case(io_pin_spi):
				{
					if(!(info->caps & caps_spi))
					{
						pin_config->mode = io_pin_disabled;
						pin_config->llmode = io_pin_ll_disabled;
						continue;
					}

					spi_pin++;

					break;
				}

				default:
				{
					break;
				}
			}
		}

		if(info->init_fn(info) == io_ok)
		{
			data->detected = true;

			for(pin = 0; pin < info->pins; pin++)
			{
				pin_config = &io_config[io][pin];
				pin_data = &data->pin[pin];

				if(!info->init_pin_mode_fn || (info->init_pin_mode_fn((string_t *)0, info, pin_data, pin_config, pin) == io_ok))
				{
					switch(pin_config->mode)
					{
						case(io_pin_output_digital):
						case(io_pin_lcd):
						{
							if(pin_config->flags & io_flag_autostart)
								io_trigger_pin_x((string_t *)0, info, pin_data, pin_config, pin, io_trigger_on);
							else
								io_trigger_pin_x((string_t *)0, info, pin_data, pin_config, pin, io_trigger_off);

							break;
						}

						case(io_pin_timer):
						case(io_pin_output_pwm1):
						case(io_pin_output_pwm2):
						{
							if(pin_config->flags & io_flag_autostart)
								io_trigger_pin_x((string_t *)0, info, pin_data, pin_config, pin, io_trigger_start);
							else
							{
								io_trigger_pin_x((string_t *)0, info, pin_data, pin_config, pin, io_trigger_stop);
								io_trigger_pin_x((string_t *)0, info, pin_data, pin_config, pin, io_trigger_off);
							}
							break;
						}

						case(io_pin_i2c):
						{
							if(pin_config->shared.i2c.pin_mode == io_i2c_sda)
								i2c_sda = pin;

							if(pin_config->shared.i2c.pin_mode == io_i2c_scl)
								i2c_scl = pin;

							if((i2c_sda >= 0) && (i2c_scl >= 0))
							{
								if(!config_get_uint("i2c.speed_delay", &i2c_set_speed_delay, -1, -1))
									i2c_set_speed_delay = 1000;
								i2c_init(i2c_sda, i2c_scl);
								i2c_speed_delay(i2c_set_speed_delay);
							}

							break;
						}

						case(io_pin_ledpixel):
						{
							io_ledpixel_pre_init(io, pin);

							break;
						}

						case(io_pin_cfa634):
						{
							display_cfa634_setup(io, pin);

							break;
						}

						default:
						{
							break;
						}
					}
				}
			}
		}

		if((spi_pin > 2) && !spi_init(&error, io))
			log("io: %s\n", string_to_cstr(&error));
	}

	sequencer_init();
	remote_trigger_init();

	stat_init_io_time_us = time_get_us() - start;
}

void io_pin_changed(unsigned int io, unsigned int pin, uint32_t pin_value_mask)
{
	const io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;
	unsigned int trigger;
	int remote_trigger;
	unsigned int partner_pin;
	unsigned int primary_pin, secondary_pin;
	unsigned int state;
	io_trigger_t trigger_action;

	if(io >= io_id_size)
	{
		log("[io] pin_change: io invalid\n");
		return;
	}

	if(pin >= io_pin_size)
	{
		log("[io] pin_change: pin invalid\n");
		return;
	}

	if(!io_data[io].detected)
	{
		log("[io] pin_change: io inactive\n");
		return;
	}

	pin_config = &io_config[io][pin];
	pin_data = &io_data[io].pin[pin];

	switch(pin_config->mode)
	{
		case(io_pin_counter):
		{
			if((!(pin_config->flags & io_flag_invert)) != !!(pin_value_mask & (1 << pin)))
				pin_data->value++;

			break;
		}

		case(io_pin_trigger): // FIXME: add remote trigger to normal triggers
		{
			if((!(pin_config->flags & io_flag_invert)) != !!(pin_value_mask & (1 << pin)))
				for(trigger = 0; trigger < max_triggers_per_pin; trigger++)
					if(pin_config->shared.trigger[trigger].action != io_trigger_none)
						io_trigger_pin((string_t *)0,
								pin_config->shared.trigger[trigger].io.io,
								pin_config->shared.trigger[trigger].io.pin,
								pin_config->shared.trigger[trigger].action);

			break;
		}

		case(io_pin_rotary_encoder):
		{
			partner_pin = pin_config->shared.renc.partner;

			if(partner_pin >= io_pin_size)
			{
				log("[io] pin_change: invalid partner pin\n");
				break;
			}

			if(pin > partner_pin)
			{
				primary_pin = partner_pin;
				secondary_pin = pin;
			}
			else
			{
				primary_pin = pin;
				secondary_pin = partner_pin;
			}

			state =
				(io_data[io].pin[primary_pin].previous << 3) |
				(io_data[io].pin[secondary_pin].previous << 2) |
				((!!(pin_value_mask & (1 << primary_pin))) << 1) |
				((!!(pin_value_mask & (1 << secondary_pin))) << 0);

			trigger_action = io_trigger_none;

			switch(state)
			{
				case(0b1101):
				case(0b0100):
				case(0b0010):
				case(0b1011):
				{
					trigger_action = io_trigger_up;
					break;
				}

				case(0b1000):
				case(0b0001):
				case(0b0111):
				case(0b1110):
				{
					trigger_action = io_trigger_down;
					break;
				}

				default:
				{
#if 0
					string_new(, state_string, 32);
					string_clear(&state_string);
					string_word_to_bin(&state_string, state, 4);
					log("invalid state: %s\n", string_to_cstr(&state_string));
#endif
					stat_renc_invalid_state++;
					break;
				}
			}

			if(trigger_action != io_trigger_none)
			{
				if((remote_trigger = pin_config->shared.renc.trigger_pin.remote) >= 0)
					remote_trigger_add((unsigned int)remote_trigger,
							pin_config->shared.renc.trigger_pin.io,
							pin_config->shared.renc.trigger_pin.pin,
							trigger_action);
				else
					io_trigger_pin((string_t *)0,
							pin_config->shared.renc.trigger_pin.io,
							pin_config->shared.renc.trigger_pin.pin,
							trigger_action);
			}

			break;
		}

		default:
		{
			log("[io] pin change on invalid pin type: %u\n", pin);
			log("   io: %u pin: %u value mask: %x\n", io, pin, pin_value_mask); // FIXME
			break;
		}
	}

	if((trigger_alert.io >= 0) && (trigger_alert.pin >= 0))
		io_trigger_pin((string_t *)0, trigger_alert.io, trigger_alert.pin, io_trigger_on);

	pin_data->previous = !!(pin_value_mask & (1 << pin));
}

iram void io_periodic_fast(unsigned int rate_ms)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_config_pin_entry_t *pin_config;
	io_data_pin_entry_t *pin_data;
	unsigned int io, pin;
	io_trigger_t trigger_action;

	for(io = 0; io < io_id_size; io++)
	{
		info = &io_info[io];
		data = &io_data[io];

		if(!data->detected)
			continue;

		if(info->periodic_fast_fn)
			info->periodic_fast_fn(io, info, data, rate_ms);

		for(pin = 0; pin < info->pins; pin++)
		{
			pin_config = &io_config[io][pin];
			pin_data = &data->pin[pin];

			switch(pin_config->mode)
			{
				case(io_pin_timer):
				{
					if(pin_data->direction != io_dir_none)
					{
						if(pin_data->speed > rate_ms)
							pin_data->speed -= rate_ms;
						else
						{
							pin_data->speed = 0;

							switch(pin_data->direction)
							{
								case(io_dir_up):
								{
									info->write_pin_fn((string_t *)0, info, pin_data, pin_config, pin, 1);
									pin_data->direction = io_dir_down;
									break;
								}

								case(io_dir_down):
								{
									info->write_pin_fn((string_t *)0, info, pin_data, pin_config, pin, 0);
									pin_data->direction = io_dir_up;
									break;
								}

								case(io_dir_none):
								{
									break;
								}
							}

							if(pin_config->flags & io_flag_repeat)
								pin_data->speed = pin_config->speed;
							else
							{
								pin_data->speed = 0;
								pin_data->direction = io_dir_none;
							}
						}
					}

					break;
				}

				case(io_pin_output_pwm1):
				case(io_pin_output_pwm2):
				{
					if((pin_config->shared.output_pwm.upper_bound > pin_config->shared.output_pwm.lower_bound) &&
							(pin_config->speed > 0) &&
							(pin_data->direction != io_dir_none))
					{
						trigger_action = (pin_data->direction == io_dir_up) ? io_trigger_up : io_trigger_down;
						io_trigger_pin((string_t *)0, io, pin, trigger_action);
					}

					break;
				}

				default:
				{
					break;
				}
			}
		}
	}

	if((sequencer_get_repeats() > 0) && ((time_get_us() / 1000) > sequencer_get_current_end_time()))
		dispatch_post_task(task_prio_high, task_run_sequencer, 0, 0, 0);
}

void io_periodic_slow(unsigned int rate_ms)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	unsigned int io;
	static bool post_init_run = false;

	for(io = 0; io < io_id_size; io++)
	{
		data = &io_data[io];

		if(!data->detected)
			continue;

		info = &io_info[io];

		if(!post_init_run && info->post_init_fn)
			info->post_init_fn(info);

		if(info->periodic_slow_fn)
			info->periodic_slow_fn(io, info, data, rate_ms);
	}

	post_init_run = true;
}

/* app commands */

app_action_t application_function_io_mode(app_params_t *parameters)
{
	const io_info_entry_t	*info;
	io_data_entry_t			*data;
	io_config_pin_entry_t	*pin_config;
	io_data_pin_entry_t		*pin_data;
	io_pin_mode_t			mode;
	io_pin_ll_mode_t		llmode;
	unsigned int			io, pin;
	int						trigger_io, trigger_pin;
	io_trigger_t			trigger_type;

	if(parse_uint(1, parameters->src, &io, 0, ' ') != parse_ok)
	{
		io_config_dump(parameters->dst, -1, -1, false);
		return(app_action_normal);
	}

	if(io >= io_id_size)
	{
		string_format(parameters->dst, "invalid io %u\n", io);
		return(app_action_error);
	}

	info = &io_info[io];
	data = &io_data[io];

	if(!data->detected)
	{
		string_format(parameters->dst, "io %u not detected\n", io);
		return(app_action_error);
	}

	if(parse_uint(2, parameters->src, &pin, 0, ' ') != parse_ok)
	{
		io_config_dump(parameters->dst, io, -1, false);
		return(app_action_normal);
	}

	if(pin >= info->pins)
	{
		string_append(parameters->dst, "io pin out of range\n");
		return(app_action_error);
	}

	pin_config = &io_config[io][pin];
	pin_data = &data->pin[pin];

	if(parse_string(3, parameters->src, parameters->dst, ' ') != parse_ok)
	{
		string_clear(parameters->dst);
		io_config_dump(parameters->dst, io, pin, false);
		return(app_action_normal);
	}

	if((mode = io_mode_from_string(parameters->dst)) == io_pin_error)
	{
		string_clear(parameters->dst);
		string_append(parameters->dst, "invalid mode, available modes: ");
		io_strings_from_modes(parameters->dst);
		string_append(parameters->dst, "\n");
		return(app_action_error);
	}

	string_clear(parameters->dst);

	llmode = io_pin_ll_error;

	if(!config_open_write())
	{
		string_append(parameters->dst, "config write failure (open)\n");
		return(app_action_error);
	}

	switch(mode)
	{
		case(io_pin_input_digital):
		{
			if(!(info->caps & caps_input_digital))
			{
				config_abort_write();
				string_append(parameters->dst, "digital input mode invalid for this io\n");
				return(app_action_error);
			}

			llmode = io_pin_ll_input_digital;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_input_digital, io, pin);

			break;
		}

		case(io_pin_counter):
		{
			if(!(info->caps & caps_counter))
			{
				config_abort_write();
				string_append(parameters->dst, "counter mode invalid for this io\n");
				return(app_action_error);
			}

			llmode = io_pin_ll_counter;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_counter, io, pin);

			break;
		}

		case(io_pin_rotary_encoder):
		{
			io_renc_pin_t pin_type;
			int trigger_remote_index = -1;
			unsigned int partner_pin;

			if(!(info->caps & caps_rotary_encoder))
			{
				string_append(parameters->dst, "rotary encoder mode invalid for this io\n");
				goto renc_error1;
			}

			if(parse_string(4, parameters->src, parameters->dst, ' ') != parse_ok)
				goto renc_error;

			if((pin_type = io_renc_pin_from_string(parameters->dst)) == io_renc_error)
				goto renc_error;

			string_clear(parameters->dst);

			if((parse_int(5, parameters->src, &trigger_io, 0, ' ') == parse_ok) && (parse_int(6, parameters->src, &trigger_pin, 0, ' ') == parse_ok))
				if(parse_int(7, parameters->src, &trigger_remote_index, 0, ' ') == parse_ok)
					(void)0;
				else
					trigger_remote_index = -1;
			else
			{
				trigger_io = -1;
				trigger_pin = -1;
			}

			if((pin_type == io_renc_1b) || (pin_type == io_renc_2b))
			{
				io_renc_pin_t partner_pin_type = (pin_type == io_renc_1b) ? io_renc_1a : io_renc_2a;

				for(partner_pin = 0; partner_pin < pin; partner_pin++)
					if((io_config[io][partner_pin].mode == io_pin_rotary_encoder) &&
							(io_config[io][partner_pin].shared.renc.pin_type == partner_pin_type))
						break;

				if(partner_pin >= pin)
				{
					string_append(parameters->dst, "rotary encoder: no matching 'a' pin for 'b' pin on this io\n");
					goto renc_error1;
				}

				io_config[io][partner_pin].shared.renc.partner = pin;
				trigger_io = io_config[io][partner_pin].shared.renc.trigger_pin.io;
				trigger_pin = io_config[io][partner_pin].shared.renc.trigger_pin.pin;
				trigger_remote_index = io_config[io][partner_pin].shared.renc.trigger_pin.remote;
				pin_config->shared.renc.partner = partner_pin;
			}

			pin_config->shared.renc.pin_type = pin_type;
			pin_config->shared.renc.trigger_pin.remote = trigger_remote_index;
			pin_config->shared.renc.trigger_pin.io = trigger_io;
			pin_config->shared.renc.trigger_pin.pin = trigger_pin;
			llmode = io_pin_ll_counter;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_counter, io, pin);
			config_set_int("io.%u.%u.renc.pintype", pin_type, io, pin);

			if(trigger_remote_index >= 0)
				config_set_int("io.%u.%u.renc.remote", trigger_remote_index, io, pin);

			if((trigger_io >= 0) && (trigger_pin >= 0))
			{
				config_set_int("io.%u.%u.renc.trigger_pin.io", trigger_io, io, pin);
				config_set_int("io.%u.%u.renc.trigger_pin.pin", trigger_pin, io, pin);
			}

			break;
renc_error:
			string_clear(parameters->dst);
			string_append(parameters->dst, "rotary encoder: <pin mode> [<trigger io> <trigger pin> [<remote index>]], <pin mode>=1a|1b|2a|2b\n");
renc_error1:
			config_abort_write();
			return(app_action_error);
		}

		case(io_pin_trigger):
		{
			if(!(info->caps & caps_counter))
			{
				config_abort_write();
				string_append(parameters->dst, "trigger mode invalid for this io\n");
				return(app_action_error);
			}

			if((parse_string(4, parameters->src, parameters->dst, ' ') != parse_ok))
			{
				config_abort_write();
				string_clear(parameters->dst);
				iomode_trigger_usage(parameters->dst, "action 1");
				return(app_action_error);
			}

			if((trigger_type = string_to_trigger_action(parameters->dst)) == io_trigger_error)
			{
				config_abort_write();
				string_clear(parameters->dst);
				iomode_trigger_usage(parameters->dst, "action 2");
				return(app_action_error);
			}

			if((parse_int(5, parameters->src, &trigger_io, 0, ' ') != parse_ok))
			{
				config_abort_write();
				iomode_trigger_usage(parameters->dst, "io");
				return(app_action_error);
			}

			if((parse_int(6, parameters->src, &trigger_pin, 0, ' ') != parse_ok))
			{
				config_abort_write();
				iomode_trigger_usage(parameters->dst, "pin");
				return(app_action_error);
			}

			pin_config->shared.trigger[0].io.io = trigger_io;
			pin_config->shared.trigger[0].io.pin = trigger_pin;
			pin_config->shared.trigger[0].action = trigger_type;

			pin_config->shared.trigger[1].io.io = -1;
			pin_config->shared.trigger[1].io.pin = -1;
			pin_config->shared.trigger[1].action = io_trigger_none;

			if((parse_string(7, parameters->src, parameters->dst, ' ') != parse_ok))
			{
				string_clear(parameters->dst);
				goto skip;
			}

			string_clear(parameters->dst);

			if((trigger_type = string_to_trigger_action(parameters->dst)) == io_trigger_error)
			{
				string_clear(parameters->dst);
				goto skip;
			}

			if((parse_int(8, parameters->src, &trigger_io, 0, ' ') != parse_ok))
				goto skip;

			if((parse_int(9, parameters->src, &trigger_pin, 0, ' ') != parse_ok))
				goto skip;

			pin_config->shared.trigger[1].io.io = trigger_io;
			pin_config->shared.trigger[1].io.pin = trigger_pin;
			pin_config->shared.trigger[1].action = trigger_type;

skip:
			llmode = io_pin_ll_counter;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_counter, io, pin);

			config_set_int("io.%u.%u.trigger.0.io", pin_config->shared.trigger[0].io.io, io, pin);
			config_set_int("io.%u.%u.trigger.0.pin", pin_config->shared.trigger[0].io.pin, io, pin);
			config_set_int("io.%u.%u.trigger.0.type", pin_config->shared.trigger[0].action, io, pin);

			if((pin_config->shared.trigger[1].io.io >= 0) &&
				(pin_config->shared.trigger[1].io.pin >= 0) &&
				(pin_config->shared.trigger[1].action != io_trigger_none))
			{
				config_set_int("io.%u.%u.trigger.1.io", pin_config->shared.trigger[1].io.io, io, pin);
				config_set_int("io.%u.%u.trigger.1.pin", pin_config->shared.trigger[1].io.pin, io, pin);
				config_set_int("io.%u.%u.trigger.1.type", pin_config->shared.trigger[1].action, io, pin);
			}

			break;
		}

		case(io_pin_output_digital):
		{
			if(!(info->caps & caps_output_digital))
			{
				config_abort_write();
				string_append(parameters->dst, "digital output mode invalid for this io\n");
				return(app_action_error);
			}

			llmode = io_pin_ll_output_digital;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_output_digital, io, pin);

			break;
		}

		case(io_pin_timer):
		{
			io_direction_t direction;
			unsigned int speed;

			if(!(info->caps & caps_output_digital))
			{
				config_abort_write();
				string_append(parameters->dst, "timer mode invalid for this io\n");
				return(app_action_error);
			}

			if(parse_string(4, parameters->src, parameters->dst, ' ') != parse_ok)
			{
				config_abort_write();
				string_clear(parameters->dst);
				string_append(parameters->dst, "timer: <direction>:up/down <speed>:ms\n");
				return(app_action_error);
			}

			if(string_match_cstr(parameters->dst, "up"))
				direction = io_dir_up;
			else if(string_match_cstr(parameters->dst, "down"))
				direction = io_dir_down;
			else
			{
				config_abort_write();
				string_append(parameters->dst, ": timer direction invalid\n");
				return(app_action_error);
			}

			string_clear(parameters->dst);

			if((parse_uint(5, parameters->src, &speed, 0, ' ') != parse_ok))
			{
				config_abort_write();
				string_clear(parameters->dst);
				string_append(parameters->dst, "timer: <direction>:up/down <speed>:ms\n");
				return(app_action_error);
			}

			if(speed < 100)
			{
				config_abort_write();
				string_format(parameters->dst, "timer: speed too small: must be >= %d ms\n", 100);
				return(app_action_error);
			}

			pin_config->direction = direction;
			pin_config->speed = speed;

			llmode = io_pin_ll_output_digital;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_output_digital, io, pin);
			config_set_int("io.%u.%u.timer.direction", direction, io, pin);
			config_set_int("io.%u.%u.timer.delay", speed, io, pin);

			break;
		}

		case(io_pin_input_analog):
		{
			if(!(info->caps & caps_input_analog))
			{
				config_abort_write();
				string_append(parameters->dst, "analog input mode invalid for this io\n");
				return(app_action_error);
			}

			llmode = io_pin_ll_input_analog;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_input_analog, io, pin);

			break;
		}

		case(io_pin_output_pwm1):
		{
			unsigned int lower_bound = 0;
			unsigned int upper_bound = 0;
			int speed = 0;

			if(!(info->caps & caps_output_pwm1))
			{
				config_abort_write();
				string_append(parameters->dst, "primary pwm output mode invalid for this io\n");
				return(app_action_error);
			}

			parse_uint(4, parameters->src, &lower_bound, 0, ' ');
			parse_uint(5, parameters->src, &upper_bound, 0, ' ');
			parse_int(6, parameters->src, &speed, 0, ' ');

			if(upper_bound == 0)
				upper_bound = ~0;

			if(upper_bound < lower_bound)
			{
				config_abort_write();
				string_append(parameters->dst, "upper bound below lower bound\n");
				return(app_action_error);
			}

			pin_config->shared.output_pwm.lower_bound = lower_bound;
			pin_config->shared.output_pwm.upper_bound = upper_bound;
			pin_config->speed = speed;

			llmode = io_pin_ll_output_pwm1;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_output_pwm1, io, pin);

			if(speed > 0)
				config_set_int("io.%u.%u.outputa.speed", speed, io, pin);

			if(lower_bound > 0)
				config_set_int("io.%u.%u.outputa.lower", lower_bound, io, pin);

			if(upper_bound < ~0UL)
				config_set_int("io.%u.%u.outputa.upper", upper_bound, io, pin);

			break;
		}

		case(io_pin_output_pwm2):
		{
			unsigned int lower_bound = 0;
			unsigned int upper_bound = 0;
			int speed = 0;

			if(!(info->caps & caps_output_pwm2))
			{
				config_abort_write();
				string_append(parameters->dst, "secondary pwm output mode invalid for this io\n");
				return(app_action_error);
			}

			parse_uint(4, parameters->src, &lower_bound, 0, ' ');
			parse_uint(5, parameters->src, &upper_bound, 0, ' ');
			parse_int(6, parameters->src, &speed, 0, ' ');

			if(upper_bound == 0)
				upper_bound = ~0;

			if(upper_bound < lower_bound)
			{
				config_abort_write();
				string_append(parameters->dst, "upper bound below lower bound\n");
				return(app_action_error);
			}

			pin_config->shared.output_pwm.lower_bound = lower_bound;
			pin_config->shared.output_pwm.upper_bound = upper_bound;
			pin_config->speed = speed;

			llmode = io_pin_ll_output_pwm2;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_output_pwm2, io, pin);

			if(speed > 0)
				config_set_int("io.%u.%u.outputa.speed", speed, io, pin);

			if(lower_bound > 0)
				config_set_int("io.%u.%u.outputa.lower", lower_bound, io, pin);

			if(upper_bound < ~0UL)
				config_set_int("io.%u.%u.outputa.upper", upper_bound, io, pin);

			break;
		}

		case(io_pin_i2c):
		{
			io_i2c_t pin_mode;

			if(!(info->caps & caps_i2c))
			{
				config_abort_write();
				string_append(parameters->dst, "i2c mode invalid for this io\n");
				return(app_action_error);
			}

			if(parse_string(4, parameters->src, parameters->dst, ' ') != parse_ok)
			{
				config_abort_write();
				string_clear(parameters->dst);
				string_append(parameters->dst, "i2c: <pin mode>=sda|scl\n");
				return(app_action_error);
			}

			if((pin_mode = io_i2c_pin_from_string(parameters->dst)) == io_i2c_error)
			{
				config_abort_write();
				string_clear(parameters->dst);
				string_append(parameters->dst, "i2c: <pin mode>=sda|scl\n");
				return(app_action_error);
			}

			string_clear(parameters->dst);

			pin_config->shared.i2c.pin_mode = pin_mode;

			llmode = io_pin_ll_i2c;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_i2c, io, pin);
			config_set_int("io.%u.%u.i2c.pinmode", pin_mode, io, pin);

			break;
		}

		case(io_pin_uart):
		{
			if(!(info->caps & caps_uart))
			{
				config_abort_write();
				string_append(parameters->dst, "uart mode invalid for this io\n");
				return(app_action_error);
			}

			llmode = io_pin_ll_uart;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_uart, io, pin);

			break;
		}

		case(io_pin_lcd):
		{
			io_lcd_mode_t pin_mode;

			if(parse_string(4, parameters->src, parameters->dst, ' ') != parse_ok)
			{
				config_abort_write();
				string_clear(parameters->dst);
				string_append(parameters->dst, "lcd: <pin use>=rs|rw|e|d0|d1|d2|d3|d4|d5|d6|d7|bl\n");
				return(app_action_error);
			}

			if((pin_mode = io_lcd_mode_from_string(parameters->dst)) == io_lcd_error)
			{
				config_abort_write();
				string_clear(parameters->dst);
				string_append(parameters->dst, "lcd: <pin use>=rs|rw|e|d0|d1|d2|d3|d4|d5|d6|d7|bl\n");
				return(app_action_error);
			}

			string_clear(parameters->dst);

			if(pin_mode == io_lcd_bl) // backlight
			{
				if(info->caps & caps_output_pwm2)
					llmode = io_pin_ll_output_pwm2;
				else
					if(info->caps & caps_output_pwm1)
						llmode = io_pin_ll_output_pwm1;
					else
						if(info->caps & caps_output_digital)
							llmode = io_pin_ll_output_digital;
						else
						{
							config_abort_write();
							string_append(parameters->dst, "pwm1/digital output mode invalid for this io\n");
							return(app_action_error);
						}
			}
			else
			{
				if(!(info->caps & caps_output_digital))
				{
					config_abort_write();
					string_append(parameters->dst, "digital output mode invalid for this io\n");
					return(app_action_error);
				}

				llmode = io_pin_ll_output_digital;
			}

			pin_config->shared.lcd.pin_use = pin_mode;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", llmode, io, pin);
			config_set_int("io.%u.%u.lcd.pin", pin_mode, io, pin);

			break;
		}

		case(io_pin_ledpixel):
		{
			if(!(info->caps & caps_ledpixel))
			{
				config_abort_write();
				string_append(parameters->dst, "ledpixel mode invalid for this io\n");
				return(app_action_error);
			}

			switch(io_ledpixel_mode(io, pin))
			{
				case(ledpixel_i2s):
				{
					llmode = io_pin_ll_i2s;
					break;
				}

				case(ledpixel_uart_0):
				case(ledpixel_uart_1):
				{
					llmode = io_pin_ll_uart;
					break;
				}

				default:
				{
					config_abort_write();
					string_append(parameters->dst, "ledpixel mode invalid for this pin\n");
					return(app_action_error);
				}
			}

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", llmode, io, pin);

			break;
		}

		case(io_pin_cfa634):
		{
			if(!(info->caps & caps_uart))
			{
				config_abort_write();
				string_append(parameters->dst, "cfa634 mode invalid for this io (must be an uart)\n");
				return(app_action_error);
			}

			llmode = io_pin_ll_uart;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_uart, io, pin);

			break;
		}

		case(io_pin_spi):
		{
			if(!(info->caps & caps_spi))
			{
				config_abort_write();
				string_append(parameters->dst, "spi mode invalid for this io\n");
				return(app_action_error);
			}

			llmode = io_pin_ll_spi;

			config_delete("io.%u.%u.", true, io, pin);
			config_set_int("io.%u.%u.mode", mode, io, pin);
			config_set_int("io.%u.%u.llmode", io_pin_ll_spi, io, pin);

			break;
		}

		case(io_pin_disabled):
		{
			llmode = io_pin_ll_disabled;

			config_delete("io.%u.%u.", true, io, pin);

			break;
		}

		case(io_pin_error):
		{
			llmode = io_pin_ll_error;

			config_abort_write();
			string_append(parameters->dst, "unsupported io mode\n");
			return(app_action_error);
		}
	}

	config_close_write();

	if((mode == io_pin_error) || (llmode == io_pin_ll_error))
	{
		string_append(parameters->dst, "error\n");
		return(app_action_error);
	}

	pin_config->mode = mode;
	pin_config->llmode = llmode;

	if(info->init_pin_mode_fn && (info->init_pin_mode_fn(parameters->dst, info, pin_data, pin_config, pin) != io_ok))
	{
		pin_config->mode = io_pin_disabled;
		pin_config->llmode = io_pin_ll_disabled;
		return(app_action_error);
	}

	io_config_dump(parameters->dst, io, pin, false);

	return(app_action_normal);
}

app_action_t application_function_io_read(app_params_t *parameters)
{
	const io_info_entry_t *info;
	io_config_pin_entry_t *pin_config;
	unsigned int io, pin, value;

	if(parse_uint(1, parameters->src, &io, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-read: <io> <pin>\n");
		return(app_action_error);
	}

	if(io >= io_id_size)
	{
		string_format(parameters->dst, "invalid io %u\n", io);
		return(app_action_error);
	}

	info = &io_info[io];

	if(parse_uint(2, parameters->src, &pin, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "get: <io> <pin>\n");
		return(app_action_error);
	}

	if(pin >= info->pins)
	{
		string_append(parameters->dst, "io pin out of range\n");
		return(app_action_error);
	}

	pin_config = &io_config[io][pin];

	io_string_from_mode(parameters->dst, pin_config->mode, 0);

	if(pin_config->mode == io_pin_i2c)
	{
		string_append(parameters->dst, "/");
		io_string_from_i2c_type(parameters->dst, pin_config->shared.i2c.pin_mode);
	}

	if(pin_config->mode == io_pin_lcd)
	{
		string_append(parameters->dst, "/");
		io_string_from_lcd_mode(parameters->dst, pin_config->shared.lcd.pin_use);
	}

	if(pin_config->mode == io_pin_rotary_encoder)
	{
		string_append(parameters->dst, "/");
		io_string_from_renc_pin(parameters->dst, pin_config->shared.renc.pin_type);
	}

	string_append(parameters->dst, ": ");

	if(io_read_pin(parameters->dst, io, pin, &value) != io_ok)
		return(app_action_error);

	string_format(parameters->dst, "[%u]\n", value);

	return(app_action_normal);
}

app_action_t application_function_io_write(app_params_t *parameters)
{
	const io_info_entry_t *info;
	io_config_pin_entry_t *pin_config;
	unsigned int io, pin, value;

	if(parse_uint(1, parameters->src, &io, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-write <io> <pin> <value>\n");
		return(app_action_error);
	}

	if(io >= io_id_size)
	{
		string_format(parameters->dst, "invalid io %u\n", io);
		return(app_action_error);
	}

	info = &io_info[io];

	if(parse_uint(2, parameters->src, &pin, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-write <io> <pin> <value>\n");
		return(app_action_error);
	}

	if(pin >= info->pins)
	{
		string_append(parameters->dst, "invalid pin\n");
		return(app_action_error);
	}

	pin_config = &io_config[io][pin];

	value = 0;
	parse_uint(3, parameters->src, &value, 0, ' ');

	io_string_from_mode(parameters->dst, pin_config->mode, 0);

	if(pin_config->mode == io_pin_lcd)
	{
		string_append(parameters->dst, "/");
		io_string_from_lcd_mode(parameters->dst, pin_config->shared.lcd.pin_use);
	}

	string_append(parameters->dst, ": ");

	if(io_write_pin(parameters->dst, io, pin, value) != io_ok)
	{
		string_append(parameters->dst, "\n");
		return(app_action_error);
	}

	if(io_read_pin(parameters->dst, io, pin, &value) != io_ok)
	{
		string_append(parameters->dst, "\n");
		return(app_action_error);
	}

	string_format(parameters->dst, "[%u]\n", value);

	return(app_action_normal);
}

app_action_t application_function_io_set_mask(app_params_t *parameters)
{
	unsigned int io, mask, pins;

	if(parse_uint(1, parameters->src, &io, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-set-mask <io> <mask> <pins>\n");
		return(app_action_error);
	}

	if(io >= io_id_size)
	{
		string_format(parameters->dst, "invalid io %u\n", io);
		return(app_action_error);
	}

	if(parse_uint(2, parameters->src, &mask, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-set-mask <io> <mask> <pins>\n");
		return(app_action_error);
	}

	if(parse_uint(3, parameters->src, &pins, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-set-mask <io> <mask> <pins>\n");
		return(app_action_error);
	}

	if(io_set_mask(parameters->dst, io, mask, pins) != io_ok)
	{
		string_append(parameters->dst, "error\n");
		return(app_action_error);
	}

	string_append(parameters->dst, "ok\n");

	return(app_action_normal);
}

app_action_t application_function_io_trigger(app_params_t *parameters)
{
	const io_info_entry_t *info;
	unsigned int io, pin;
	io_trigger_t trigger_type;

	if(parse_uint(1, parameters->src, &io, 0, ' ') != parse_ok)
	{
		string_clear(parameters->dst);
		trigger_usage(parameters->dst);
		return(app_action_normal);
	}

	if(io >= io_id_size)
	{
		string_format(parameters->dst, "invalid io %u\n", io);
		return(app_action_error);
	}

	info = &io_info[io];

	if(parse_uint(2, parameters->src, &pin, 0, ' ') != parse_ok)
	{
		string_clear(parameters->dst);
		trigger_usage(parameters->dst);
		return(app_action_normal);
	}

	if(pin >= info->pins)
	{
		string_append(parameters->dst, "invalid pin\n");
		return(app_action_error);
	}

	if(parse_string(3, parameters->src, parameters->dst, ' ') != parse_ok)
	{
		string_clear(parameters->dst);
		trigger_usage(parameters->dst);
		return(app_action_normal);
	}

	if((trigger_type = string_to_trigger_action(parameters->dst)) == io_trigger_error)
	{
		string_clear(parameters->dst);
		trigger_usage(parameters->dst);
		return(app_action_normal);
	}

	string_clear(parameters->dst);

	string_append(parameters->dst, "trigger ");
	trigger_action_to_string(parameters->dst, trigger_type);
	string_format(parameters->dst, " %u/%u: ", io, pin);

	if(io_trigger_pin(parameters->dst, io, pin, trigger_type) != io_ok)
	{
		string_append(parameters->dst, "\n");
		return(app_action_error);
	}

	string_append(parameters->dst, "ok\n");

	return(app_action_normal);
}

static app_action_t application_function_io_clear_set_flag(app_params_t *parameters, unsigned int value)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_data_pin_entry_t *pin_data;
	io_config_pin_entry_t *pin_config;
	unsigned int io, pin;
	io_pin_flag_t saved_flags;
	io_pin_flag_to_int_t io_pin_flag_to_int;

	if(parse_uint(1, parameters->src, &io, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-flag <io> <pin> <flag>\n");
		return(app_action_error);
	}

	if(io >= io_id_size)
	{
		string_format(parameters->dst, "invalid io %u\n", io);
		return(app_action_error);
	}

	info = &io_info[io];
	data = &io_data[io];

	if(parse_uint(2, parameters->src, &pin, 0, ' ') != parse_ok)
	{
		string_append(parameters->dst, "io-flag <io> <pin> <flag>\n");
		return(app_action_error);
	}

	if(pin >= info->pins)
	{
		string_append(parameters->dst, "invalid pin\n");
		return(app_action_error);
	}

	pin_data = &data->pin[pin];
	pin_config = &io_config[io][pin];

	saved_flags = pin_config->flags;

	if((parse_string(3, parameters->src, parameters->dst, ' ') == parse_ok) && !io_string_to_flags(parameters->dst, pin_config, !!value))
	{
		string_clear(parameters->dst);
		string_append(parameters->dst, "io-flag <io> <pin> <flag>\n");
		return(app_action_error);
	}

	if((pin_config->flags & io_flag_pullup) && !(info->caps & caps_pullup))
	{
		pin_config->flags = saved_flags;
		string_clear(parameters->dst);
		string_append(parameters->dst, "io does not support pullup\n");
		return(app_action_error);
	}

	if(info->init_pin_mode_fn && (info->init_pin_mode_fn(parameters->dst, info, pin_data, pin_config, pin) != io_ok))
	{
		pin_config->flags = saved_flags;
		string_clear(parameters->dst);
		string_append(parameters->dst, "cannot enable this flag\n");
		return(app_action_error);
	}

	io_pin_flag_to_int.io_pin_flags = pin_config->flags;
	config_open_write();
	config_set_int("io.%u.%u.flags", io_pin_flag_to_int.intvalue, io, pin);
	config_close_write();

	string_clear(parameters->dst);
	string_format(parameters->dst, "flags for pin %u/%u:", io, pin);

	io_flags_to_string(parameters->dst, pin_config);

	string_append(parameters->dst, "\n");

	return(app_action_normal);
}

app_action_t application_function_io_set_flag(app_params_t *parameters)
{
	return(application_function_io_clear_set_flag(parameters, 1));
}

app_action_t application_function_io_clear_flag(app_params_t *parameters)
{
	return(application_function_io_clear_set_flag(parameters, 0));
}

/* dump */

typedef enum
{
	ds_id_io,
	ds_id_pin_1,
	ds_id_pin_2,
	ds_id_flags_1,
	ds_id_flags_2,
	ds_id_mode_1,
	ds_id_mode_2,
	ds_id_disabled,
	ds_id_input,
	ds_id_counter,
	ds_id_rotary_encoder_1,
	ds_id_rotary_encoder_2,
	ds_id_trigger_1,
	ds_id_trigger_2,
	ds_id_trigger_3,
	ds_id_output,
	ds_id_timer,
	ds_id_pwm1_output,
	ds_id_pwm2_output,
	ds_id_i2c_sda,
	ds_id_i2c_scl,
	ds_id_uart,
	ds_id_ledpixel,
	ds_id_cfa634,
	ds_id_lcd,
	ds_id_spi,
	ds_id_unknown,
	ds_id_max_value,
	ds_id_info_1,
	ds_id_info_2,
	ds_id_pins_header,
	ds_id_hw_1,
	ds_id_hw_2,
	ds_id_table_start,
	ds_id_table_end,
	ds_id_error,

	ds_id_length,
	ds_id_invalid = ds_id_length
} dump_string_id_t;

typedef const char string_array_t[ds_id_length][256];

typedef struct {
	const string_array_t plain;
	const string_array_t html;
} dump_string_t;

static const roflash dump_string_t roflash_dump_strings =
{
	.plain =
	{
		/* ds_id_io */				"%c io[%d]: %s@%02x\n",
		/* ds_id_pin_1 */			"  pin: %2d, ",
		/* ds_id_pin_2 */			"\n",
		/* ds_id_flags_1 */			" flags: [",
		/* ds_id_flags_2 */			"], ",
		/* ds_id_mode_1 */			"mode: ",
		/* ds_id_mode_2 */			"",
		/* ds_id_disabled */		"",
		/* ds_id_input */			"state: %s",
		/* ds_id_counter */			"counter: %d",
		/* ds_id_rotary_encoder_1 */"pin ",
		/* ds_id_rotary_encoder_2 */", counter: %d, partner pin: %u, trigger io: %d, pin: %d, remote: %d",
		/* ds_id_trigger_1 */		"trigger, counter: %d\n",
		/* ds_id_trigger_2 */		"             action #%d: io: %d, pin: %d, action: ",
		/* ds_id_trigger_3 */		"",
		/* ds_id_output */			"output, state: %s",
		/* ds_id_timer */			"config direction: %s, speed: %d ms, current direction: %s, delay: %d ms, state: %s",
		/* ds_id_pwm1_output */		"primary pwm output, min/static: %u, max: %u, current speed: %d, direction: %s, value: %u, saved value: %u",
		/* ds_id_pwm2_output */		"secondary pwm output, min/static: %u, max: %u, current speed: %d, direction: %s, value: %u, saved value: %u",
		/* ds_id_i2c_sda */			"sda",
		/* ds_id_i2c_scl */			"scl",
		/* ds_id_uart */			"uart",
		/* ds_id_ledpixel */		"ledpixel",
		/* ds_id_cfa634 */			"cfa634",
		/* ds_id_lcd */				"lcd",
		/* ds_id_spi */				"spi",
		/* ds_id_unknown */			"unknown",
		/* ds_id_max_value */		", max value: %u",
		/* ds_id_info_1 */			", info: ",
		/* ds_id_info_2 */			"",
		/* ds_id_pins_header */		"",
		/* ds_id_hw_1 */			" [hw: ",
		/* ds_id_hw_2 */			"]",
		/* ds_id_table_start */		"",
		/* ds_id_table_end */		"",
		/* ds_id_error */			"error",
	},

	.html =
	{
		/* ds_id_io */				"<tr><th colspan=\"6\" align=\"center\">%c IO %d: %s@%02x</th></tr>\n",
		/* ds_id_pin_1 */			"<tr><td align=\"center\">%d</td>",
		/* ds_id_pin_2 */			"</tr>\n",
		/* ds_id_flags_1 */			"<td>",
		/* ds_id_flags_2 */			"</td>",
		/* ds_id_mode_1 */			"<td>",
		/* ds_id_mode_2 */			"</td>",
		/* ds_id_disabled */		"<td></td>",
		/* ds_id_input */			"<td>state: %s</td>",
		/* ds_id_counter */			"<td><td>counter: %d</td>",
		/* ds_id_rotary_encoder_1 */"<td>pin ",
		/* ds_id_rotary_encoder_2 */", counter: %d, partner pin: %u, trigger io: %d, pin: %d, remote: %d</td>",
		/* ds_id_trigger_1 */		"<td>counter: %d, ",
		/* ds_id_trigger_2 */		"action: #%d, io: %d, pin: %d, trigger action: ",
		/* ds_id_trigger_3 */		"</td>",
		/* ds_id_output */			"<td>output</td><td>state: %s</td>",
		/* ds_id_timer */			"<td>config direction: %s, speed: %d ms, current direction %s, delay: %d ms, state: %s</td>",
		/* ds_id_pwm1_output */		"<td>min/static: %u, max: %u, speed: %d, current direction: %s, value: %u, saved value: %u",
		/* ds_id_pwm2_output */		"<td>min/static: %u, max: %u, speed: %d, current direction: %s, value: %u, saved value: %u",
		/* ds_id_i2c_sda */			"<td>sda</td>",
		/* ds_id_i2c_scl */			"<td>scl</td>",
		/* ds_id_uart */			"<td>uart</td>",
		/* ds_id_ledpixel */		"<td>ledpixel</td>",
		/* ds_id_cfa634 */			"<td>cfa634</td>",
		/* ds_id_lcd */				"<td>lcd</td>",
		/* ds_id_spi */				"<td>spi</td>",
		/* ds_id_unknown */			"<td>unknown</td>",
		/* ds_id_max_value */		"<td>%u</td>",
		/* ds_id_info_1 */			"<td>",
		/* ds_id_info 2 */			"</td>",
		/* ds_id_pins_header */		"<tr><th>pin</th><th>mode</th><th>hw mode</th><th>flags</th><th>pin info</th><th>extra info</th></tr>\n",
		/* ds_id_hw_1 */			"<td>",
		/* ds_id_hw_2 */			"</td>",
		/* ds_id_table_start */		"<table border=\"1\" cellpadding=\"1\" cellspacing=\"1\">\n",
		/* ds_id_table_end */		"</table>\n",
		/* ds_id_error */			"<td>error</td>",
	}
};

void io_config_dump(string_t *dst, int io_id, int pin_id, bool html)
{
	const io_info_entry_t *info;
	io_data_entry_t *data;
	io_data_pin_entry_t *pin_data;
	const io_config_pin_entry_t *pin_config;
	const string_array_t *roflash_strings;
	unsigned int io, pin, value;
	io_error_t error;

	if(html)
		roflash_strings = &roflash_dump_strings.html;
	else
		roflash_strings = &roflash_dump_strings.plain;

	string_append_cstr_flash(dst, (*roflash_strings)[ds_id_table_start]);

	for(io = 0; io < io_id_size; io++)
	{
		if((io_id >= 0) && (io_id != (int)io))
			continue;

		info = &io_info[io];
		data = &io_data[io];

		string_format_flash_ptr(dst, (*roflash_strings)[ds_id_io], data->detected ? '*' : ' ', io, info->name, info->address);

		if(!data->detected || (io_id < 0))
			continue;

		string_append_cstr_flash(dst, (*roflash_strings)[ds_id_pins_header]);

		for(pin = 0; pin < info->pins; pin++)
		{
			if((pin_id >= 0) && (pin_id != (int)pin))
				continue;

			if((io == 0) && !io_gpio_pin_usable(pin))
				continue;

			pin_config = &io_config[io][pin];
			pin_data = &data->pin[pin];

			string_format_flash_ptr(dst, (*roflash_strings)[ds_id_pin_1], pin);

			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_mode_1]);
			io_string_from_mode(dst, pin_config->mode, -1);
			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_mode_2]);

			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_hw_1]);
			io_string_from_ll_mode(dst, pin_config->llmode, -1);
			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_hw_2]);

			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_flags_1]);
			io_flags_to_string(dst, pin_config);
			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_flags_2]);

			switch(pin_config->mode)
			{
				case(io_pin_input_digital):
				case(io_pin_counter):
				case(io_pin_output_digital):
				case(io_pin_timer):
				case(io_pin_input_analog):
				case(io_pin_output_pwm1):
				case(io_pin_output_pwm2):
				case(io_pin_i2c):
				{
					if((error = io_read_pin_x(dst, info, pin_data, pin_config, pin, &value)) != io_ok)
						string_append(dst, "\n");

					break;
				}

				default:
				{
					error = io_ok;
					break;
				}
			}

			switch(pin_config->mode)
			{
				case(io_pin_disabled):
				{
					string_append_cstr_flash(dst, (*roflash_strings)[ds_id_disabled]);

					break;
				}

				case(io_pin_input_digital):
				{
					if(error == io_ok)
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_input], onoff(value));
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_counter):
				{
					if(error == io_ok)
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_counter], value);
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_rotary_encoder):
				{
					if(error == io_ok)
					{
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_rotary_encoder_1]);
						io_string_from_renc_pin(dst, pin_config->shared.renc.pin_type);
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_rotary_encoder_2], value, pin_config->shared.renc.partner,
								pin_config->shared.renc.trigger_pin.io, pin_config->shared.renc.trigger_pin.pin, pin_config->shared.renc.trigger_pin.remote);
					}
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_trigger):
				{
					if(error == io_ok)
					{
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_trigger_1], value);
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_trigger_2], 0,
								pin_config->shared.trigger[0].io.io,
								pin_config->shared.trigger[0].io.pin);
						trigger_action_to_string(dst, pin_config->shared.trigger[0].action);

						if(pin_config->shared.trigger[1].action != io_trigger_none)
						{
							string_append(dst, "\n");
							string_format_flash_ptr(dst, (*roflash_strings)[ds_id_trigger_2], 1,
									pin_config->shared.trigger[1].io.io,
									pin_config->shared.trigger[1].io.pin);
							trigger_action_to_string(dst, pin_config->shared.trigger[1].action);
						}
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_trigger_3]);
					}
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_output_digital):
				{
					if(error == io_ok)
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_output], onoff(value));
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_timer):
				{
					if(error == io_ok)
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_timer],
								pin_config->direction == io_dir_up ? "up" : (pin_config->direction == io_dir_down ? "down" : "none"),
								pin_config->speed,
								pin_data->direction == io_dir_up ? "up" : (pin_data->direction == io_dir_down ? "down" : "none"),
								pin_data->speed,
								onoff(value));
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_output_pwm1):
				{
					if(error == io_ok)
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_pwm1_output],
								pin_config->shared.output_pwm.lower_bound,
								pin_config->shared.output_pwm.upper_bound,
								pin_config->speed,
								pin_data->direction == io_dir_up ? "up" : (pin_data->direction == io_dir_down ? "down" : "none"),
								value, pin_data->saved_value);
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_output_pwm2):
				{
					if(error == io_ok)
						string_format_flash_ptr(dst, (*roflash_strings)[ds_id_pwm2_output],
								pin_config->shared.output_pwm.lower_bound,
								pin_config->shared.output_pwm.upper_bound,
								pin_config->speed,
								pin_data->direction == io_dir_up ? "up" : (pin_data->direction == io_dir_down ? "down" : "none"),
								value, pin_data->saved_value);
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_error]);

					break;
				}

				case(io_pin_i2c):
				{
					if(pin_config->shared.i2c.pin_mode == io_i2c_sda)
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_i2c_sda]);
					else
						string_append_cstr_flash(dst, (*roflash_strings)[ds_id_i2c_scl]);

					break;
				}

				case(io_pin_uart):
				{
					string_append_cstr_flash(dst, (*roflash_strings)[ds_id_uart]);

					break;
				}

				case(io_pin_lcd):
				{
					string_append_cstr_flash(dst, (*roflash_strings)[ds_id_lcd]);
					string_append(dst, "/");
					io_string_from_lcd_mode(dst, pin_config->shared.lcd.pin_use);

					break;
				}

				case(io_pin_ledpixel):
				{
					string_append_cstr_flash(dst, (*roflash_strings)[ds_id_ledpixel]);

					break;
				}

				case(io_pin_cfa634):
				{
					string_append_cstr_flash(dst, (*roflash_strings)[ds_id_cfa634]);

					break;
				}

				case(io_pin_spi):
				{
					string_append_cstr_flash(dst, (*roflash_strings)[ds_id_spi]);

					break;
				}

				default:
				{
					string_append_cstr_flash(dst, (*roflash_strings)[ds_id_unknown]);

					break;
				}
			}

			string_format_flash_ptr(dst, (*roflash_strings)[ds_id_max_value], io_pin_max_value(io, pin));
			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_info_1]);
			if(info->get_pin_info_fn)
				info->get_pin_info_fn(dst, info, pin_data, pin_config, pin);
			string_append_cstr_flash(dst, (*roflash_strings)[ds_id_info_2]);

			string_format_flash_ptr(dst, (*roflash_strings)[ds_id_pin_2], pin);
		}
	}

	string_append_cstr_flash(dst, (*roflash_strings)[ds_id_table_end]);
}
