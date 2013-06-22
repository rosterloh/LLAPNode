//
// LLAPNode by Richard Osterloh - June 2013
//
// sRF module on RFu328
//
#include <ChibiOS_AVR.h>
#include <LLAPSerial.h>
#include "LLAPNode.h"

// Declare a semaphore with an inital counter value of zero.
SEMAPHORE_DECL(sem, 0);
//------------------------------------------------------------------------------
// Thread 1, process messages when signalled by thread 2.

// 64 byte stack beyond task switch and interrupt needs 
static WORKING_AREA(waThread1, 64);
 
static msg_t Thread1 (void *arg) {
  while (!chThdShouldTerminate()) {
    // Wait for signal from thread 2.
    chSemWait(&sem);

    String msg = LLAP.sMessage;
    String reply = msg.substring(0,4);
    NodeErr err = NONE;
    byte typeOfIO = msg.charAt(0);
    // TODO: Check if not A or D and set INVALID_TYPE
    byte ioNumber = (msg.charAt(1) - '0') * 10 + msg.charAt(2) - '0';
    // TODO: Check num > 1 and not serial or srf control lines and set INVALID_PIN
    msg = msg.substring(3);
    if(msg == "INPUT") 
      pinMode(ioNumber, INPUT);
    else if(msg == "OUTPUT")
      pinMode(ioNumber, OUTPUT);
    else if(msg == "HIGH")
      digitalWrite(ioNumber, HIGH);
    else if(msg == "LOW")
      digitalWrite(ioNumber, LOW);
    else if(msg == "READ") {
      if (typeOfIO == 'A') {
        int val = analogRead(ioNumber);
        LLAP.sendMessage(reply + "+" + val); // not sure about this
      } else {
        if (digitalRead(ioNumber)) {
          LLAP.sendMessage(reply + "HIGH");
        } else {
          LLAP.sendMessage(reply + "LOW");
        }
      }
    } else if( msg.startsWith("PWM")) {
      byte val = ((msg.charAt(3) - '0') * 10 + msg.charAt(4) - '0') * 10 + msg.charAt(5) - '0';
      analogWrite(ioNumber,val);
    } else {
      err = UNKNOWN_CMD;
    }
    if (err)
      LLAP.sendMessage("ERROR"+err);
    else
      LLAP.sendMessage("ACK");
  }
  return 0;
}

//------------------------------------------------------------------------------
// Thread 2, receive messages and signal thread 1 to process them.

// 64 byte stack beyond task switch and interrupt needs
static WORKING_AREA(waThread2, 64);

static msg_t Thread2(void *arg) {
  LLAP.init(DEVICEID); // TODO: Get ID from non volatile storage
  // Signal that main thread has started
  LLAP.sendMessage("STARTED");
  while (1) {
    if (LLAP.bMsgReceived) {
      // Signal thread 1 to process message.
      chSemSignal(&sem);
      
      LLAP.bMsgReceived = false;	// if we do not clear the message flag then message processing will be blocked
    }
    chThdSleepMilliseconds(100);  // TODO: Try out sleepForaWhile() from LLAP for low power
  }
  return 0;
}
//------------------------------------------------------------------------------
void setup () {
  Serial.begin(115200);
  pinMode(8, OUTPUT);    // initialize pin 8 to control the radio
  digitalWrite(8, HIGH); // select the radio
  delay(1000);
    
  chBegin(mainThread);
  // chBegin never returns, main thread continues with mainThread()
  while(1) {}
}
//------------------------------------------------------------------------------
// main thread runs at NORMALPRIO 
void mainThread () {
  // start the threads
  chThdCreateStatic(waThread1, sizeof(waThread1),
    NORMALPRIO + 2, Thread1, NULL);

  //chThdCreateStatic(waThread2, sizeof(waThread2),
  //  NORMALPRIO + 1, Thread2, NULL);
  
  LLAP.init(DEVICEID); // TODO: Get ID from non volatile storage
  // Signal that main thread has started
  LLAP.sendMessage("STARTED");
      
  while (1) {
    //  loop();
    // must insure increment is atomic in case of context switch for print
    // should use mutex for longer critical sections
    //noInterrupts();
    //count++;
    //interrupts();
    if (LLAP.bMsgReceived) {
      // Signal thread 1 to process message.
      chSemSignal(&sem);
      
      LLAP.bMsgReceived = false;	// if we do not clear the message flag then message processing will be blocked
    }
  }
}
//------------------------------------------------------------------------------
void loop () {
  // not used
}




