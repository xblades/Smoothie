/*  
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>. 
*/

#include "libs/Kernel.h"
#include "Panel.h"
#include "libs/nuts_bolts.h"
#include "libs/utils.h"
#include "I2CLCD.h"
#include <string>
using namespace std;
#include "Button.h"

Panel::Panel(){
    this->counter = 0;
    this->counter_change = false;
    this->click_change = false;
}

void Panel::on_module_loaded(){
    // Exit if this module is not enabled
    if( !this->kernel->config->value( panel_checksum, enable_checksum )->by_default(false)->as_bool() ){ return; } 

    // Register for events
    this->register_for_event(ON_IDLE);

    // Initialise the LCD
    this->lcd = new I2CLCD();
    this->lcd->init();

    this->lcd->printf("test");

    // Control buttons
    this->up_button           = (new Button())->pin( this->kernel->config->value( panel_checksum, up_button_pin_checksum    )->by_default("nc")->as_pin()->as_input() )->up_attach(   this, &Panel::on_up );
    this->down_button         = (new Button())->pin( this->kernel->config->value( panel_checksum, down_button_pin_checksum  )->by_default("nc")->as_pin()->as_input() )->up_attach(   this, &Panel::on_down );
    this->click_button        = (new Button())->pin( this->kernel->config->value( panel_checksum, click_button_pin_checksum )->by_default("nc")->as_pin()->as_input() )->down_attach( this, &Panel::on_click_release );
    this->kernel->slow_ticker->attach( 100, this, &Panel::button_tick );

}


// Read and update each button
uint32_t Panel::button_tick(uint32_t dummy){
    this->up_button->check_signal();
    this->down_button->check_signal();
    this->click_button->check_signal();
}


// Main loop things, we don't want to do shit in interrupts
void Panel::on_idle(void* argument){
    if( this->counter_change ){
        this->counter_change = false;
        this->lcd->clear();
        this->lcd->printf("value: %d", this->counter);
    }
    if( this->click_change ){
        this->click_change = false;
        this->lcd->clear();
        this->lcd->printf("click");
    }
}

uint32_t Panel::on_up(uint32_t dummy){
    this->counter++;
    this->counter_change = true;
}

uint32_t Panel::on_down(uint32_t dummy){
    this->counter--;
    this->counter_change = true;
}

uint32_t Panel::on_click_release(uint32_t dummy){
    this->click_change = true;
}




