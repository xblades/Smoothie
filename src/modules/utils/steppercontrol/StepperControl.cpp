#include "libs/Kernel.h"
#include "StepperControl.h"
#include "libs/nuts_bolts.h"
#include "libs/utils.h"
#include "libs/LPC17xx/sLPC17xx.h"
#include "system_LPC17xx.h"

#include <string>
using namespace std;

StepperControl::StepperControl(PinName mosi, PinName miso, PinName sclk):_spi(mosi, miso, sclk) {}

void StepperControl::on_module_loaded(){
//	debug_cnt=0;

	unsigned char i;
	int divider;
	int scale;
	
/*
	printf("Checksum steppercontrol_module_enable %d\r\n",get_checksum("steppercontrol_module_enable"));
	printf("Checksum epsilon_current %d\r\n",get_checksum("epsilon_current"));
	printf("Checksum zeta_current %d\r\n",get_checksum("zeta_current"));
	printf("Checksum steppercontrol_cs0_pin %d\r\n",get_checksum("steppercontrol_cs0_pin"));
	printf("Checksum steppercontrol_cs1_pin %d\r\n",get_checksum("steppercontrol_cs1_pin"));
	printf("Checksum steppercontrol_cs2_pin %d\r\n",get_checksum("steppercontrol_cs2_pin"));
	printf("Checksum steppercontrol_cs3_pin %d\r\n",get_checksum("steppercontrol_cs3_pin"));
	printf("Checksum steppercontrol_cs4_pin %d\r\n",get_checksum("steppercontrol_cs4_pin"));
	printf("Checksum steppercontrol_cs5_pin %d\r\n",get_checksum("steppercontrol_cs5_pin"));
	printf("Checksum steppercontrol_power_pin %d\r\n",get_checksum("steppercontrol_power_pin"));
	printf("Checksum steppercontrol_enable_pin %d\r\n",get_checksum("steppercontrol_enable_pin"));
	printf("Checksum steppercontrol_frequency %d\r\n",get_checksum("steppercontrol_frequency"));
	printf("Checksum steppercontrol_scale0 %d\r\n",get_checksum("steppercontrol_scale0"));
	printf("Checksum steppercontrol_scale1 %d\r\n",get_checksum("steppercontrol_scale1"));
	printf("Checksum steppercontrol_scale2 %d\r\n",get_checksum("steppercontrol_scale2"));
	printf("Checksum steppercontrol_scale3 %d\r\n",get_checksum("steppercontrol_scale3"));
	printf("Checksum steppercontrol_scale4 %d\r\n",get_checksum("steppercontrol_scale4"));
	printf("Checksum steppercontrol_scale5 %d\r\n",get_checksum("steppercontrol_scale5"));

	printf("Checksum steppercontrol_chopper_off_time %d\r\n",get_checksum("steppercontrol_chopper_off_time"));
	printf("Checksum steppercontrol_chopper_hyst_start %d\r\n",get_checksum("steppercontrol_chopper_hyst_start"));
	printf("Checksum steppercontrol_chopper_hyst_low %d\r\n",get_checksum("steppercontrol_chopper_hyst_low"));
	printf("Checksum steppercontrol_chopper_hyst_dec %d\r\n",get_checksum("steppercontrol_chopper_hyst_dec"));
	printf("Checksum steppercontrol_chopper_mode %d\r\n",get_checksum("steppercontrol_chopper_mode"));
	printf("Checksum steppercontrol_chopper_random %d\r\n",get_checksum("steppercontrol_chopper_random"));
	printf("Checksum steppercontrol_chopper_blank_time %d\r\n",get_checksum("steppercontrol_chopper_blank_time"));
	printf("Checksum steppercontrol_smart_min %d\r\n",get_checksum("steppercontrol_smart_min"));
	printf("Checksum steppercontrol_smart_max %d\r\n",get_checksum("steppercontrol_smart_max"));
	printf("Checksum steppercontrol_smart_up %d\r\n",get_checksum("steppercontrol_smart_up"));
	printf("Checksum steppercontrol_smart_down %d\r\n",get_checksum("steppercontrol_smart_down"));
	printf("Checksum steppercontrol_smart_currentsave %d\r\n",get_checksum("steppercontrol_smart_currentsave"));
	printf("Checksum steppercontrol_stallguard_threshold %d\r\n",get_checksum("steppercontrol_stallguard_threshold"));
	printf("Checksum steppercontrol_stallguard_filter %d\r\n",get_checksum("steppercontrol_stallguard_filter"));
	printf("Checksum steppercontrol_reduce_power_factor %d\r\n",get_checksum("steppercontrol_reduce_power_factor"));
	printf("Checksum acceleration_time %d\r\n",get_checksum("acceleration_time"));
*/

    if( !this->kernel->config->value( steppercontrol_module_enable_checksum )->by_default(false)->as_bool() ){ return; } 

    // Get configuration
    this->current[0]   = this->kernel->config->value(alpha_current_checksum  )->by_default(0.5)->as_number(); 
    this->current[1]    = this->kernel->config->value(beta_current_checksum  )->by_default(0.5)->as_number(); 
    this->current[2]   = this->kernel->config->value(gamma_current_checksum  )->by_default(0.5)->as_number(); 
    this->current[3]   = this->kernel->config->value(delta_current_checksum  )->by_default(0.5)->as_number();
    this->current[4] = this->kernel->config->value(epsilon_current_checksum  )->by_default(0.5)->as_number();
    this->current[5]    = this->kernel->config->value(zeta_current_checksum  )->by_default(0.5)->as_number();
    this->_frequency      = this->kernel->config->value(steppercontrol_frequency_checksum  )->by_default(16000000)->as_number();
    
	this->_cs[0]          = this->kernel->config->value(steppercontrol_cs0_pin_checksum          )->by_default("nc" )->as_pin()->as_output();
	this->_cs[1]          = this->kernel->config->value(steppercontrol_cs1_pin_checksum          )->by_default("nc" )->as_pin()->as_output();
	this->_cs[2]          = this->kernel->config->value(steppercontrol_cs2_pin_checksum          )->by_default("nc" )->as_pin()->as_output();
	this->_cs[3]          = this->kernel->config->value(steppercontrol_cs3_pin_checksum          )->by_default("nc" )->as_pin()->as_output();
	this->_cs[4]          = this->kernel->config->value(steppercontrol_cs4_pin_checksum          )->by_default("nc" )->as_pin()->as_output();
	this->_cs[5]          = this->kernel->config->value(steppercontrol_cs5_pin_checksum          )->by_default("nc" )->as_pin()->as_output();

	this->_power          = this->kernel->config->value(steppercontrol_power_pin_checksum          )->by_default("nc" )->as_pin()->as_output();
	this->_enable         = this->kernel->config->value(steppercontrol_enable_pin_checksum          )->by_default("nc" )->as_pin()->as_output();
	
	this->_chopper_off_time		= round(this->kernel->config->value(steppercontrol_chopper_off_time_checksum          )->by_default(5)->as_number());
	this->_chopper_hyst_start	= round(this->kernel->config->value(steppercontrol_chopper_hyst_start_checksum          )->by_default(3)->as_number()); 
	this->_chopper_hyst_low		= round(this->kernel->config->value(steppercontrol_chopper_hyst_low_checksum          )->by_default(3)->as_number());
	this->_chopper_hyst_dec		= round(this->kernel->config->value(steppercontrol_chopper_hyst_dec_checksum          )->by_default(0)->as_number());
	this->_chopper_mode			= this->kernel->config->value( steppercontrol_chopper_mode_checksum )->by_default(false)->as_bool();
	this->_chopper_random		= this->kernel->config->value( steppercontrol_chopper_random_checksum )->by_default(false)->as_bool();
	this->_chopper_blank_time	= round(this->kernel->config->value(steppercontrol_chopper_blank_time_checksum          )->by_default(1)->as_number());

	this->_smart_min			= round(this->kernel->config->value(steppercontrol_smart_min_checksum          )->by_default(3)->as_number());
	this->_smart_max			= round(this->kernel->config->value(steppercontrol_smart_max_checksum          )->by_default(2)->as_number());
	this->_smart_up				= round(this->kernel->config->value(steppercontrol_smart_up_checksum          )->by_default(3)->as_number());
	this->_smart_down			= round(this->kernel->config->value(steppercontrol_smart_down_checksum          )->by_default(1)->as_number());
	this->_smart_currentsave	= this->kernel->config->value( steppercontrol_smart_currentsave_checksum )->by_default(false)->as_bool();
	
	this->_stallguard_threshold	= round(this->kernel->config->value(steppercontrol_stallguard_threshold_checksum          )->by_default(0)->as_number());
	this->_stallguard_filter	= this->kernel->config->value( steppercontrol_stallguard_filter_checksum )->by_default(true)->as_bool();
	
	this->_reduce_power_factor  = this->kernel->config->value(steppercontrol_reduce_power_factor_checksum          )->by_default(2.0)->as_number();
	
	this->register_for_event(ON_GCODE_EXECUTE);
	this->register_for_event(ON_CONFIG_RELOAD);
	this->register_for_event(ON_START);
	this->register_for_event(ON_FINISH);
	this->register_for_event(ON_IDLE);
	
	for (i=0; i<6; i++) {
		this->_cs[i]->set(1);
	}
	this->_enable->set(1);
	wait_ms(20);
	this->_power->set(1);
	wait_ms(20);
	
//	initialize refclock to 4..20 MHz -> 16MHz
	divider = round(SystemCoreClock/_frequency) -1;
	if (divider<4) divider=4;
	if (divider>15) divider=15;
//	printf("Enable Refclk\r\n");
//	printf("Divider Refclk %d    %f   %d\r\n",SystemCoreClock,_frequency,divider);
	LPC_SC->CLKOUTCFG = 0x00000100 | (divider<<4);		// cpu_clk, divider, eneable
	LPC_PINCON->PINSEL3 &= ~(3<<22);
	LPC_PINCON->PINSEL3 |= (1<<22);			// 23:22 = 01

    _spi.format(10,3);
    _spi.frequency(2000000);
    
    _rdsel = Microstep;
	_rdsel = Current;
	_reduce_power = true;
	
	_config_tmc();

	wait_ms(20);
	this->_enable->set(0);
}

