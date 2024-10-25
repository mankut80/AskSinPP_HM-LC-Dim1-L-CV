//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-10-07 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2019-01-10 scuba82 Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// ci-test=yes board=328p aes=no
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
//#define USE_OTA_BOOTLOADER


#define NDEBUG

#define EI_NOTEXTERNAL
#define NORTC

// PIN definitions
#define DIMMERPIN          7  
#define LED_PIN            4
#define CONFIG_BUTTON_PIN  8
#define GDO0_PIN           2
#define ZEROPIN            3
#define NTC_SENSE_PIN      A0
#define NTC_ACTIVATOR_PIN   0  // pin to power ntc (or 0 if connected to vcc)

// NTC definitions
#define NTC_T0 25 // temperature where ntc has resistor value of R0
#define NTC_R0 10000 // resistance both of ntc and known resistor
#define NTC_B 3977 // b value of ntc (see datasheet)
#define NTC_OVERSAMPLING 2 // number of additional bits by oversampling (should be between 0 and 6, highly increases number of measurements)

// Phase Cut mode 
#define PHASECUT_MODE 1 // 0 = trailing-edge phase cut; 1 = leading-edge phase cut

// number of available peers per channel
#define PEERS_PER_DimChannel     4
#define PEERS_PER_RemoteChannel  8

#include <Wire.h>
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <Dimmer.h>
#include <sensors/Ntc.h>


// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x10,0x12,0x22},       // Device ID
    "mankutHM02",           // Device Serial
    {0x00,0x12},            // Device Model
    0x10,                   // Firmware Version
    as::DeviceType::Dimmer, // Device Type
    {0x01,0x00}             // Info Bytes
};
/**
   Configure the used hardware
*/
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType, GDO0_PIN> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, NoBattery, RadioType> Hal;
Hal hal;
typedef DimmerChannel<Hal, PEERS_PER_DimChannel> DimChannel;
typedef DimmerDevice<Hal, DimChannel, 3, 3> DimDevice;

DimDevice sdev(devinfo, 0x20);
DimmerControl<Hal,DimDevice,ZC_Control<>> control(sdev);
ConfigToggleButton<DimDevice> cfgBtn(sdev);

class TempSens : public Alarm {
  Ntc<NTC_SENSE_PIN,NTC_R0,NTC_B,NTC_ACTIVATOR_PIN,NTC_T0,NTC_OVERSAMPLING> ntc;
  
  public:
  TempSens () : Alarm(0) {}
  virtual ~TempSens () {}

  void init () {
    ntc.init();
    set(seconds2ticks(15));
    sysclock.add(*this);
  }

  virtual void trigger (AlarmClock& clock) {
    ntc.measure();
    DPRINT("Temp: ");DDECLN(ntc.temperature());
    control.setTemperature(ntc.temperature());
    set(seconds2ticks(2));
    clock.add(*this);
  }
};
TempSens tempsensor;


void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  if( control.init(hal,DIMMERPIN) ){
    sdev.channel(1).peer(cfgBtn.peer());
  }
 
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  tempsensor.init();
  sdev.initDone();
  phaseCut.Stop();
  
}


void loop() {
  hal.runready();
  sdev.pollRadio();

  if(!phaseCut.isrunning()) digitalWrite(DIMMERPIN, false);
}
