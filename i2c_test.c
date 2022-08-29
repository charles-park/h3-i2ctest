//------------------------------------------------------------------------------
/**
 * @file i2c_test.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief server main control.
 * @version 0.1
 * @date 2022-08-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>

#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define	SYSTEM_LOOP_DELAY_uS	100
//------------------------------------------------------------------------------
// for my lib
//------------------------------------------------------------------------------
/* 많이 사용되는 define 정의 모음 */
#include "typedefs.h"

/* framebuffer를 control하는 함수 */
#include "lib_fb.h"

/* file parser control 함수 */
#include "lib_ui.h"

#include "i2c_test.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
__s32 i2c_smbus_access(int file, char read_write, __u8 command,
		       int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;
	__s32 err;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;

	err = ioctl(file, I2C_SMBUS, &args);
	if (err == -1)
		err = -errno;
	return err;
}

//------------------------------------------------------------------------------
void app_info_display (app_data_t *app_data)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

    ui_set_str (app_data->pfb, app_data->pui, 0, -1, -1,
                4, -1, "ODROID-H3 I2C Test App");
    ui_set_str (app_data->pfb, app_data->pui, 1, -1, -1,
                3, -1, "%d/%d/%d, %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	{
		int fd, i;

		for (i = 0; i < 2; i++) {
			ui_set_str (app_data->pfb, app_data->pui, i + 2, -1, -1,
						3, -1, "Check %s Node", app_data->i2c_node_name[i]);
			if (access (app_data->i2c_node_name[i], F_OK) != 0) {
				ui_set_ritem(app_data->pfb, app_data->pui, i + 2, COLOR_RED, -1);
			}
			else {
				ui_set_ritem(app_data->pfb, app_data->pui, i + 2, COLOR_GREEN, -1);
				ui_set_str (app_data->pfb, app_data->pui, i + 4, -1, -1,
							3, -1, "Check %s Device (Addr = 0x%02x)",
							app_data->i2c_node_name[i],
							app_data->i2c_test_addr[i]);

				if ((fd = open(app_data->i2c_node_name[i], O_RDWR)) < 0) {
					ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_RED, -1);
				} else {
					// set the I2C slave address for all subsequent I2C device transfers
					if (ioctl(fd, I2C_SLAVE, app_data->i2c_test_addr[i]) < 0) {
						err("Error failed to set I2C address [0x%02x].\n",
							app_data->i2c_test_addr[i]);
						ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_RED, -1);
					} else {
						union i2c_smbus_data data;
						int err;

						err = i2c_smbus_access(fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
						if (err)
							ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_RED, -1);
						else
							ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_GREEN, -1);
					}
					close(fd);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int app_main (app_data_t *app_data)
{
	while (1) {
		app_info_display(app_data);
		sleep(1);
	}
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
