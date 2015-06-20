/************************************************************************************************************
* Library: TimedAutomata																					*
* Author: Abhishek N. Kulkarni	(abhibp1993)																*
*																											*
* Description:																								*
*	The library provides an easy interface to implement TimedAutomata where	the 							*
* 	transitions are check during the ticks issued by an internal timer. The timer							*
*	timer can be configured to tick at intervals in range 50us to 4ms, while the							*
* 	machine can be configured to tick at any integral multiple of tick interval.							*
*	Refer to my webpage (??link??) for tutorial.															*
*																											*	
*	The library is inspired by MsTimer2 and StateMachine libraries.											*
*																											*
* License:																									*
*																											*
* Copyright 2015 Abhishek N. Kulkarni (abhi.bp1993@gmail.com)												*
*																											*
* This program is free software: you can redistribute it and/or modify										*
* it under the terms of the GNU General Public License as published by										*
* the Free Software Foundation, either version 3 of the License, or											*
* (at your option) any later version.																		*
*																											*
* This program is distributed in the hope that it will be useful,											*
* but WITHOUT ANY WARRANTY; without even the implied warranty of											*
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the												*
* GNU General Public License for more details.																*
*																											*
* You should have received a copy of the GNU General Public License											*
* along with this program.  If not, see <http://www.gnu.org/licenses/>.			                        	*
 ***********************************************************************************************************/

#include "TimedAutomata1.h"

SM* mySM = NULL;

//====================================================================================
// TickTimer Implementation

unsigned long TickTimer::tickTime;
volatile unsigned char TickTimer::_tcnt_reload;
volatile unsigned char TickTimer::_callback_array_head;

uint8_t tccr2b_value;

callback arrCallback[MAX_CALLBACK];


/******************************************************************
Function: configure
Parameters: 
	1. tickTime_us: The time after which the timer issues ticks, 
		that is calls the registered callbacks.

Assumptions:
	1. Timer2 is free and not used anywhere else.
	2. IC is Atmega328
	
Remarks: 
	The function is a low-level function and sets the Timer2 
	registers in AVR. If any other library is used which tweaks
	these registers, this library can cause DISASTER!
	
Warning: (issued if <Log.h> is defined)
	W001: If unacceptable tickTime_us is provided.

Error: (issued if <Log.h> is defined)
	None
	
******************************************************************/
void TickTimer::configure(unsigned long tickTime_us){

  tickTime = tickTime_us;              // Set the tickTime (in microseconds) for reference
  
  TCCR2A = (1<<WGM21);                 // Timer2: CTC Mode, OC2A disconnected
  
  switch (tickTime_us){                // Appropriately configure time interval of tick
    case 50:
      tccr2b_value = (1<<CS21) | (1<<CS20);      // Prescalar = 32; 25 counts to 50us
      _tcnt_reload = 256 - 25;
      break;
      
    case 100:
      tccr2b_value = (1<<CS21) | (1<<CS20);      // Prescalar = 32; 50 counts to 100us
      _tcnt_reload = 256 - 50;
      break;
      
    case 200:
      tccr2b_value = (1<<CS21) | (1<<CS20);      // Prescalar = 32; 100 counts to 200us
      _tcnt_reload = 256 - 100;
      break;
      
    case 500:
      tccr2b_value = (1<<CS22);                  // Prescalar = 64; 125 counts to 500us
      _tcnt_reload = 256 - 125;
      break;
      
    case 1000:
      tccr2b_value = (1<<CS22);                  // Prescalar = 64; 250 counts to 1000us
      _tcnt_reload = 256 - 250;
      break;
      
    case 2000:
      tccr2b_value = (1<<CS22) | (1<<CS20);      // Prescalar = 128; 250 counts to 2000us
      _tcnt_reload = 256 - 125;
      break;
      
    case 4000:
      tccr2b_value = (1<<CS22) | (1<<CS21);      // Prescalar = 256; 250 counts to 4000us
      _tcnt_reload = 256 - 125;
      break;
    
    default:
      TCCR2B = 0;                          // Default: Timer is off
      tickTime = 0;
      #ifdef LOG_H
        warn(W001);
      #endif
      break;
  }

}

