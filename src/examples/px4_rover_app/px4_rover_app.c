/**
 * @file px4_rover_app.c
 * Control application for PX4 autopilot - Rover
 *
 * @author Sudharsan Vaidhun <svqm5@mst.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <px4_config.h>
#include <nuttx/sched.h>

#include <systemlib/systemlib.h>
#include <systemlib/err.h>

#include "drivers/drv_pwm_output.h"

static bool thread_should_exit = false;		/**< daemon exit flag */
static bool thread_running = false;		/**< daemon status flag */
static int daemon_task;				/**< Handle of daemon task / thread */
static int fd;
static int ret;
static unsigned pwm_value = 0;
static unsigned servo_count = 0;

/**
 * daemon management function.
 */
__EXPORT int px4_rover_app_main(int argc, char *argv[]);

/**
 * Mainloop of daemon.
 */
int px4_rover_app_thread_main(int argc, char *argv[]);

/**
 * Print the correct usage.
 */
static void usage(const char *reason);

static void
usage(const char *reason)
{
	if (reason) {
		warnx("%s\n", reason);
	}

	warnx("usage: rover_control {start|stop|status} [-p <additional params>]\n\n");
}

/**
 * The daemon app only briefly exists to start
 * the background job. The stack size assigned in the
 * Makefile does only apply to this management task.
 *
 * The actual stack size should be set in the call
 * to task_create().
 */
int px4_rover_app_main(int argc, char *argv[])
{
	if (argc < 2) {
		usage("missing command");
		return 1;
	}

	if (!strcmp(argv[1], "start")) {

		if (thread_running) {
			warnx("daemon already running\n");
			/* this is not an error */
			return 0;
		}

		thread_should_exit = false;
		daemon_task = px4_task_spawn_cmd("px4_rover_app",
						 SCHED_DEFAULT,
						 SCHED_PRIORITY_MAX - 20,
						 2000,
						 px4_rover_app_thread_main,
						 (argv) ? (char *const *)&argv[2] : (char *const *)NULL);
		return 0;
	}

	if (!strcmp(argv[1], "stop")) {
		warnx("[rover_control] exiting.\n");

		pwm_value = 1500;
		servo_count = 8;
		const char *dev = PWM_OUTPUT0_DEVICE_PATH;
		fd = open(dev, 0);

		for (unsigned i = 0; i < servo_count; i++) {
			ret = ioctl(fd, PWM_SERVO_SET(i), pwm_value);

			if (ret != OK) {
				err(1, "PWM_SERVO_SET(%d)", i);
			}
		}

		/* disarm, but do not revoke the SET_ARM_OK flag */
		ret = ioctl(fd, PWM_SERVO_DISARM, 0);

		if (ret != OK) {
			err(1, "PWM_SERVO_DISARM");
		}

		thread_running = false;
		thread_should_exit = true;
		return 0;
	}

	if (!strcmp(argv[1], "status")) {
		if (thread_running) {
			warnx("\trunning\n");

		} else {
			warnx("\tnot started\n");
		}

		return 0;
	}

	usage("unrecognized command");
	return 1;
}

int px4_rover_app_thread_main(int argc, char *argv[])
{
	warnx("[rover_control] starting\n");

	thread_running = true;

	/* open for ioctl only */
	const char *dev = PWM_OUTPUT0_DEVICE_PATH;
	fd = open(dev, 0);
	if (fd < 0) {
		err(1, "can't open %s", dev);
	}

	/* tell safety that its ok to disable it with the switch */
	ret = ioctl(fd, PWM_SERVO_SET_ARM_OK, 0);

	if (ret != OK) {
		err(1, "PWM_SERVO_SET_ARM_OK");
	}

	/* tell IO that the system is armed (it will output values if safety is off) */
	ret = ioctl(fd, PWM_SERVO_ARM, 0);

	if (ret != OK) {
		err(1, "PWM_SERVO_ARM");
	}

	// while (!thread_should_exit) {
		warnx("Hello rover control!\n");
		
		pwm_value = 1700;
		servo_count = 8;

		for (unsigned i = 0; i < servo_count; i++) {
			ret = ioctl(fd, PWM_SERVO_SET(i), pwm_value);

			if (ret != OK) {
				err(1, "PWM_SERVO_SET(%d)", i);
			}
		}

		// sleep(10);
	// }

	return 0;
}
