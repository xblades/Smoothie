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
#include "WatchScreen.h"
#include "libs/nuts_bolts.h"
#include "libs/utils.h"
#include "I2CLCD.h"
#include <string>
using namespace std;

WatchScreen::WatchScreen(){}

void WatchScreen::on_enter(){
    this->display_screen();
}

void WatchScreen::on_refresh(){
    // Exit if the button is clicked 
    if( this->panel->click() ){
        this->panel->enter_screen(this->parent);
        return;
    }

    // Only every 20 refreshes
    static int update_counts = 0;
    update_counts++;
    if( update_counts % 20 == 0 ){
    }
}

void WatchScreen::display_screen(){
        this->panel->lcd->clear();
        this->panel->lcd->setCursor(0,0);
        this->panel->lcd->printf("H137/000c B048/000c");
        this->panel->lcd->setCursor(0,1);
        this->panel->lcd->printf("          Z:+000.00");
        this->panel->lcd->setCursor(0,2);
        this->panel->lcd->printf("100%%      00%% sd");
        this->panel->lcd->setCursor(0,3);
        this->panel->lcd->printf("Smoothie ready");
}

