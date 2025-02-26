#ifndef TESLA_COOLANT_PUMP_H
#define TESLA_COOLANT_PUMP_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "anain.h"

#define PWM_PERIOD_MS 500   // 2Hz PWM (500ms cycle)
#define MAX_RPM 4700        // Maximum RPM allowed
#define MIN_RPM 0           // Minimum RPM allowed

class TeslaCoolantPump
{
private:
    uint32_t last_pwm_update = 0;  // Timestamp for PWM update
    uint16_t pwm_on_time = 0;      // ON time in milliseconds
    bool pwm_state = false;        // Current PWM state (ON/OFF)

public:
    /** Default constructor */
    TeslaCoolantPump() {}

    /** Task to be executed every 100ms */
    void Task100Ms()
    {
        /*
         *  Start --> [Check coolant_pump_mode] --> (Manual?)
         *                    | Yes |                 | No |
         *           [coolant_pump_manual_value]  [coolant_pump_automatic_value]
         *                    |                          |
         *                    v                          v
         *            [Validate RPM Range] --> (RPM < 0?)
         *                    | Yes |         | No |
         *              [Set to 0 RPM]       (RPM > 4700?)
         *                      | Yes |        | No |
         *               [Set to 4700 RPM]   [Continue]
         *                      |
         *                      v
         *        [Calculate PWM Duty Cycle: PWM = (RPM + 550) / 65.8]
         *                      |
         *        [Constrain PWM within Valid Ranges]
         *                      |
         *        [Update PWM Output Value]
         *                      |
         *                      v
         *                     End
         */

        // Get operation mode (0 = manual, 1 = automatic)
        int mode = Param::GetInt(Param::coolant_pump_mode);
        int target_rpm = (mode == 0) ? Param::GetInt(Param::coolant_pump_manual_value)
                                     : Param::GetInt(Param::coolant_pump_automatic_value);

        // Constrain RPM within allowed range
        if (target_rpm < MIN_RPM)
            target_rpm = MIN_RPM;
        else if (target_rpm > MAX_RPM)
            target_rpm = MAX_RPM;

        // Convert RPM to PWM % using formula: PWM = (RPM + 550) / 65.8
        uint8_t pwm = (target_rpm + 550) / 65.8;

        // Ensure PWM is within valid ranges
        if (pwm <= 17)
            pwm = 10; // 0 RPM
        else if (pwm >= 80 )
            pwm = 80; // 4700 RPM

        // Compute ON time for new PWM cycle
        pwm_on_time = (pwm * PWM_PERIOD_MS) / 100;

        // Reset timer for PWM control
        last_pwm_update = rtc_get_counter_val();
        pwm_state = true; // Start in ON state
        DigIo::tesla_coolant_pump_out.Clear(); // Turn ON (Active Low)
    }

    /** Hardware timer-based PWM control */
    void Task1Ms()
    {
        /*
         *  Start --> [Check Elapsed Time HAL_GetTick()] --> (Time > ON Time?)
         *                          | Yes |         | No |
         *                  [Switch OFF Pump]      [Continue ON]
         *                          |
         *                          v
         *        (Time > PWM_PERIOD_MS?) --> [Yes] --> [Restart PWM Cycle]
         *                           | No |
         *                        [Continue OFF]
         *                           |
         *                           v
         *                          End
         */

        uint32_t current_time = rtc_get_counter_val();
        uint32_t elapsed_time = current_time - last_pwm_update;

        if (pwm_state && elapsed_time >= pwm_on_time)
        {
            // Turn OFF pump after ON duration is reached
            DigIo::tesla_coolant_pump_out.Set();
            DigIo::led_out.Set(); //turns LED on

            pwm_state = false;
        }
        else if (!pwm_state && elapsed_time >= PWM_PERIOD_MS)
        {
            // Reset cycle after full PWM period
            last_pwm_update = current_time;
            pwm_state = true;
            DigIo::tesla_coolant_pump_out.Clear(); // Turn ON (Active Low)
            DigIo::led_out.Clear(); //turns LED off
        }
    }
};

#endif // TESLA_COOLANT_PUMP_H
