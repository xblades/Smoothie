/*  
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>. 
*/

#ifndef PANEL_H
#define PANEL_H

#include "libs/Kernel.h"
#include "libs/nuts_bolts.h"
#include "libs/utils.h"
#include "libs/Pin.h"
#include "I2CLCD.h"
#include "Button.h"

#define panel_checksum             14866 
#define up_button_pin_checksum     62859       
#define down_button_pin_checksum   61791
#define click_button_pin_checksum  9133 
#define enable_checksum            29545 
class Panel : public Module {
    public:
        Panel();
       
        void on_module_loaded();
        uint32_t button_tick(uint32_t dummy);
        void on_idle(void* argument);

        uint32_t on_up(uint32_t dummy);
        uint32_t on_down(uint32_t dummy);
        uint32_t on_click_release(uint32_t dummy);

        Button* up_button;
        Button* down_button;
        Button* click_button;
        I2CLCD* lcd;

        int counter;
        bool counter_change;
        bool click_change;
};











#endif