void StepperControl::on_gcode_execute(void* argument){
    Gcode* gcode = static_cast<Gcode*>(argument);
    // Absolute/relative mode
    if( gcode->has_letter('M')){
//    _rdsel = Microstep;	
	    _rdsel = Stallguard;	
		setreg_DRVCONF(0, _rdsel);
    }	
}

void StepperControl::on_config_reload(void* argument){
    this->current[0] = this->kernel->config->value(alpha_current_checksum    )->by_default(0.5)->as_number(); 
    this->current[1] = this->kernel->config->value(beta_current_checksum     )->by_default(0.5)->as_number(); 
    this->current[2] = this->kernel->config->value(gamma_current_checksum    )->by_default(0.5)->as_number(); 
    this->current[3] = this->kernel->config->value(delta_current_checksum    )->by_default(0.5)->as_number();
    this->current[4] = this->kernel->config->value(epsilon_current_checksum  )->by_default(0.5)->as_number();
    this->current[5] = this->kernel->config->value(zeta_current_checksum     )->by_default(0.5)->as_number();

	_config_tmc();
}

void StepperControl::on_start(void* argument) {
	if (_reduce_power) {
		_reduce_power = false;
		_config_tmc();
	}
}

void StepperControl::on_finish(void* argument) {
	if (!_reduce_power) {
		_reduce_power = true;
		_config_tmc();
	}
};

