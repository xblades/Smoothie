/*  
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>. 
*/

#include "libs/Kernel.h"
#include "Panel.h"
#include "PanelScreen.h"
#include "MainMenuScreen.h"
#include "ControlScreen.h"
#include "libs/nuts_bolts.h"
#include "libs/utils.h"
#include "I2CLCD.h"
#include <string>
using namespace std;

ControlScreen::ControlScreen(){
    this->control_mode = NULL_CONTROL_MODE;
}

void ControlScreen::on_enter(){
    this->panel->enter_menu_mode();
    this->panel->setup_menu(4, 4);
    this->refresh_menu();
}

void ControlScreen::on_refresh(){
    if( this->panel->menu_change() ){
        this->refresh_menu();
    }
    if( this->panel->click() ){
        this->clicked_menu_entry(this->panel->menu_current_line());
    }
}

void ControlScreen::display_menu_line(uint16_t line){
    switch( line ){
        case 0: this->panel->lcd->printf("Back");  break;  
        case 1: this->display_axis_line('X'); break;  
        case 2: this->display_axis_line('Y'); break;  
        case 3: this->display_axis_line('Z'); break;  
    }
}

void ControlScreen::display_axis_line(char axis){
    this->panel->lcd->printf("%c axis    %5.3f", axis, 12.356);
}


void ControlScreen::clicked_menu_entry(uint16_t line){
    switch( line ){
        case 0: this->panel->enter_screen(this->parent   ); break;
        case 1: this->enter_axis_control('X'); break;
        case 2: this->enter_axis_control('Y'); break;
        case 3: this->enter_axis_control('Z'); break;
    }
}

void ControlScreen::enter_axis_control(char axis){
    this->control_mode = AXIS_CONTROL_MODE;
    this->controlled_axis = axis;
    this->panel->enter_control_mode(1,0.05);
    this->panel->set_control_value(1);
}