/******************************************************************
Function: registerCallback
Parameters: 
	1. fcn: type callback:: void (*fcn)(): Should be a function with
		no input, no output arguments.


Remarks: 
	As there is limit on number of callbacks that can be registered, 
	a warning will be issues (if enabled globally) in case the 
	number exceeds the MAX_CALLBACK

Warning: (issued if <Log.h> is defined)
	W002: If more than MAX_CALLBACK callbacks are attempted to register.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
void TickTimer::registerCallback(callback fcn){
  if (_callback_array_head >= MAX_CALLBACK){
    // Ignore (Do not add callback. Issue warning)
    #ifdef LOG_H
        warn(W002);
    #endif
  }
  else{
    arrCallback[TickTimer::_callback_array_head] = fcn;      // Add function pointer to list of callbacks
    _callback_array_head++;                       // Increment head
  }
}

/******************************************************************
Function: startTicking()
Parameters: None

Remarks: 
	Starts the timer and enables the timer_overflow interrupt.
	
Warning: (issued if <Log.h> is defined)
	None

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
void TickTimer::startTicking(){
  TCCR2B = tccr2b_value;               // Start the timer
  noInterrupts();                      // Disable interrupts
  TCNT2  = _tcnt_reload;               // Auto-Reload value
  TIMSK2 = (1<<TOIE2);                 // Enable OVF interrupt on Timer 2
  interrupts();                        // Enable interrupts
}

/******************************************************************
Function: stopTimer
Parameters: None

Remarks: 
	Stops the Timer2 and disables the timer_overflow interrupt.

Warning: (issued if <Log.h> is defined)
	None

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
void TickTimer::stopTicking(){
  TCCR2B = 0;                          // Stop the timer
  noInterrupts();                      // Disable interrupts
  TIMSK2 &= ~(1<<TOIE2);               // Disable OVF interrupt on Timer 2
  interrupts();                        // Enable interrupts
}



ISR(TIMER2_OVF_vect){
  for (uint8_t i = 0; i < TickTimer::_callback_array_head; i++){    
    arrCallback[i]();
  }
  
  if (mySM != NULL) {
	(*mySM).tick();
  }
	
  TCNT2 = TickTimer::_tcnt_reload;
  
}



//====================================================================================
// State class Implementation

/******************************************************************
Function: State (constructor)
Parameters: 
	1. fcn: type updateFcn:: void (*fcn)(): Should be a function with
		no input, no output arguments.


Remarks: 
	Constructs the State class object. Defaults the softDeadline, 
	and hardDeadlines to -1 (== -2^32 + 1);

Warning: (issued if <Log.h> is defined)
	None.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
State::State(updateFcn fcn){
  myFcn = fcn;
  
  inProgress = false;
  softDeadline = -1;
  hardDeadline = -1;
}

/******************************************************************
Function: State (constructor)
Parameters: 
	1. fcn: type updateFcn:: void (*fcn)(): Should be a function with
		no input, no output arguments.
	2. hard_deadline_ticks: Number of ticks beyond which state is 
		not allowed to take for processing.

Remarks: 
	Constructs the State class object. Defaults the softDeadline, 
	to -1 (== -2^32 + 1);

Warning: (issued if <Log.h> is defined)
	None.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
State::State(updateFcn fcn, unsigned long hard_deadline_ticks){
  myFcn = fcn;
  
  inProgress = false;
  softDeadline = -1;
  hardDeadline = hard_deadline_ticks;
}

/******************************************************************
Function: State (constructor)
Parameters: 
	1. fcn: type updateFcn:: void (*fcn)(): Should be a function with
		no input, no output arguments.
	2. hard_deadline_ticks: Number of ticks beyond which state is 
		not allowed to take for processing.
	3. soft_deadline_ticks: Number of ticks beyond which state is 
		not allowed to take for processing.
		
Remarks: 
	Constructs the State class object. 

Warning: (issued if <Log.h> is defined)
	None.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
State::State(updateFcn fcn, unsigned long hard_deadline_ticks, unsigned long soft_deadline_ticks){
  myFcn = fcn;
  
  inProgress = false;
  softDeadline = soft_deadline_ticks;
  hardDeadline = hard_deadline_ticks;
}

/******************************************************************
Function: tick (State)
Parameters: None
Returns:
	 0: If no deadlines are crossed
	 1: If soft deadline is crossed but hard deadline is not crossed.
	-1: If hard deadline is crossed.
	
Remarks: 
	Ticks the state. Internal Function. DO NOT EXPLICITLY CALL.

Warning: (issued if <Log.h> is defined)
	None.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
int8_t State::tick(){
  if (inProgress == true){
    exec_time++;
    
    if       (exec_time > hardDeadline) {return -1;}    // First check hard deadline as it 
    else if  (exec_time > softDeadline) {return 1;}     // will be larger than soft deadline
  }
  
  return 0;
}



//====================================================================================
// SM class Implementation

/******************************************************************
Function: SM (constructor)
Parameters: 
	1. gnv: type transitionFcn:: State* (*gnv)(State*)
		The function should input a state(presumed current state)
		and return a state(presumed next state). The inputs
		to SM are assumed to be either declared as static in 
		gnv implementation by user OR declared globally. 
	2. interval: Number of time steps (ticks) of TickTimer after 
		which the SM should check for transitions.

Remarks: 
	Constructs the SM class object. 
	Careful selection of interval needs to be done for stable 
	functioning of SM class. More discussion is done in tutorial.
	(Refer to webpage. (??link??)

Warning: (issued if <Log.h> is defined)
	None.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
SM::SM(transitionFcn gnv, unsigned long interval){
  getNextValues = gnv;
  tickTime = interval;
}

/******************************************************************
Function: registerToTimer (SM)
Parameters: None
	
Remarks: 
	Registers the SM::tick to TickTimer as callback. Hence, tick of
	this SM object will be executed every time TickTimer ticks.

Warning: (issued if <Log.h> is defined)
	W003: If mySM is not NULL, the primary SM for program is 
		overwritten and the warning is issued to indicate this.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
void SM::registerToTimer(){
	if (mySM != NULL) {
		#ifdef LOG_H
                  warn(W003);
                #endif
	}
	mySM = this;
}

/******************************************************************
Function: addState (SM)
Parameters: 
	1. s: State object to be added to list of children.
	
Remarks: 
	Adds new state object to SM.

Warning: (issued if <Log.h> is defined)
	W004: If the maximum number of children possible (MAX_CHILD_STATE)
		are already registered. In this case, the new state addition is
		ignored.

Error: (issued if <Log.h> is defined)
	None.

******************************************************************/
void SM::addState(State* s){
  if (_childState_head >= MAX_CHILD_STATE){
    #ifdef LOG_H
        warn(W004);
    #endif
  }
  else{
    childStates[_childState_head] = s;
    _childState_head++;
  }
}