void StepperControl::_config_tmc() {
//	this->_enable->set(1);
	unsigned char i;
	int scale;
	for (i=0; i<6; i++) {
		// 31 = CurrentMax 1.65A @ 0R1 Sense Resistor and VSense_Setting = 165mV
		if (_reduce_power) {
			scale = round(31*current[i]/1.65/_reduce_power_factor);	
		} else {
			scale = round(31*current[i]/1.65);
		}
		if (scale>31) scale=31;
		if (scale<=1) scale=0;
//printf("scale %d\r\n",scale);
//if (scale>5) scale=5; // for safety reasons with rsense=0R015

		setreg_SMARTEN(i, 0, 0, 0, 0, false);
//		setreg_CHOPCONF(i, 7, 5, 10, 0, false, true , 2);
		setreg_CHOPCONF(i, _chopper_off_time, _chopper_hyst_start, _chopper_hyst_low, _chopper_hyst_dec, _chopper_random, _chopper_mode, _chopper_blank_time);
		setreg_SGCSCONF(i, scale, _stallguard_threshold, _stallguard_filter);
		setreg_DRVCONF(i, _rdsel);
		setreg_DRVCTRL(i, true, step16);		// interpolate at 16 Microsteps
		setreg_SMARTEN(i, _smart_min, _smart_max, _smart_up, _smart_down, _smart_currentsave);
	}    
//	this->_enable->set(0);
}

