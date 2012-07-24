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
#include "PanelScreen.h"

#define panel_checksum             14866 
#define up_button_pin_checksum     62859       
#define down_button_pin_checksum   61791
#define click_button_pin_checksum  9133 
#define enable_checksum            29545 
#define encoder_a_pin_checksum     63819 
#define encoder_b_pin_checksum     65100 

#define MENU_MODE                  0
#define CONTROL_MODE               1


class Panel : public Module {
    public:
        Panel();
       
        void on_module_loaded();
        uint32_t button_tick(uint32_t dummy);
        void on_idle(void* argument);
        void enter_screen(PanelScreen* screen);
        void reset_counter();

        // Encoder and buttons
        uint32_t on_up(uint32_t dummy);
        uint32_t on_down(uint32_t dummy);
        uint32_t on_click_release(uint32_t dummy);
        uint32_t refresh_tick(uint32_t dummy);
        uint32_t encoder_check(uint32_t dummy);
        bool counter_change();
        bool click();

        // Menu
        void enter_menu_mode();
        void setup_menu(uint16_t rows, uint16_t lines);
        void menu_update();
        uint16_t menu_current_line();
        bool menu_change();
        char menu_offset;
        int menu_selected_line;
        int menu_start_line;
        int menu_rows;
        int menu_lines;
        bool menu_changed;

        // Control
        bool enter_control_mode(double passed_normal_increment, double passed_pressed_increment);
        bool set_control_value(double value);
        double normal_increment;
        double pressed_increment;
        int control_normal_counter;
        int control_pressed_counter;

        Button* up_button;
        Button* down_button;
        Pin* encoder_a_pin;
        Pin* encoder_b_pin;
        Button* click_button;
        I2CLCD* lcd;

        int* counter;
        bool counter_changed;
        bool click_changed;
        bool refresh_flag;

        char mode;

        PanelScreen* top_screen;
        PanelScreen* current_screen;
};











#endif
