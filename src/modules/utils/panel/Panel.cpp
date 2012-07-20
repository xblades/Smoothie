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
#include "PanelScreen.h"
#include "screens/MainMenuScreen.h"

Panel::Panel(){
    this->counter_changed = false;
    this->click_changed = false;
    this->refresh_flag = false;
    this->enter_menu_mode();
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
    this->encoder_a_pin       =                      this->kernel->config->value( panel_checksum, encoder_a_pin_checksum    )->by_default("nc")->as_pin()->as_input() ;
    this->encoder_b_pin       =                      this->kernel->config->value( panel_checksum, encoder_b_pin_checksum    )->by_default("nc")->as_pin()->as_input() ;
    this->click_button        = (new Button())->pin( this->kernel->config->value( panel_checksum, click_button_pin_checksum )->by_default("nc")->as_pin()->as_input() )->down_attach( this, &Panel::on_click_release );
    this->kernel->slow_ticker->attach( 100,  this, &Panel::button_tick );
    this->kernel->slow_ticker->attach( 1000, this, &Panel::encoder_check );

    // Default screen
    this->top_screen = (new MainMenuScreen())->set_panel(this);
    this->enter_screen(this->top_screen);

    // Refresh timer
    this->kernel->slow_ticker->attach( 20, this, &Panel::refresh_tick );

}

// Enter a screen, we only care about it now
void Panel::enter_screen(PanelScreen* screen){
    screen->panel = this;
    this->current_screen = screen;
    this->reset_counter();
    this->current_screen->on_enter();
}

// Reset the counter
void Panel::reset_counter(){
    *this->counter = 0;
    this->counter_changed = false;
}

// Indicate the idle loop we want to call the refresh hook in the current screen
uint32_t Panel::refresh_tick(uint32_t dummy){
    this->refresh_flag = true;
}

// Encoder pins changed
uint32_t Panel::encoder_check(uint32_t dummy){
    static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    static uint8_t old_AB = 0;
    static int encoder_counter = 0;
    old_AB <<= 2;                   //remember previous state
    old_AB |= ( this->encoder_a_pin->get() + ( this->encoder_b_pin->get() * 2 ) );  //add current state 
    int change =  enc_states[(old_AB&0x0f)];
    encoder_counter += change; 
    if( change != 0 ){ this->counter_changed = true; }
    if( encoder_counter % 2 == 0 ){
        (*this->counter) += change;
    }
}

// Read and update each button
uint32_t Panel::button_tick(uint32_t dummy){
    this->up_button->check_signal();
    this->down_button->check_signal();
    this->click_button->check_signal();
}

// Main loop things, we don't want to do shit in interrupts
void Panel::on_idle(void* argument){
    // If we are in menu mode and the position has changed
    if( this->mode == MENU_MODE && this->counter_change() ){
        this->menu_update();
    }

    // If we must refresh
    if( this->refresh_flag ){
        this->refresh_flag = false;
        this->current_screen->on_refresh();
    }
}

// Hooks for button clicks
uint32_t Panel::on_up(uint32_t dummy){ ++*this->counter; this->counter_changed = true; }
uint32_t Panel::on_down(uint32_t dummy){ --*this->counter; this->counter_changed = true; }
uint32_t Panel::on_click_release(uint32_t dummy){ this->click_changed = true; }
bool Panel::counter_change(){ if( this->counter_changed ){ this->counter_changed = false; return true; }else{ return false; } }
bool Panel::click(){ if( this->click_changed ){ this->click_changed = false; return true; }else{ return false; } }


// Enter menu mode
void Panel::enter_menu_mode(){
    this->mode = MENU_MODE;
    this->counter = &this->menu_selected_line;
    this->menu_changed = false;
}

void Panel::setup_menu(uint16_t rows, uint16_t lines){
    this->menu_selected_line = 0;
    this->menu_start_line = 0; 
    this->menu_rows = rows;
    this->menu_lines = lines;
}

void Panel::menu_update(){
    // Limits, up and down 
    this->menu_selected_line = this->menu_selected_line % this->menu_rows;
    while( this->menu_selected_line < 0 ){ this->menu_selected_line += this->menu_rows; }

    // What to display
    this->menu_start_line = 0;
    if( this->menu_rows > this->menu_lines ){
        if( this->menu_selected_line >= 2 ){
            this->menu_start_line = this->menu_selected_line - 1;
        }
        if( this->menu_selected_line > this->menu_rows - this->menu_lines ){
            this->menu_start_line = this->menu_rows - this->menu_lines;
        }
    }
    this->menu_changed = true;
}

bool Panel::menu_change(){
    if( this->menu_changed ){ this->menu_changed = false; return true; }else{ return false; }
}