void StepperControl::_spi_write(int stepper, unsigned long value) {
	this->_cs[stepper]->set(0);
	_status = _spi.write(value >> 10);
	_status = (_status<<10);
    _status |= _spi.write(value & 0x3FF);
	this->_cs[stepper]->set(1);
//	printf("Steppercontrol %d  %5x    Status %2x    Position %3x\r\n",stepper,value,_status & 0x0f,_status>>10);
}

void StepperControl::setreg_DRVCTRL(int stepper, bool interpol, microstep step){
	unsigned long value;
	value = step;
	if (interpol) value |= (1<<9);
	_spi_write(stepper,value);
}

void StepperControl::setreg_CHOPCONF(int stepper, unsigned int off_time, unsigned int hyst_start, unsigned int hyst_low, unsigned int hyst_dec, bool random, bool mode, unsigned int blank_time){
	unsigned long value;
	value = (1<<19);
	if ((off_time<16) && (off_time>1)) value |= off_time;
	if (hyst_start<8) value |= (hyst_start<<4);
	if (hyst_low<16) value |= (hyst_low<<7);
	if (hyst_dec<4) value |= (hyst_dec<<11);
	if (random) value |= (1<<13);
	if (mode) value |= (1<<14);
	if (blank_time<4) value |= (blank_time<<15);
	_spi_write(stepper,value);
}

void StepperControl::setreg_SMARTEN(int stepper, unsigned int min, unsigned int max, unsigned int up, unsigned int down, bool currentsave){
	unsigned long value;
	value = (1<<19) | (1<<17);
	if (min<16) value |= min;
	if (max<16) value |= (max<<8);
	if (up<4) value |= (up<<5);
	if (down<4) value |= (down<<13);
	if (currentsave) value |= (1<<15);	// min 1/4 current (Bit 15)
	_spi_write(stepper,value);
}

void StepperControl::setreg_SGCSCONF(int stepper, unsigned int scale, int stall_level, bool stall_filter){
	unsigned long value;
	value = (1<<19) | (1<<18);
	if (scale<32) value |= scale;
	if ((stall_level>-64) && (stall_level<63)) value |= (stall_level<<8);
	if (stall_filter) value |= (1<<16);	// stallguard filter disabled (Bit16)
	_spi_write(stepper,value);
}

void StepperControl::setreg_DRVCONF(int stepper, rdsel RDSEL){
	unsigned long value;
	value = (1<<19) | (1<<18) | (1<<17);
	value |= (RDSEL<<4);
	value |= (1<<6);	// VSENSE=165mV (Bit6) -> 1.2A max - check this
//	value |= (1<<7);	// SDOFF=0 (Bit 7)
//	value |= (1<<10);	// don't set to 1 (turn off) - seek for the reason (current too high ?)
	// short to GND protection on (Bit 10)
	// with 3.2us (Bit 9..8)
	// slope is min (Bit 15..14, 13..12)
	_spi_write(stepper,value);
}

/*
void StepperControl::stepper(char typs, unsigned d1, unsigned d2, unsigned d3, unsigned d4, unsigned d5, unsigned d6, unsigned d7, unsigned d8){
	unsigned int n;
	n=debug_cnt;
	if (debug_cnt>=debug_max) n=debug_cnt % debug_max;
	typ[n]=typs;
	data[1][n]=d1;
	data[2][n]=d2;
	data[3][n]=d3;
	data[4][n]=d4;
	data[5][n]=d5;
	data[6][n]=d6;
	debug_cnt++;
}
*/


/*
SPI = $901B4; // Hysteresis mode
or
SPI = $94557; // Const. toff mode
SPI = $D001F; // Current setting: $d001F (max. current)
SPI = $EF010; // high gate driver strength, stallGuard read, SDOFF=0 for TMC262
or
SPI = $E0010; // low driver strength, stallGuard read, SDOFF=0 for TMC260/261
SPI = $00000; // 256 microstep setting
First test of coolStep™ current control:
SPI = $A8202; // Enable coolStep with minimum current 1/4 CS
*/