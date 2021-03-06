/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  rover simulator class
*/

#ifndef _SIM_ROVER_H
#define _SIM_ROVER_H

#include "SIM_Aircraft.h"

/*
  a rover simulator
 */
class Rover : public Aircraft
{
public:
    Rover(const char *home_str, const char *frame_str);

    /* update model by one time step */
    void update(const struct sitl_input &input);

    /* static object creator */
    static Aircraft *create(const char *home_str, const char *frame_str) {
        return new Rover(home_str, frame_str);
    }

private:
    float max_speed;
    float max_accel;
    float wheelbase;
    float wheeltrack;
    float max_wheel_turn;
    float turning_circle;
    float skid_turn_rate;
    bool skid_steering;

    float turn_circle(float steering);
    float calc_yaw_rate(float steering, float speed);
    float calc_lat_accel(float steering_angle, float speed);
};


#endif // _SIM_ROVER_H
