/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include "libs/Module.h"
#include "libs/Kernel.h"
#include "libs/nuts_bolts.h"
#include <math.h>
#include <string>
#include "Block.h"
#include "Planner.h"
#include "Player.h"
using std::string;
#include <vector>
#include "../communication/utils/Gcode.h"

Block::Block(){
    clear_vector(this->steps);
    this->times_taken = 0;   // A block can be "taken" by any number of modules, and the next block is not moved to until all the modules have "released" it. This value serves as a tracker.
    this->is_ready = false;
    this->initial_rate = -1;
    this->final_rate = -1;
    this->steps_finished = 0;
    this->steps_per_minute = 0.0;
    this->gcode_valid = false;
    this->moving_block = false;
    this->gcode.on_gcode_execute_event_called = true;
}

void Block::debug(Kernel* kernel){
    kernel->streams->printf("%p: steps:%4d|%4d|%4d(max:%4d) nominal:r%10d/s%6.1f mm:%9.6f rdelta:%8f acc:%5d dec:%5d rates:%10d>%10d taken:%d ready:%d \r\n", this, this->steps[0], this->steps[1], this->steps[2], this->steps_event_count, this->nominal_rate, this->nominal_speed, this->millimeters, this->rate_delta, this->accelerate_until, this->decelerate_after, this->initial_rate, this->final_rate, this->times_taken, this->is_ready );
}


// Calculate a braking factor to reach baseline speed which is max_jerk/2, e.g. the
// speed under which you cannot exceed max_jerk no matter what you do.
double Block::compute_factor_for_safe_speed(){
    return( this->planner->max_jerk / this->nominal_speed );
}


// Calculates trapezoid parameters so that the entry- and exit-speed is compensated by the provided factors.
// The factors represent a factor of braking and must be in the range 0.0-1.0.
//                                +--------+ <- nominal_rate
//                               /          \
// nominal_rate*entry_factor -> +            \
//                              |             + <- nominal_rate*exit_factor
//                              +-------------+
//                                  time -->
void Block::calculate_trapezoid_part( double entryfactor, double exitfactor, unsigned int steps_event_count){

    //this->player->kernel->streams->printf("%p calculating trapezoid\r\n", this);

    this->initial_rate = ceil(entryfactor * (float)this->nominal_rate);   // (step/min) 
    this->final_rate   = ceil(exitfactor * (float)this->nominal_rate);    // (step/min)

    //this->player->kernel->streams->printf("initrate:%f finalrate:%f\r\n", this->initial_rate, this->final_rate);

    double acceleration_per_minute = this->rate_delta * this->planner->kernel->stepper->acceleration_ticks_per_second * 60.0; // ( step/min^2)
    int accelerate_steps = ceil( this->estimate_acceleration_distance( this->initial_rate, this->nominal_rate, acceleration_per_minute ) );
    int decelerate_steps = floor( this->estimate_acceleration_distance( this->nominal_rate, this->final_rate,  -acceleration_per_minute ) );


    // Calculate the size of Plateau of Nominal Rate.
    int plateau_steps = steps_event_count-accelerate_steps-decelerate_steps;

    //this->player->kernel->streams->printf("accelperminute:%f accelerate_steps:%d decelerate_steps:%d plateau:%d \r\n", acceleration_per_minute, accelerate_steps, decelerate_steps, plateau_steps );

   // Is the Plateau of Nominal Rate smaller than nothing? That means no cruising, and we will
   // have to use intersection_distance() to calculate when to abort acceleration and start braking
   // in order to reach the final_rate exactly at the end of this block.
   if (plateau_steps < 0) {
       accelerate_steps = ceil(this->intersection_distance(this->initial_rate, this->final_rate, acceleration_per_minute, steps_event_count));
       accelerate_steps = max( accelerate_steps, 0 ); // Check limits due to numerical round-off
       accelerate_steps = min( accelerate_steps, int(steps_event_count) );
       plateau_steps = 0;
   }

   if (steps_event_count < this->steps_event_count) {
       this->accelerate_until = this->steps_event_count + accelerate_steps - steps_event_count;
       this->decelerate_after = this->steps_event_count + accelerate_steps+plateau_steps - steps_event_count; 
   } else {
   this->accelerate_until = accelerate_steps;
   this->decelerate_after = accelerate_steps+plateau_steps;
   }

   //this->debug(this->player->kernel);

   /*
   // TODO: FIX THIS: DIRTY HACK so that we don't end too early for blocks with 0 as final_rate. Doing the math right would be better. Probably fixed in latest grbl
   if( this->final_rate < 0.01 ){
        this->decelerate_after += floor( this->nominal_rate / 60 / this->planner->kernel->stepper->acceleration_ticks_per_second ) * 3;
    }
    */
}

void Block::calculate_trapezoid( double entryfactor, double exitfactor ){
	calculate_trapezoid_part(entryfactor, exitfactor, this->steps_event_count);
}

