/*  __________           .___      .__  .__                   ___ ________________    ___
 *  \______   \ ____   __| _/____  |  | |__| ____   ____     /  / \__    ___/     \   \  \   
 *   |     ___// __ \ / __ |\__  \ |  | |  |/    \ /  _ \   /  /    |    | /  \ /  \   \  \  
 *   |    |   \  ___// /_/ | / __ \|  |_|  |   |  (  <_> ) (  (     |    |/    Y    \   )  )
 *   |____|    \___  >____ |(____  /____/__|___|  /\____/   \  \    |____|\____|__  /  /  /
 *                 \/     \/     \/             \/           \__\                 \/  /__/
 *                                                                (c) 2018 alf45star
 *                                                        https://github.com/alf45tar/Pedalino
 */

#include "Pedalino.h"
#include "Serialize.h"
#include "Controller.h"
#include "BlynkRPC.h"
#include "Config.h"
#include "MIDIRouting.h"
#include "Display.h"
#include "Menu.h"

void serial_pass_run()
{
  static bool startSerialPassthrough = true;
    
  if (startSerialPassthrough) {
#ifndef NOLCD
    lcd.clear();
    lcd.print(" USB to Serial ");
    lcd.setCursor(0, 1);
    lcd.print(" Reset to stop ");
#endif
    startSerialPassthrough = false;
  }

  if (Serial.available()) {         // If anything comes in Serial (USB),
    Serial3.write(Serial.read());   // read it and send it out Serial3
  }

  if (Serial3.available()) {        // If anything comes in Serial3
    Serial.write(Serial3.read());   // read it and send it out Serial (USB)
  }
}

// Standard setup() and loop()

void setup(void)
{
#ifdef DEBUG_PEDALINO
  SERIALDEBUG.begin(115200);

  DPRINTLNF("");
  DPRINTLNF("  __________           .___      .__  .__                   ___ ________________    ___");
  DPRINTLNF("  \\______   \\ ____   __| _/____  |  | |__| ____   ____     /  / \\__    ___/     \\   \\  \\");
  DPRINTLNF("   |     ___// __ \\ / __ |\\__  \\ |  | |  |/    \\ /  _ \\   /  /    |    | /  \\ /  \\   \\  \\");
  DPRINTLNF("   |    |   \\  ___// /_/ | / __ \\|  |_|  |   |  (  <_> ) (  (     |    |/    Y    \\   )  )");
  DPRINTLNF("   |____|    \\___  >____ |(____  /____/__|___|  /\\____/   \\  \\    |____|\\____|__  /  /  /");
  DPRINTLNF("                 \\/     \\/     \\/             \\/           \\__\\                 \\/  /__/");
  DPRINTLNF("                                                                (c) 2018 alf45star");
  DPRINTLNF("                                                        https://github.com/alf45tar/Pedalino");
  DPRINTLNF("");

#endif

  // Reset to factory default if RIGHT key is pressed and hold for alt least 8 seconds at power on
  // Enter serial passthrough mode if RIGHT key is pressed adn hold for less than 8 seconds at power on
  pinMode(A0, INPUT_PULLUP);
  unsigned long milliStart = millis();
  unsigned long duration = 0;
#ifndef NOLCD
    lcd.clear();
#endif 
  while ((digitalRead(A0) == LOW) && (duration < 8500)) {
    DPRINT("#");
#ifndef NOLCD
    lcd.setCursor(duration / 500, 0);
    lcd.print("#");
#endif
    delay(100);
    duration = millis() - milliStart;
  }
  DPRINTLN("");
  if ((digitalRead(A0) == HIGH) && (duration > 100 && duration < 8500)) {
    DPRINTLN("Serial passthrough mode for ESP firmware update and monitor");
    serialPassthrough = true;
  } else if ((digitalRead(A0) == LOW) && (duration >= 8500)) {
    DPRINTLN("Reset EEPROM to factory default");
#ifndef NOLCD
    lcd.setCursor(0, 1);
    lcd.print("Factory default ");
    delay(1000);
#endif
    load_factory_default();
    update_eeprom();
  }

  read_eeprom();

  // Initiate serial MIDI communications, listen to all channels and turn Thru off
#ifndef DEBUG_PEDALINO
  USB_MIDI.begin(MIDI_CHANNEL_OMNI);
  interfaces[PED_USBMIDI].midiThru ? USB_MIDI.turnThruOn() : USB_MIDI.turnThruOff();
#endif
  DIN_MIDI.begin(MIDI_CHANNEL_OMNI);
  interfaces[PED_DINMIDI].midiThru ? DIN_MIDI.turnThruOn() : DIN_MIDI.turnThruOff();
  ESP_MIDI.begin(MIDI_CHANNEL_OMNI);
  ESP_MIDI.turnThruOff();

  autosensing_setup();
  controller_setup();
  mtc_setup();
  midi_routing_start();
  blynk_config();
  menu_setup();
}

void loop(void)
{
  if (serialPassthrough) {
    serial_pass_run();
  }
  else {
    // Display menu on request
    menu_run();

    // Process Blynk messages
    blynk_run();

    // Check whether the input has changed since last time, if so, send the new value over MIDI
    midi_refresh();
    midi_routing();
  }
}