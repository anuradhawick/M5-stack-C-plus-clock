#include <Arduino.h>
#include <M5StickCPlus.h>
#include "views/View.h"
#include "views/TimeView.h"
#include "views/BTView.h"
#include "views/AnalogClock.h"
#include "views/BrightnessView.h"
#include "tasks.h"

static const char *TAG = "MAIN";

View *active_view;
uint8_t state = 1;
unsigned long rst_clicked_time = millis();
unsigned long home_clicked_time = millis();
unsigned long active_time = millis();
float pitch, roll, yaw;
float next_pitch, next_roll, next_yaw;

TASKS::Dimmer dimmer;

bool rst_btn_clicked()
{
    if (rst_clicked_time + 500 > millis())
    {
        return false;
    }

    ESP_LOGD(TAG, "Rst Btn Clicked");

    rst_clicked_time = millis();

    return true;
}

bool home_btn_clicked()
{
    if (home_clicked_time + 500 > millis())
    {
        return false;
    }

    ESP_LOGD(TAG, "Home btn Clicked");
    home_clicked_time = millis();

    return true;
}

void setup()
{
    M5.begin();
    M5.Axp.ScreenBreath(9);
    M5.Imu.Init();

    ESP_LOGD(TAG, "%d", M5.Axp.Read8bit(0x28) >> 4);

    active_view = new AnalogClock();

    pinMode(M5_BUTTON_RST, INPUT_PULLUP);
    pinMode(M5_BUTTON_HOME, INPUT_PULLUP);
}

void loop()
{
    // // as long as shaking, follow the attitude
    // if (!dimmer.is_dim())
    // {
    //     M5.IMU.getAhrsData(&pitch, &roll, &yaw);
    // }
    // else
    // {
    //     M5.IMU.getAhrsData(&next_pitch, &next_roll, &next_yaw);

    //     // if dimmed keep comparing
    //     if (abs(next_pitch - pitch) > 5 || abs(next_roll - roll) > 5 || abs(next_yaw - yaw) > 5)
    //     {
    //         active_time = millis();
    //         dimmer.recover();
    //         ESP_LOGD(TAG, "Exit dim by shake");
    //     }
    //     // follow small changes to avoid wakeup
    //     else
    //     {
    //         pitch = next_pitch;
    //         roll = next_roll;
    //         yaw = next_yaw;
    //     }
    // }

    // auto dimming timed
    if (active_time + 10000 < millis())
    {
        dimmer.go_dim();
    }

    if (active_time + 30000 < millis())
    {
        dimmer.go_dark();
    }

    if (active_time + 60000 < millis())
    {
        M5.Axp.PowerOff();
    }

    // render view
    active_view->render();

    if (digitalRead(M5_BUTTON_RST) == LOW)
    {
        active_time = millis();
        if (dimmer.is_dim())
        {
            dimmer.recover();
        }

        if (rst_btn_clicked() && !dimmer.dim_exitting())
        {
            active_view->receive_event(EVENTS::RST_CLICKED);
        }
    }

    if (M5.Axp.GetBtnPress() == 0x02)
    {
        ESP_LOGD(TAG, "Power btn Clicked");
        active_time = millis();
        if (dimmer.is_dim())
        {
            dimmer.recover();
        }
        else if (!dimmer.dim_exitting())
        {
            active_view->receive_event(EVENTS::POWER_CLICKED);
        }
    }

    if (digitalRead(M5_BUTTON_HOME) == LOW)
    {
        active_time = millis();
        if (dimmer.is_dim())
        {
            dimmer.recover();
        }
        else if (home_btn_clicked() && !dimmer.dim_exitting())
        {
            if (!active_view->receive_event(EVENTS::HOME_CLICKED))
            {
                state++;

                if (state > 4)
                {
                    state = 1;
                }

                delete active_view;
                active_view = NULL;

                switch (state)
                {
                case 1:
                    active_view = new AnalogClock();
                    break;
                case 2:
                    active_view = new TimeView();
                    break;
                case 3:
                    active_view = new BrightnessView();
                    break;
                case 4:
                    active_view = new BTView();
                    break;

                default:
                    break;
                }
            }
        }
    }
}