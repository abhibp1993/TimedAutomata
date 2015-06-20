/************************************************************************************************************
* Library: TimedAutomata																					*
* Author: Abhishek N. Kulkarni	(abhibp1993)																*
* Description:																								*
*	Refer to TimedAutomata.cpp																				*
* 	Refer to my website for tutorials (?link?)																*
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

#ifndef TIMEDAUTOMATA1_H
#define TIMEDAUTOMATA1_H

        // Uncomment following block if Logs are to be enabled.
        
//        #include "Log.h"
//        
//        #define W001    1    // unacceptable tickTime_us
//        #define W002    2    // more than MAX_CALLBACK callbacks
//        #define W003    3    // overwritten mySM
//        #define W004    4    // addition after MAX_CHILD_STATE
//        #define W005    5    // soft-deadline
//        
//        #define E001    1    // hard deadline

        

//==========================================================================================================
// DO NOT EDIT BELOW THIS.


	#include "Arduino.h"
	#include <avr/interrupt.h>
        
	#define  MAX_CALLBACK  10        // Maximum callbacks permitted (keep it small for smaller tick-times)
	#define MAX_CHILD_STATE 5		 // Maximum child states per SM
	
	
	#define  TICK_50US     50      
	#define  TICK_100US    100
	#define  TICK_200US    200
	#define  TICK_500US    500
	#define  TICK_1000US   1000
	#define  TICK_2000US   2000
	#define  TICK_4000US   4000
	#define  TICK_1MS	   1000
	#define  TICK_2MS	   2000
	#define  TICK_4MS	   4000

	
	
	
	class State;
	class SM;
  
	typedef void (*updateFcn)();
	typedef void (*callback)();      	// Type definition for no-input, no-output function pointers
	typedef State* (*transitionFcn)(State* cState);
	
	
	

	// namespace with timer2 functionality wrappers --> "dot" operator don't work!!
	namespace TickTimer{

		extern unsigned long tickTime;                                // time between two ticks in microseconds
		extern volatile unsigned char _tcnt_reload;                   // internal variable for timer2 configuration (reload count for tcnt)
		extern volatile unsigned char _callback_array_head;           // pointer to current position of array of callbacks (internal)


		void configure(unsigned long tickTime_us);                    // Configuration function for tickTime
		void registerCallback(callback fcn);                          // Register a new callback function
		void startTicking();                                          // Starts the operation of timer
		void stopTicking();                                           // Stops the operation of timer

	}
	
	class State{

		public:
			unsigned long softDeadline;            // Triggers warning
			unsigned long hardDeadline;            // Triggers error
			updateFcn myFcn;                       // State Update function pointer
			volatile boolean inProgress = false;   // true, if state is in update mode
			volatile unsigned long exec_time;      // Maintains the execution time
		  
		public:
			State(updateFcn fcn);
			State(updateFcn fcn, unsigned long hard_deadline_ticks);
			State(updateFcn fcn, unsigned long hard_deadline_ticks, unsigned long soft_deadline_ticks); 

			int8_t tick();                         // 0 = OK, -1: hard_deadline crossed, 1: soft_deadline crossed

			inline void update() {if (inProgress) {myFcn();}}        // Executes the update function of state
			inline void reset()  {exec_time = 0;}                    // Resets the run time of state
	};

	class SM{

		public:
			State* childStates[MAX_CHILD_STATE];				// list of child states
			uint8_t _childState_head;							// current number of child states

			unsigned long tickTime, tickCount;					// tickTime: Time after which the machine should check for transition
																// tickCount: Elapsed counts
			transitionFcn getNextValues;						// transition function (user should define)

			State* currState;									// Current state of machine
			boolean isTrnActive;								// state variable: denotes whether the transition is enabled or not.

		public:
			SM(transitionFcn gnv, unsigned long interval);			// Constructor. interval = tickInterval 
																	// (MUST BE CONFIGURED AFTER TickTimer::configure)
			void registerToTimer();									// Configure this machine as main machine. Registers callback to this SM.
			
			inline void setStartState(State* s) {currState = s;}	// Set start state of machine.
			void addState(State* s);								// Adds new state to SM.
			void tick();											// Tick any running state and evaluates for error/warning
			void step();											// Implements the transition if enabled.
			void reset();											// Resets each and every constituent states.
	};
	
#endif