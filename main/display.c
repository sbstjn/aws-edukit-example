/*
 * AWS IoT EduKit - Smart Thermostat v1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "core2forAWS.h"

extern SemaphoreHandle_t xGuiSemaphore;

static lv_obj_t *out_txtarea;
static lv_obj_t *wifi_label;

void display_textarea_add(char *baseTxt, char *param, size_t *paramLen)
{
    if (baseTxt != NULL)
    {
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
        if (param != NULL)
        {
            size_t baseTxtLen = strlen(baseTxt);
            if (paramLen != NULL)
            {
                size_t *bufLen = baseTxtLen + paramLen;
                char buf[(int)bufLen];
                sprintf(buf, baseTxt, param);
                lv_textarea_add_text(out_txtarea, buf);
            }
            else
            {
                printf("Textarea parameter length is NULL!");
                lv_textarea_add_text(out_txtarea, baseTxt);
            }
        }
        else
        {
            lv_textarea_add_text(out_txtarea, baseTxt);
        }
        xSemaphoreGive(xGuiSemaphore);
    }
    else
    {
        printf("Textarea baseTxt is NULL!");
    }
}

void display_wifi_label_update(bool state)
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    if (state == false)
    {
        lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    }
    else
    {
        char buffer[25];
        sprintf(buffer, "#0000ff %s #", LV_SYMBOL_WIFI);
        lv_label_set_text(wifi_label, buffer);
    }
    xSemaphoreGive(xGuiSemaphore);
}

void display_init(void)
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    wifi_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(wifi_label, NULL, LV_ALIGN_IN_TOP_RIGHT, 0, 6);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    lv_label_set_recolor(wifi_label, true);

    out_txtarea = lv_textarea_create(lv_scr_act(), NULL);
    lv_obj_set_size(out_txtarea, 300, 180);
    lv_obj_align(out_txtarea, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -12);
    lv_textarea_set_text(out_txtarea, "");
    xSemaphoreGive(xGuiSemaphore);
}