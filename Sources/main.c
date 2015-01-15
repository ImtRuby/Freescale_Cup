#include "derivative.h" /* include peripheral declarations */
#include "TFC\TFC.h"
#include "camera_processing.h"
#include "DistantIO\distantio.h"
#include "chrono.h"
#include "UnitTests/UnitTests.h"
#include "TFC/TFC_UART.h"

//TODO : Select servo offset with potard

// WARNING : SysTick frequency is 50kHz. Will overflow after roughly 2.5 hours
// Derivative not clean

void cam_program();
void configure_bluetooth();

int main(void)

{
	//test_serial1();
	//test_protocol1();
	//test_distantio_minimal();
	
	cam_program();
	
	//configure_bluetooth();
	
	return 0;
}

void cam_program()
{
	TFC_Init();
	//Init all 3 communication layers
	init_serial();
	init_protocol();
	init_distantio();
	
	cameraData data;
	
	init_data(&data);
	
	//Computation variables
	float position_error = 0.f, alpha_error = 0.8;
	float previous_error = 0.f;
	float error_derivative = 0.f, new_derivative = 0.f, alpha_deriv = 0.85;
	float filtered_error = 0.f, previous_filtered_error = 0.f;
	
	float command = 0.f,new_command = 0.f, alpha_command = 0.0;
	float servo_offset = 0.2;
	
	float commandD = 0.f;
	float commandP = 0.f;
	
	//      PID        
	
	//STABLE
	float P = 0.012;
	float D = 0.f;
	float command_engines = 0.4f;
	
	//Stable
	P = 0.018;
	D = 0.f;
	command_engines = 0.4f;
	
	//Start oscillating
	//P = 0.019;
	//D = 0.f;
	//command_engines = 0.4f;
	
	//NUL
	//P = 0.018f;
	//D = 0.001f;
	//command_engines = 0.4f;
	
	//To check loop times
	chrono chr_cam_m, chr_loop_m;
	//To ensure loop times
	chrono chr_distantio,chr_cam,chr_led,chr_servo;
	float t_cam = 0, t_loop = 0;
	float looptime_cam;
	float queue_size = 0;
	
	uint32_t  testESC = 0x007F7DF7;
	
	//Camera processing parameters
	data.threshold_coefficient = 0.65;
	data.edgeleft = 20;
	data.edgeright = 15;
	data.alpha = 0.25;
	
	//TFC_HBRIDGE_ENABLE;
	
	uint16_t pload;
	
	uint8_t led_state = 0;
	TFC_SetBatteryLED_Level(led_state);
	
	//Readonly variables
	register_scalar(&testESC,UINT32,0,"TestESC");
	register_scalar(&pload,UINT16,0,"Serial load");
	
	register_scalar(&t_cam,FLOAT,0,"cam time(ms)");
	register_scalar(&t_loop,FLOAT,0,"main time(ms)");
	register_scalar(&looptime_cam,FLOAT,0,"cam exe period(us)");
	
	register_scalar(&queue_size, FLOAT,0,"queue size");
	
	register_scalar(&position_error, FLOAT,0,"error");
	register_scalar(&error_derivative, FLOAT,0,"derivative");
	register_scalar(&command,FLOAT,0,"command");
	register_scalar(&commandP,FLOAT,0,"cmdP");
	register_scalar(&commandD,FLOAT,0,"cmdD");
	
	//Read/write variables
	register_scalar(&P,FLOAT,1,"P");
	register_scalar(&D,FLOAT,1,"D");
	register_scalar(&alpha_error,FLOAT,1,"alpha P");
	register_scalar(&alpha_deriv,FLOAT,1,"alpha_D");
	register_scalar(&alpha_command,FLOAT,1,"alpha cmd");
	register_scalar(&command_engines,FLOAT,1,"engines");
	register_scalar(&servo_offset,FLOAT,1,"servo_offset");
	
	register_scalar(&data.offset,FLOAT,0,"cmd offset");
	
	register_array(data.filtered_image,128,FLOAT,0,"filtered_line");
	register_array(data.raw_image,128,UINT16,0,"raw_line");
	
	float exposure_time_us = 10000;
	uint32_t servo_update_ms = 10;
	
	TFC_SetLineScanExposureTime(exposure_time_us);
	TFC_SetServo(0, servo_offset);
	
	Restart(&chr_distantio);
	Restart(&chr_led);
	Restart(&chr_servo);
	
	for(;;)
	{
			
		Restart(&chr_loop_m);					//**TIME MONITORING
		t_loop = GetLastDelay_ms(&chr_loop_m);	//**TIME MONITORING
		
		//TFC_Task must be called in your main loop.  This keeps certain processing happy (I.E. Serial port queue check)
		TFC_Task();
		
		Capture(&chr_distantio);
		if(GetLastDelay_us(&chr_distantio) > 10000)
		{
			Restart(&chr_distantio);
				
			update_distantio();
			pload = getPeakLoad();
		}			
		
		//Compute line position
		Capture(&chr_cam);
		looptime_cam = GetLastDelay_us(&chr_cam);
		if(looptime_cam > exposure_time_us)
		{
			Restart(&chr_cam);
			
			Restart(&chr_cam_m);				//**TIME MONITORING
			
			read_process_data(&data);			
				
				
			
			//Compute errors for PD
			previous_error = position_error;
			//position_error =  position_error * alpha_error + data.line_position * (1 - alpha_error);
			position_error = data.line_position;
			
			
			previous_filtered_error = filtered_error;
			filtered_error = filtered_error * alpha_deriv + (1 - alpha_deriv) * position_error;
			
			error_derivative = (filtered_error - previous_filtered_error) * 1000000.f / looptime_cam;
			
			commandP = P * position_error;
			commandD = D * error_derivative;
			
			//Compute servo command between -1.0 & 1.0
			new_command = commandD + commandP + servo_offset;
			//command = command * alpha_command + (1 - alpha_command) * new_command;
			command = new_command;
			
			if(command > 1.f)
				command = 1.f;
			if(command < -1.f)
				command = -1.f;
			
			Capture(&chr_cam_m); 				//**TIME MONITORING
			t_cam = GetLastDelay_ms(&chr_cam_m);//**TIME MONITORING
		}
		
		
		//Update servo command
		Capture(&chr_servo);
		if(GetLastDelay_ms(&chr_servo) > servo_update_ms)
		{
			Restart(&chr_servo);
			TFC_SetServo(0, command);
		}
		
		//Led state
		Capture(&chr_led);
		if(GetLastDelay_ms(&chr_led) > 500)
		{
			Restart(&chr_led);
			led_state ^= 0x01; 			
		}
		
		//Calibrate
		if(TFC_PUSH_BUTTON_0_PRESSED)
		{
			calibrate_data(&data);
			led_state = 3;
		}
		
		if(TFC_GetDIP_Switch() == 0)
		{
			TFC_HBRIDGE_DISABLE;
			TFC_SetMotorPWM(0 , 0);
		}
		else
		{
			TFC_HBRIDGE_ENABLE;
			TFC_SetMotorPWM(command_engines , command_engines);
		}
	
		TFC_SetBatteryLED_Level(led_state);
		Capture(&chr_loop_m);	//**TIME MONITORING
	}
}

void configure_bluetooth()
{
	TFC_Init();
		
		uart0_init (CORE_CLOCK/2/1000, 38400);
		 for(;;) //Try AT Command
		 {   
			 TFC_Task();
			
			 
			 if(TFC_Ticker[0]>1000)
			 {                   
				 
				 
				 serial_printf("AT+UART=115200,0,0\r\n");
				 TFC_Ticker[0] = 0;
				
				 TFC_Task();
				 
				 //Reset uart 
				 //uart0_init (CORE_CLOCK/2/1000, 115200);
			 }
		 }
}
