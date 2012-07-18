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
#include "libs/nuts_bolts.h"
#include "libs/utils.h"
#include "I2CLCD.h"
#include <string>
using namespace std;

MainMenuScreen::MainMenuScreen(){}

void MainMenuScreen::on_enter(){
    this->panel->enter_menu_mode();
    this->panel->setup_menu(7, 4);  // 6 menu items, 4 lines
    this->refresh_screen();
}

void MainMenuScreen::on_refresh(){
    if( this->panel->menu_change() ){
        this->refresh_screen();
    }
}

void MainMenuScreen::refresh_screen(){
    this->panel->lcd->clear(); 
    for(uint16_t i = this->panel->menu_start_line; i < this->panel->menu_start_line + this->panel->menu_lines; i++ ){
        this->panel->lcd->setCursor(2, i - this->panel->menu_start_line );
        this->panel->lcd->printf("i:%d", i);
    }

    this->panel->lcd->setCursor(0, this->panel->menu_selected_line - this->panel->menu_start_line );
    this->panel->lcd->printf(">");

}

