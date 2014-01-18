/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PLAYER_H
#define PLAYER_H
#include "libs/Module.h"
#include "libs/Kernel.h"
using namespace std;
#include <string>
#include <vector>

class Player : public Module {
    public:
        Player();

        void on_module_loaded();
        void on_idle(void* argument);
        void finish();
        void wait_for_queue(int free_blocks);
        Block* new_block();

        RingBuffer<Block,32> queue;  // Queue of Blocks
        Block* current_block;
        bool running;
};



#endif
