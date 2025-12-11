#include <lvgl.h>
#include "pin_config.h"
#include "lv_conf.h"
#include "globals.h"
#include <Arduino.h>
#include "Arduino_DriveBus_Library.h"

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
    std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

void Arduino_IIC_Touch_Interrupt(void);

std::unique_ptr<Arduino_IIC> CST816T(new Arduino_CST816x(IIC_Bus, CST816T_DEVICE_ADDRESS,
                                                         TP_RST, TP_INT, Arduino_IIC_Touch_Interrupt));
void Arduino_IIC_Touch_Interrupt(void)
{
    CST816T->IIC_Interrupt_Flag = true;
}

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

// ********* Touth events *********
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    static bool last_pressed = false;

    if (CST816T->IIC_Interrupt_Flag)
    {
        CST816T->IIC_Interrupt_Flag = false;

        int32_t touchX = CST816T->IIC_Read_Device_Value(
            Arduino_CST816x::Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
        int32_t touchY = CST816T->IIC_Read_Device_Value(
            Arduino_CST816x::Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);

        if (touchX >= 0 && touchY >= 0)
        {
            last_x = touchX;
            last_y = touchY;
            last_pressed = true;

            USBSerial.printf("Touch x: %d, y: %d\n", touchX, touchY);
        }
        else
        {
            last_pressed = false;
        }
    }
    else
    {
        // нет нового касания — считаем, что отпустили
        last_pressed = false;
    }

    data->state = last_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->point.x = last_x;
    data->point.y = last_y;
}

void initTouth() {
    /* Initialize the Touth */
    while (CST816T->begin() == false)
    {
        USBSerial.println("CST816T Touth initialization fail");
        delay(2000);
    }
    USBSerial.println("CST816T Touth initialization successfully");

    CST816T->IIC_Write_Device_State(CST816T->Arduino_IIC_Touch::Device::TOUCH_DEVICE_INTERRUPT_MODE,
                                    CST816T->Arduino_IIC_Touch::Device_Mode::TOUCH_DEVICE_INTERRUPT_PERIODIC);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};

    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);
}