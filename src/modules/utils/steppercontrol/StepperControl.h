#ifndef STEPPERCONTROL_H
#define STEPPERCONTROL_H

#include "libs/Kernel.h"
#include "libs/nuts_bolts.h"
#include "libs/utils.h"
#include "libs/Pin.h"
#include "mbed.h"

#define steppercontrol_module_enable_checksum 50618
#define alpha_current_checksum   22381 
#define beta_current_checksum    60163 
#define gamma_current_checksum   12906 
#define delta_current_checksum   30321
#define epsilon_current_checksum 50274
#define zeta_current_checksum    3355
#define steppercontrol_cs0_pin_checksum 536
#define steppercontrol_cs1_pin_checksum 1817
#define steppercontrol_cs2_pin_checksum 3098
#define steppercontrol_cs3_pin_checksum 4379
#define steppercontrol_cs4_pin_checksum 5660
#define steppercontrol_cs5_pin_checksum 6941
#define steppercontrol_power_pin_checksum 51008
#define steppercontrol_enable_pin_checksum 63866
#define steppercontrol_frequency_checksum 28479
#define steppercontrol_chopper_off_time_checksum 47113
#define steppercontrol_chopper_hyst_start_checksum 18710
#define steppercontrol_chopper_hyst_low_checksum 34873
#define steppercontrol_chopper_hyst_dec_checksum 18451
#define steppercontrol_chopper_mode_checksum 7779
#define steppercontrol_chopper_random_checksum 11840
#define steppercontrol_chopper_blank_time_checksum 49110
#define steppercontrol_smart_min_checksum 34615
#define steppercontrol_smart_max_checksum 33081
#define steppercontrol_smart_up_checksum 26583
#define steppercontrol_smart_down_checksum 11691
#define steppercontrol_smart_currentsave_checksum 59816
#define steppercontrol_stallguard_threshold_checksum 35792
#define steppercontrol_stallguard_filter_checksum 648
#define steppercontrol_reduce_power_factor_checksum 24659

#define debug_max 60

class StepperControl : public Module {
    public:
        StepperControl(PinName mosi, PinName miso, PinName sclk);
       
        void on_module_loaded();
		void on_gcode_execute(void* argument);
        void on_config_reload(void* argument);
		void on_start(void* argument);
		void on_finish(void* argument);
       
//		void stepper(char typs, unsigned d1, unsigned d2, unsigned d3, unsigned d4, unsigned d5, unsigned d6, unsigned d7, unsigned d8); 
    	double current[6];
    
    private:
		typedef enum {
			Microstep = 0,	
			Stallguard,		
			Current		
		} rdsel;

		typedef enum {
			step256 = 0,	
			step128,
			step64,
			step32,
			step16,
			step8,
			step4,
			step2,
			fullstep		
		} microstep;

    	SPI _spi;
		Pin* _cs[6];
		Pin* _power;
		Pin* _enable;
		double _frequency;
		unsigned long _status;
		rdsel _rdsel;

		bool _reduce_power;
		double _reduce_power_factor;
		
		unsigned char _chopper_off_time;
		unsigned char _chopper_hyst_start;
		unsigned char _chopper_hyst_low;
		unsigned char _chopper_hyst_dec;
		bool _chopper_mode;
		bool _chopper_random;
		unsigned char _chopper_blank_time;
	
		unsigned char _smart_min;
		unsigned char _smart_max;
		unsigned char _smart_up;
		unsigned char _smart_down;
		bool _smart_currentsave;
		
		unsigned char _stallguard_threshold;
		unsigned char _stallguard_filter;

//		unsigned int debug_start;
//		unsigned int debug_cnt;
//		unsigned int data[6][debug_max];
//		char typ[debug_max];
		    	
		void _config_tmc();
		void _spi_write(int stepper, unsigned long value);
		void setreg_DRVCTRL(int stepper, bool interpol, microstep step);
		void setreg_CHOPCONF(int stepper, unsigned int off_time, unsigned int hyst_start, unsigned int hyst_low, unsigned int hyst_dec, bool random, bool mode, unsigned int blank_time);
    	void setreg_SMARTEN(int stepper, unsigned int min, unsigned int max, unsigned int up, unsigned int down, bool currentsave);
    	void setreg_SGCSCONF(int stepper, unsigned int scale, int stall_level, bool stall_filter);
		void setreg_DRVCONF(int stepper, rdsel RDSEL);
};





#endif