/******************************************************************
Function: tick (SM)
Parameters: None
	
Remarks: 
	Internal Function... DO NOT EXPLICITLY CALL.
	Ticks the SM. Generally, it is registered as callback for TickTimer.
	and is executed every time TickTimer ticks.
	
Algorithm:
	1.	If it is time to check for SM transition?
	2.		If current state has done its job?
	3.			If not, tick it (current state). 
	4. 				Raise any warning/errors.
	5.			If yes, check enable the transition and get out.

Warning: (issued if <Log.h> is defined)
	W005: If state hits the soft-deadline.

Error: (issued if <Log.h> is defined)
	E001: If state hits hard-deadline.

******************************************************************/
void SM::tick(){

  tickCount++;
  if (tickCount >= tickTime){
    tickCount = 0;                          
    
    if (currState != NULL){
      if ((*currState).inProgress){
        
        int8_t retVal = (*currState).tick();
        
        // Validations on retVal...
        if (retVal == 1){
          #ifdef LOG_H
            warn(W005);
          #endif
        }
        else if (retVal == -1){
          #ifdef LOG_H
            error(E001);
          #endif
        }
        
        return; // 0;
      }
      else{
        isTrnActive = true;
        return; // 0 ;
      }
    }
    return; // -1;
  }
  return; // 0;
}

/******************************************************************
Function: reset (SM)
Parameters: None
	
Remarks: 
	Resets each of children states.
	
Warning: (issued if <Log.h> is defined)
	None

Error: (issued if <Log.h> is defined)
	None

******************************************************************/
void SM::reset(){
  for (int i = 0; i < _childState_head; i++){
    (*childStates[i]).reset();
  }
}

/******************************************************************
Function: step (SM)
Parameters: None
	
Remarks: 
	Executes the state and computes the next state if transition is 
	enabled.
	
Warning: (issued if <Log.h> is defined)
	None

Error: (issued if <Log.h> is defined)
	None

******************************************************************/
void SM::step(){
  if (isTrnActive == true){
    
    //is resetting here required?
    currState->inProgress = true;
    currState->update();
    currState->inProgress = false;
    
    currState = getNextValues(currState);    
    isTrnActive = false;
  }
}