// Calculates the distance (not time) it takes to accelerate from initial_rate to target_rate using the
// given acceleration:
double Block::estimate_acceleration_distance(double initialrate, double targetrate, double acceleration) {
      return( ((targetrate*targetrate)-(initialrate*initialrate))/(2L*acceleration));
}

// This function gives you the point at which you must start braking (at the rate of -acceleration) if
// you started at speed initial_rate and accelerated until this point and want to end at the final_rate after
// a total travel of distance. This can be used to compute the intersection point between acceleration and
// deceleration in the cases where the trapezoid has no plateau (i.e. never reaches maximum speed)
//
/*                          + <- some maximum rate we don't care about
                           /|\
                          / | \
                         /  |  + <- final_rate
                        /   |  |
       initial_rate -> +----+--+
                            ^ ^
                            | |
        intersection_distance distance */
double Block::intersection_distance(double initialrate, double finalrate, double acceleration, double distance) {
   return((2*acceleration*distance-initialrate*initialrate+finalrate*finalrate)/(4*acceleration));
}

// Calculates the maximum allowable speed at this point when you must be able to reach target_velocity using the
// acceleration within the allotted distance.
inline double max_allowable_speed(double acceleration, double target_velocity, double distance) {
  return(
    sqrt(target_velocity*target_velocity-2L*acceleration*distance)  //Was acceleration*60*60*distance, in case this breaks, but here we prefer to use seconds instead of minutes
  );
}


// Called by Planner::recalculate() when scanning the plan from last to first entry.
void Block::reverse_pass(Block* next){

    if (next) {
        // If entry speed is already at the maximum entry speed, no need to recheck. Block is cruising.
        // If not, block in state of acceleration or deceleration. Reset entry speed to maximum and
        // check for maximum allowable speed reductions to ensure maximum possible planned speed.
        if (this->entry_speed != this->max_entry_speed) {

            // If nominal length true, max junction speed is guaranteed to be reached. Only compute
            // for max allowable speed if block is decelerating and nominal length is false.
            if ((!this->nominal_length_flag) && (this->max_entry_speed > next->entry_speed)) {
                this->entry_speed = min( this->max_entry_speed, max_allowable_speed(-this->acceleration,next->entry_speed,this->millimeters));
            } else {
                this->entry_speed = this->max_entry_speed;
            }
            this->recalculate_flag = true;

        }
    } // Skip last block. Already initialized and set for recalculation.

}


// Called by Planner::recalculate() when scanning the plan from first to last entry.
void Block::forward_pass(Block* previous){

    if(!previous) { return; } // Begin planning after buffer_tail

    unsigned int steps_finished = previous->steps_finished;

    // If the previous block is an acceleration block, but it is not long enough to complete the
    // full speed change within the block, we need to adjust the entry speed accordingly. Entry
    // speeds have already been reset, maximized, and reverse planned by reverse planner.
    // If nominal length is true, max junction speed is guaranteed to be reached. No need to recheck.
    if (!previous->nominal_length_flag) {
        if (previous->entry_speed < this->entry_speed) {
            double entry_speed = min( this->entry_speed, max_allowable_speed(-this->acceleration,previous->entry_speed,previous->millimeters) );
            if (steps_finished > 0) {	// already moving and not accellerating, so previous->milimeters is smaller
            	double speed = previous->steps_per_minute / previous->nominal_rate * nominal_speed;
          		speed = max_allowable_speed(-this->acceleration,speed,previous->millimeters*(previous->steps_event_count-steps_finished)/previous->steps_event_count);
                entry_speed = min( entry_speed, speed);
                // previous is a active block with moving steppers -> detect early a higher speed needed
                if (entry_speed > this->entry_speed) { // we are to slow -> quick increase accelerate_until
                	previous->accelerate_until = previous->steps_event_count; // calculate properly afterwards in calculate_trapezoid_part
                }
            }
          // Check for junction speed change
          if (this->entry_speed != entry_speed) {
            this->entry_speed = entry_speed;
            this->recalculate_flag = true;
          }
        }
    }

    if (this->recalculate_flag || previous->recalculate_flag) {
        previous->calculate_trapezoid_part(previous->entry_speed/previous->nominal_speed, this->entry_speed/previous->nominal_speed,previous->steps_event_count - steps_finished);
        previous->recalculate_flag = false;
    }
}


// Gcodes are attached to their respective blocks so that on_gcode_execute can be called with it
void Block::append_gcode(Gcode* gcode){
	this->gcode=*gcode;
}

// Signal the player that this block is ready to be injected into the system
void Block::ready(){
    this->is_ready = true;
}

// Mark the block as taken by one more module
void Block::take(){
    this->times_taken++;
    //printf("taking %p times now:%d\r\n", this, int(this->times_taken) );
}

// Mark the block as no longer taken by one module, go to next block if this free's it
void Block::release(){
    //printf("release %p \r\n", this );
    this->times_taken--;
    //printf("releasing %p times now:%d\r\n", this, int(this->times_taken) );
    if( this->times_taken < 1 ){
    	this->player->finish();
    }
}



