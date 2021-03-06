/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Temp sensor host commands for Chrome EC */

#include "common.h"
#include "host_command.h"
#include "temp_sensor.h"
#include "util.h"

/*
 * Defined in board_temp_sensor.c. Must be in the same order as in enum
 * temp_sensor_id.
 */
extern const struct temp_sensor_t temp_sensors[TEMP_SENSOR_COUNT];

int temp_sensor_command_get_info(struct host_cmd_handler_args *args)
{
	const struct ec_params_temp_sensor_get_info *p = args->params;
	struct ec_response_temp_sensor_get_info *r = args->response;
	int id = p->id;

	if (id >= TEMP_SENSOR_COUNT)
		return EC_RES_ERROR;

	strzcpy(r->sensor_name, temp_sensors[id].name, sizeof(r->sensor_name));
	r->sensor_type = temp_sensors[id].type;

	args->response_size = sizeof(*r);

	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_TEMP_SENSOR_GET_INFO,
		     temp_sensor_command_get_info,
		     EC_VER_MASK(0));
