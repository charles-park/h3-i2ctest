//------------------------------------------------------------------------------
/**
 * @file main.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief ODROID-H3 I2C Test application.
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
#include <getopt.h>

//------------------------------------------------------------------------------
// for my lib
//------------------------------------------------------------------------------
/* 많이 사용되는 define 정의 모음 */
#include "typedefs.h"

/* framebuffer를 control하는 함수 */
#include "lib_fb.h"

/* file parser control 함수 */
#include "lib_ui.h"

//------------------------------------------------------------------------------
// Application header file
//------------------------------------------------------------------------------
#include "i2c_test.h"

//------------------------------------------------------------------------------
// Default global value
//------------------------------------------------------------------------------
const char	*OPT_UI_CFG_FILE	= "default_ui.cfg";
const char	*OPT_APP_CFG_FILE 	= "default_app.cfg";

//------------------------------------------------------------------------------
// function prototype define
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
	while (*p++)   *p = tolower(*p);
}

//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
	while (*p++)   *p = toupper(*p);
}

//------------------------------------------------------------------------------
static void print_usage(const char *prog)
{
	printf("Usage: %s [-fu]\n", prog);
	puts("  -f --app_cfg_file    default name is default_app.cfg.\n"
		 "  -u --ui_cfg_file     default name is default_ui.cfg\n"
	);
	exit(1);
}

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "app_config_file"	, 1, 0, 'f' },
			{ "ui_config_file"	, 1, 0, 'u' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "f:u:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'f':
			OPT_APP_CFG_FILE = optarg;
			break;
		case 'u':
			OPT_UI_CFG_FILE = optarg;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

//------------------------------------------------------------------------------
char *_str_remove_space (char *ptr)
{
	/* 문자열이 없거나 앞부분의 공백이 있는 경우 제거 */
	int slen = strlen(ptr);

	while ((*ptr == 0x20) && slen--)
		ptr++;

	return ptr;
}

//------------------------------------------------------------------------------
void _strtok_strcpy (char *dst)
{
	char *ptr;

	if ((ptr = strtok (NULL, ",")) != NULL) {
		ptr = _str_remove_space(ptr);
		strncpy(dst, ptr, strlen(ptr));
	}
}

//------------------------------------------------------------------------------
void _parse_model_name (app_data_t *app_data)
{
	_strtok_strcpy(app_data->model);
}

//------------------------------------------------------------------------------
void _parse_fb_config (app_data_t *app_data)
{
	_strtok_strcpy(app_data->fb_dev);
}

//------------------------------------------------------------------------------
#define I2C_BUS_NAME		"/sys/bus/i2c/devices/"

void _parse_i2c_config (app_data_t *app_data)
{
	char	i2c_adpter_name[64];
	char	i2c_addr_str[10], found_i2c = 0, cnt;

	memset (i2c_adpter_name, 0x00, sizeof(i2c_adpter_name));
	_strtok_strcpy(i2c_adpter_name);

	for (cnt = 0; cnt < 2; cnt++) {
		memset (i2c_addr_str, 0, sizeof(i2c_addr_str));
		_strtok_strcpy(i2c_addr_str);
		tolowerstr(i2c_addr_str);

		if (i2c_addr_str[0] == '0' && i2c_addr_str[1] == 'x')
			app_data->i2c_test_addr[cnt] = (__u8)(strtoul(i2c_addr_str, NULL, 16));
		else
			app_data->i2c_test_addr[cnt] = (__u8)(strtoul(i2c_addr_str, NULL, 10));
	}

	for (cnt = 0; cnt < 20; cnt++) {
		char bname[64], line_str[64];
		FILE *fp;

		memset (bname,    0x00, sizeof(bname));
		memset (line_str, 0x00, sizeof(line_str));

		sprintf(bname, "%si2c-%d/name", I2C_BUS_NAME, cnt);

		if (access (bname, F_OK) < 0)
			continue;
		if ((fp = fopen(bname, "r")) < 0)
			continue;

		if (fgets(line_str, sizeof(line_str)-1, fp) != NULL) {
			if (!strncmp(i2c_adpter_name, line_str, strlen(i2c_adpter_name)-1)) {
				if (found_i2c < 2) {
					memset (app_data->i2c_node_name[found_i2c], 0x00, sizeof(app_data->i2c_node_name[0]));
					sprintf(app_data->i2c_node_name[found_i2c], "/dev/i2c-%d", cnt);
					info ("I2C Node = %s, I2C TEST ADDR = 0x%02X\n",
						app_data->i2c_node_name[found_i2c], app_data->i2c_test_addr[found_i2c]);
					found_i2c++;
				}
			}
		}
		fclose(fp);
	}
}

//------------------------------------------------------------------------------
bool parse_cfg_file (char *cfg_filename, app_data_t *app_data)
{
	FILE *pfd;
	char buf[256], *ptr, is_cfg_file = 0;

	if ((pfd = fopen(cfg_filename, "r")) == NULL) {
		err ("%s file open fail!\n", cfg_filename);
		return false;
	}

	/* config file에서 1 라인 읽어올 buffer 초기화 */
	memset (buf, 0, sizeof(buf));
	while(fgets(buf, sizeof(buf), pfd) != NULL) {
		/* config file signature 확인 */
		if (!is_cfg_file) {
			is_cfg_file = strncmp ("ODROID-APP-CONFIG", buf, strlen(buf)-1) == 0 ? 1 : 0;
			memset (buf, 0x00, sizeof(buf));
			continue;
		}

		ptr = strtok (buf, ",");
		if (!strncmp(ptr, "MODEL", strlen("MODEL")))	_parse_model_name (app_data);
		if (!strncmp(ptr,    "FB", strlen("FB")))		_parse_fb_config  (app_data);
		if (!strncmp(ptr,   "I2C", strlen("I2C")))		_parse_i2c_config (app_data);
		memset (buf, 0x00, sizeof(buf));
	}

	if (pfd)
		fclose (pfd);

	if (!is_cfg_file) {
		err("This file is not APP Config File! (filename = %s)\n", cfg_filename);
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	app_data_t	*app_data;

    parse_opts(argc, argv);

	if ((app_data = (app_data_t *)malloc(sizeof(app_data_t))) == NULL) {
		err ("create application fail!\n");
		goto err_out;
	}
	memset  (app_data, 0, sizeof(app_data_t));

	info("APP Config file : %s\n", OPT_APP_CFG_FILE);
	if (!parse_cfg_file ((char *)OPT_APP_CFG_FILE, app_data)) {
		err ("APP init fail!\n");
		goto err_out;
	}
	strncpy (app_data->bdate, __DATE__, strlen(__DATE__));
	strncpy (app_data->btime, __TIME__, strlen(__TIME__));
	info ("Application Build : %s / %s\n", app_data->bdate, app_data->btime);

	info("Framebuffer Device : %s\n", app_data->fb_dev);
	if ((app_data->pfb = fb_init (app_data->fb_dev)) == NULL) {
		err ("create framebuffer fail!\n");
		goto err_out;
	}

	printf("========== FB SCREENINFO ==========\n");
	printf("xres   : %d\n", app_data->pfb->w);
	printf("yres   : %d\n", app_data->pfb->h);
	printf("bpp    : %d\n", app_data->pfb->bpp);
	printf("stride : %d\n", app_data->pfb->stride);
	printf("bgr    : %d\n", app_data->pfb->is_bgr);
	printf("fb_base     : %p\n", app_data->pfb->base);
	printf("fb_data     : %p\n", app_data->pfb->data);
	printf("==================================\n");

	info("UI Config file : %s\n", OPT_UI_CFG_FILE);
	if ((app_data->pui = ui_init (app_data->pfb, OPT_UI_CFG_FILE)) == NULL) {
		err ("create ui fail!\n");
		goto err_out;
	}

	// main control function (server.c)
	app_main (app_data);

err_out:
	ui_close (app_data->pui);
	fb_clear (app_data->pfb);
	fb_close (app_data->pfb);

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
