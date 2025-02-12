/*
 * This file is part of the ZombieVerter project.
 *
 * Copyright (C) 2012-2020 Johannes Huebner <dev@johanneshuebner.com>
 *               2021-2022 Damien Maguire <info@evbmw.com>
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

 #ifndef TESLA_VALVE_H
 #define TESLA_VALVE_H
 
 #include "utils.h"
 #include "params.h"
 #include "anain.h"
 #include "hwdefs.h"
 #include "digio.h"
 
 class TeslaValve
 {
 public:
     /** Default constructor */
     TeslaValve() {}
 
     /** Write parameter to the digital pin */
     void WriteValveState()
     {
         int valveState = Param::GetInt(Param::valve_out);
         if (valveState == 1)
         {
             DigIo::tesla_valve_out.Set();
         }
         else
         {
             DigIo::tesla_valve_out.Clear();
         }
     }
 
     /** Read the analog pin and update the parameter */
     void ReadValveState()
     {
         float valveInput = AnaIn::tesla_valve_in.Get();
         Param::SetFloat(Param::valve_in, valveInput);
     }
 
     /** Task to be executed every 100ms */
     void Task100Ms()
     {
         WriteValveState();
         ReadValveState();
     }
 };
 
 #endif // TESLA_VALVE_H
 
 
 