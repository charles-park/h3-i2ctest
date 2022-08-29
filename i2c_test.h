//-----------------------------------------------------------------------------
/**
 * @file i2c_test.h
 * @author charles-park (charles-park@hardkernel.com)
 * @brief server control header file.
 * @version 0.1
 * @date 2022-08-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <unistd.h>
#include <sys/time.h>
//-----------------------------------------------------------------------------
typedef struct app_data__t {
	/* build info */
	char		bdate[32], btime[32];
	/* JIG model name */
	char		model[32];
	/* I2C dev node */
	char		i2c_node_name[2][32];
	__u8		i2c_test_addr[2];
	/* FB dev node */
	char		fb_dev[32];

	fb_info_t	*pfb;
	ui_grp_t	*pui;

}	app_data_t;

//------------------------------------------------------------------------------
extern  int app_main (app_data_t *app_data);

//------------------------------------------------------------------------------
#endif  // #define __APP_DATA_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
