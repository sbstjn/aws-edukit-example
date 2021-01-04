#include "esp_event_loop.h"

#pragma once

esp_err_t event_handler(void *ctx, system_event_t *event);
void initialise_wifi(void);