/**
 * Clayton Power LPS - CAN HMI Dashboard + Settings
 * ESP32-S3-Touch-LCD-5 (1024x600)
 *
 * Ported from RP2350-Touch-LCD-4 (480x480) version.
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the CAN HMI UI (create all pages/widgets).
 *        Must be called with the LVGL mutex held.
 */
void can_hmi_init(void);

/**
 * @brief CAN HMI background task — polls TWAI, decodes messages,
 *        updates UI at ~20 fps.  Runs as a FreeRTOS task.
 *
 * @param arg  Unused (pass NULL).
 */
void can_hmi_task(void *arg);

#ifdef __cplusplus
}
#endif
