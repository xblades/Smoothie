/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl) with additions from Sungeun K. Jeon (https://github.com/chamnit/grbl)
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

using namespace std;
#include <vector>
#include "libs/nuts_bolts.h"
#include "libs/RingBuffer.h"
#include "../communication/utils/Gcode.h"
#include "libs/Module.h"
#include "libs/Kernel.h"
#include "Timer.h" // mbed.h lib
#include "wait_api.h" // mbed.h lib
#include "Block.h"
#include "Player.h"
#include "Planner.h"

Player::Player(){
    this->current_block = NULL;
    this->running = false;
}

void Player::on_module_loaded(){
    this->register_for_event(ON_IDLE); 
}

void Player::on_idle(void* argument){
	if ((!this->running) && (this->queue.size() > 0)) {
        if (this->queue.get_ref(0)->is_ready){
        	this->running = true;
            this->kernel->call_event(ON_START);
            this->current_block = this->queue.get_ref(0);
//printf("a\r\n");
            if ( this->current_block->gcode_valid ) {
	            if (! this->current_block->gcode.on_gcode_execute_event_called) {
    	            this->current_block->gcode.on_gcode_execute_event_called = true;
//printf("G1: %s \r\n", this->current_block->gcode.command.c_str() ); 
                    this->kernel->call_event(ON_GCODE_EXECUTE, &(this->current_block->gcode));
                }
            }
            this->kernel->call_event(ON_BLOCK_BEGIN, this->current_block);
            // In case the module was not taken
            if( this->current_block->times_taken < 1 ){
                kernel->call_event(ON_BLOCK_END, this->current_block);
                this ->running = false;
                if( this->queue.size() > 0 ){ 
                    this->queue.delete_first();
                } 
                this->kernel->call_event(ON_FINISH);
            }
        }
    }
}

void Player::finish(){
    this->kernel->call_event(ON_BLOCK_END, this);
    this->current_block->is_ready = false;

    if( this->queue.size() > 0 ){ 
        this->queue.delete_first();
    } 

    if( this->queue.size() > 0 ){
    	while( this->queue.size() > 0 ){
            if( this->queue.get_ref(0)->is_ready ){
                this->current_block = this->queue.get_ref(0);
                if ( this->current_block->gcode_valid ) {
                    if (! this->current_block->gcode.on_gcode_execute_event_called) {
  	                    this->current_block->gcode.on_gcode_execute_event_called = true;
                    }
//printf("GCODE Z: %s \r\n", candidate->gcode.command.c_str() ); 
                    this->kernel->call_event(ON_GCODE_EXECUTE, &(this->current_block->gcode));
                }
                this->kernel->call_event(ON_BLOCK_BEGIN, this->current_block);
                if( this->current_block->times_taken < 1 ){
                    this->kernel->call_event(ON_BLOCK_END, this->current_block);
                    this->current_block->is_ready = false;
                    if( this->queue.size() > 0 ){ 
                        this->queue.delete_first();
                    }
                    if( this->queue.size() == 0 ){ 
                        this->kernel->call_event(ON_FINISH);
                        this->running = false;
                        return;
                    }
                } else {
                    return;
                } 

            }else{
                this->kernel->call_event(ON_FINISH);
                this->running = false;
                return;
            } 
        } 
    }else{
        this->kernel->call_event(ON_FINISH);
        this->running = false;
    }
}

// Append a block to the list
Block* Player::new_block(){

    // Clean up the vector of commands in the block we are about to replace
    // It is quite strange to do this here, we really should do it inside Block->pop_and_execute_gcode
    // but that function is called inside an interrupt and thus can break everything if the interrupt was trigerred during a memory access
    
    // Take the next untaken block on the queue ( the one after the last one )
    Block* block = this->queue.get_ref( this->queue.size() );
    
    // Create a new virgin Block in the queue
    this->queue.push_back(Block());
//printf("index %d\r\n",index);    
    block = this->queue.get_ref( this->queue.size()-1 );
    while( block == NULL ){
        block = this->queue.get_ref( this->queue.size()-1 );
    }
    block->is_ready = false;
    block->initial_rate = -2;
    block->final_rate = -2;
    block->player = this;
    block->steps_per_minute = 0.0;
    block->gcode_valid = false;
    block->moving_block = false;
//    block->index = index;
    return block;
}

void Player::wait_for_queue(int free_blocks){
    mbed::Timer t;
    while( this->queue.size() >= this->queue.capacity()-free_blocks ){
        t.reset();
        t.start();
        this->kernel->call_event(ON_IDLE);
        t.stop();
        if(t.read_us() < 500)
            wait_us(500 - t.read_us());
    }
}
