/*
 * This file is part of the ZombieVerter project.
 *
 * Copyright (C) 2024 Tom de Bree <tom@voltinflux.com>
 * Copyright (C) 2010 Johannes Huebner <contact@johanneshuebner.com>
 * Copyright (C) 2010 Edward Cheeseman <cheesemanedward@gmail.com>
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2019-2022 Damien Maguire <info@evbmw.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 #include <stdint.h>
 #include <libopencm3/stm32/usart.h>
 #include <libopencm3/stm32/timer.h>
 #include <libopencm3/stm32/rtc.h>
 #include <libopencm3/stm32/can.h>
 #include <libopencm3/stm32/iwdg.h>
 #include <libopencm3/stm32/spi.h>
 #include <libopencm3/stm32/exti.h>
 #include "stm32_can.h"
 #include "terminal.h"
 #include "params.h"
 #include "hwdefs.h"
 #include "digio.h"
 #include "hwinit.h"
 #include "anain.h"
 #include "param_save.h"
 #include "my_math.h"
 #include "errormessage.h"
 #include "printf.h"
 #include "stm32scheduler.h"
 #include "cansdo.h"
 #include "canmap.h"
 #include "terminalcommands.h"
  
 #define PRECHARGE_TIMEOUT 5  //5s
 
 #define PRINT_JSON 0
 
 
 extern "C" void __cxa_pure_virtual()
 {
     while (1);
 }
 
 static Stm32Scheduler* scheduler;
 static CanHardware* canInterface[3];
 static CanMap* canMap;


 
 
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 static void Ms200Task(void)
 {
      
 }
 
 static void Ms100Task(void)
 {
     DigIo::led_out.Toggle();
     iwdg_reset();
     float cpuLoad = scheduler->GetCpuLoad() / 10.0f;
     Param::SetFloat(Param::cpuload, cpuLoad);
     Param::SetInt(Param::lasterr, ErrorMessage::GetLastError());
     
 }
 

 static void Ms10Task(void)
 {
    
 }
 
 static void Ms1Task(void)
 {

 }
 

  void Param::Change(Param::PARAM_NUM paramNum)
 {

 }
 
 
 static bool CanCallback(uint32_t id, uint32_t data[2], uint8_t dlc) //This is where we go when a defined CAN message is received.
 {

 }
 
 
 static void ConfigureVariantIO()
 {
     ANA_IN_CONFIGURE(ANA_IN_LIST);
     DIG_IO_CONFIGURE(DIG_IO_LIST);
     AnaIn::Start();
 }
 
 
 extern "C" void tim4_isr(void)
 {
     scheduler->Run();
 }
 
 
 extern "C" void exti15_10_isr(void)    //CAN3 MCP25625 interruppt
 {

 
 }
 
 extern "C" void rtc_isr(void)
 {
     /* The interrupt flag isn't cleared by hardware, we have to do it. */
     rtc_clear_flag(RTC_SEC);
 
 }
 
 extern "C" int main(void)
 {
     extern const TERM_CMD termCmds[];
 
     clock_setup();
     rtc_setup();
     ConfigureVariantIO();
     gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_CAN2_REMAP | AFIO_MAPR_TIM1_REMAP_FULL_REMAP);//32f107
     usart2_setup();//TOYOTA HYBRID INVERTER INTERFACE
     nvic_setup();
     parm_load();
     spi2_setup();
     spi3_setup();
     tim3_setup(); //For general purpose PWM output
    
     Param::Change(Param::PARAM_LAST);
     
 
     Terminal t(USART3, termCmds);

       
     Stm32Can c(CAN1, (CanHardware::baudrates)Param::GetInt(Param::canspeed));
     CanMap cm(&c);
     CanSdo sdo(&c, &cm);
     sdo.SetNodeId(33); // Set node ID for SDO access e.g. by wifi module
     // store a pointer for easier access
     canInterface[0] = &c;
     canMap = &cm;

      // This is all we need to do to set up a terminal on USART3
    
     TerminalCommands::SetCanMap(canMap);
 

 
     Stm32Scheduler s(TIM4); //We never exit main so it's ok to put it on stack
     scheduler = &s;
 
     s.AddTask(Ms1Task, 1);
     s.AddTask(Ms10Task, 10);
     s.AddTask(Ms100Task, 100);
     s.AddTask(Ms200Task, 200);
 
     
     Param::SetInt(Param::version, 4); //backward compatibility
     Param::SetInt(Param::opmode, MOD_OFF);//always off at startup
 
     while(1)
     {
         char c = 0;
         t.Run();
         if (sdo.GetPrintRequest() == PRINT_JSON)
         {
            TerminalCommands::PrintParamsJson(&sdo, &c);
         }
     }
 
     return 0;
 }