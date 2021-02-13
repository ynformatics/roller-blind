#include <SparkFun_TB6612.h>
extern PubSubClient client;
extern char sendBuffer[256];
extern void DBG( char* fmt, ...);
class Blind
{
    bool atHome = false;
    bool movingForwards = true;

    enum stateEnum {Idle, Stepping, SteppingPast, Overshooting} ;
    char *enums[4] =
    {
        "Idle",
        "Stepping",
        "SteppingPast",
        "Overshooting"
    };
    
    stateEnum state = Idle;

    void setState(stateEnum newState)
    {
      DBG("state %s", enums[newState]);
      state = newState;
    }
   public:
   
   Blind(int in1, int in2, int pwm, int stby, int hall )
   {
      setState(Idle);
      motor = new Motor( in1, in2, pwm, 1, stby);
      pinHall = hall;

     pinMode(hall, INPUT_PULLUP);
     atHome = digitalRead(hall) == LOW;
  
     //attachInterrupt(digitalPinToInterrupt(hall), ISR, CHANGE); 
   }
 
    int pinHall;
    Motor* motor;
    volatile bool change;
    volatile bool falling;
    volatile unsigned long last_interrupt_time;
    unsigned long last_revolution_time;
    unsigned long revolution_time;
    bool pulsePending = false;
    int steps;
    unsigned long extraTime = 0;

   
    void loop()
   {
     if(change)
    {
      change = false;
      
      if(falling)
      {
         pulsePending = true;       
      }
      else // rising
      {
         pulsePending = false;  
         if(atHome)
         {  
           atHome = false; 
 //          DBG("away %d", micros()/1000);
 //          Serial.print( "away "); Serial.println(micros()/1000);      
         }
      }
    }
    
    if(pulsePending && (micros() - last_interrupt_time > 10000))
    {
      pulsePending = false;
      if(!atHome)
      {
        atHome = true; 
 //       DBG("home %d", micros()/1000);
 //       Serial.print( "home "); Serial.println(micros()/1000);  
      }
    }

     // keep moving until we have left the home region
   
    if(state == SteppingPast && !atHome)
    {
      DBG("left home %d", micros()/1000);
      if(steps > 0)
         setState(Stepping);     
      else
         setState(Overshooting);
    }

    // keep moving until we reach the next home region
    if(state == Stepping && atHome)
    { 
      unsigned long now = millis();
      revolution_time = now - last_revolution_time;
      last_revolution_time = now;
      DBG("revtime %d steps %d", revolution_time, steps);      
    
      if(steps == 0)           // we have arrived
      {             
         setState(Overshooting);            
      }
      else
      {
         steps--;
        // keep going
        setState(SteppingPast);
      }
    }

    if(state == Overshooting)
    {
      if(millis() - last_revolution_time >= extraTime)
      {
          motor->brake();       // stop motor
          setState(Idle); // and cancel the stepping
      }
    }
   }

   void move(float revs)
   {
       movingForwards = revs >= 0;
       revs = abs(revs);
       int wholeRevs = (int)revs;
       int centiRevs = (int)((revs - wholeRevs) * 100);
      
       steps = wholeRevs;
       extraTime = std::min(1920, 1920 * centiRevs / 100);
       DBG("whole %d centi %d extra %d", wholeRevs, centiRevs, extraTime);
      
       setState(SteppingPast);
       motor->drive(movingForwards ? 1023 : -1023);
   }
   
  void ISR()
  { 
    change = true;
    falling = digitalRead(pinHall) == LOW;
    last_interrupt_time = micros();
  }

};


