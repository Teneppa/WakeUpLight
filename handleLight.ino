/*
 This thingy just handles fading the light
*/

uint16_t brightness = 0;
uint16_t maxbrightness = 1023;
long oldUpdate = 0;

// Change the state of the light in a gentle manner
void setLight(bool state, bool warm, uint16_t updateRate) {

  // IF MOVEMENT DETECTED, SPEED UP THE UPDATING
  if (digitalRead(PIR)) {
    updateRate = int(updateRate/32);
  }

  // IF MOVEMENT DETECTED AT NIGHT
  if (digitalRead(PIR) && (state == 0)) {
    maxbrightness = 10;
    state = 1;
    updateRate = 10;
  }else{
    maxbrightness = 1023;
  }

  if (millis() - oldUpdate > updateRate) {

    if (state) {
      if (brightness < maxbrightness) {
        brightness++;
      }
    } else {
      if (brightness > 0) {
        brightness--;
      }
    }
    
    analogWrite(LED, brightness);

    oldUpdate = millis();
  }

}

void runLight() {
  setLight(light_state, 0, light_updaterate);
}
