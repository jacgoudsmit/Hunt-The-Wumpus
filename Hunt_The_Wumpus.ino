//---------------------------------------------------------------------------
//
// Hunt the Wumpus
//
// Original Arduino sketch by Dan Malec 
// (https://github.com/dmalec/Hunt-The-Wumpus)
// Modified to generate "special effects" (gestures) on a RoboBrrd
// by Jac Goudsmit
// (https://github.com/jacgoudsmit/Hunt-The-Wumpus/tree/RoboBrrd)
//
// RoboBrrd was designed and created by Erin Kennedy ("RobotGrrl").
//
//---------------------------------------------------------------------------
//
// An Arduino sketch that implements the classic game Hunt the Wumpus
//
// http://en.wikipedia.org/wiki/Wumpus
//
// In this variant, there are twenty rooms layed out like a d20; therefore,
// each room is connected to three other rooms.  Two rooms are bottomless 
// pits; if you fall in to a bottomless pit you lose.
//
// Two rooms contain giant bats; if you enter a room with a giant bat it will 
// pick you up and carry you to a random room.  One room contains a wumpus;
// if you bump into the Wumpus, it will eat you and you lose.  You have 2
// arrows; if you shoot the Wumpus, you win.  If you run out of arrows, you
// lose.
//
// The RoboBrrd version uses the RoboBrrdLib library to generate gestures
// and sound effects that get triggered when important events happens in the
// game. Also, the colors of the RoboBrrd eyes match the background colors
// of the LCD screen throughout the game.
//
//---------------------------------------------------------------------------
//
// MIT license.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//
//      ******************************************************
//      Designed for the Adafruit RGB LCD Shield Kit
//      http://www.adafruit.com/products/716
//      or
//      Adafruit Negative RGB LCD Shield Kit
//      http://www.adafruit.com/products/714
//
//      For more information about RoboBrrd, visit:
//      http://www.robobrrd.com
//      ******************************************************
//
//
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// FEATURES
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//! Enable RoboBrrd special effects
//
// Uncomment the following to enable effects on the RoboBrrd. The LCD display
// should be connected to the usual A4 and A5 pins used for I2C (Wire.h),
// and the RoboBrrd hardware is also assumed to be connected in the normal
// way. If necessary, alternative pin assignments can be used by modifying
// the initialization of the robobrrd library, and/or the EEPROM (see
// below).
// If this is enabled, the program will still run fine even if you don't have
// the RoboBrrd hardware attached. Also, the effects can be disabled at
// runtime so you can play the plain no-effects version even if you do have
// the RoboBrrd hardware attached.
//#define WUMPUS_ROBOBRRD


//---------------------------------------------------------------------------
//! Enable EEPROM storage/retrieval when using Robobrrd hardware
//
// Uncomment the following if you want to use the EEPROM to store RoboBrrd
// pin assignments and servo calibration values.
// This makes it possible to calibrate the servos and store the calibration
// values into the EEPROM. Basically it makes it very easy to run the same
// sketch on multiple RoboBrrds even if they have different calibration
// values, or even if the wiring is different.
//#define WUMPUS_ROBOBRRD_EEPROM


//---------------------------------------------------------------------------
//! Enable debug output
//
// Uncomment the following to generate some debug output. Normally this
// should be off. This is an unfinished feature.
//#define DEBUG


//---------------------------------------------------------------------------
//! Enable fix for mis-wired LCD backlight LED's
//
// Uncomment this if you reversed the Red and Blue lines of the LCD 
// backlight.
//#define OHPOOPREVERSEDLCDBACKLIGHT


/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//! RoboBrrd and EEPROM manager include files
//
// See:
// EEPROM_mgr.h:  https://github.com/jacgoudsmit/EEPROM_mgr
// RobobrrdLib.h: https://github.com/jacgoudsmit/RobobrrdLib
// TinyGPS.h:     http://arduiniana.org/libraries/tinygps/
// Time.h:        http://www.pjrc.com/teensy/td_libs_Time.html
//
#ifdef WUMPUS_ROBOBRRD
#ifdef WUMPUS_ROBOBRRD_EEPROM
#include <EEPROM_mgr.h>
#endif // WUMPUS_ROBOBRRD_EEPROM

#include <RobobrrdLib.h>
#include <Servo.h>

#ifdef ROBOBRRD_HAS_GPS
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Time.h>
#endif // ROBOBRRD_HAS_GPS

#ifndef ROBOBRRD_HAS_LCD
#error Sorry, you need an LCD
#endif // ROBOBRRD_HAS_LCD

#endif // WUMPUS_ROBOBRRD


//---------------------------------------------------------------------------
//! RGB LCD Shield include files
//
// Adafruit_MCP23017.h:
// https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library
// Adafruit_RGBLCDShield.h
// https://github.com/adafruit/Adafruit-RGB-LCD-Shield-Library
//
#include <Wire.h>
#include <Adafruit_MCP23017.h> 
#include <Adafruit_RGBLCDShield.h>


//---------------------------------------------------------------------------
// Other includes
#include <Streaming.h>


/////////////////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//! Macro to implement conditional RoboBrrd effects
#ifdef WUMPUS_ROBOBRRD
#define ROBOSFX(x) do { if (robobrrd_sfx) x; } while(0)
#else
#define ROBOSFX(x)
#endif


//---------------------------------------------------------------------------
//! Debug macros
#ifdef DEBUG
#define PRINT(x) do { Serial.print(x); delay(1000); } while(0)
#define PRINTLN(x) do { Serial.println(x); delay(1000); } while(0)
#else
#define PRINT(x)
#define PRINTLN(x)
#endif


//---------------------------------------------------------------------------
//! Macro to prevent generating automatic prototype for function
//
// This macro is used to trick the brain-dead Arduino IDE into NOT generating
// an automatic prototype. Whenever a function uses a custom type and the
// the automatic prototype generator causes a compiler error, surround the
// function name and parameters with this macro to trick the Arduino compiler
// into not generating an automatic prototype.
#define NoAutoProto(x) x


/////////////////////////////////////////////////////////////////////////////
// TYPES
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//! Function type for state function callbacks
typedef void StateFunc_t(void);


//---------------------------------------------------------------------------
//! Indexes into the bitmap array for the icons on the LCD panel
enum IconIndex
{
  WUMPUS_ICON_IDX = 0,
  BAT_ICON_IDX = 1,
  PIT_ICON_IDX = 2,
  ARROW_ICON_IDX = 3
};


//---------------------------------------------------------------------------
//! Hazard types
enum HazardType 
{
  NONE   = 0,
  BAT    = 1,
  PIT    = 2, 
  WUMPUS = 4 
};


//---------------------------------------------------------------------------
//! Enum of backlight colors.
enum BackLightColor 
{
  RED    = 0x1,
  GREEN  = 0x2,
  YELLOW = 0x3,
  BLUE   = 0x4,
  VIOLET = 0x5,
  TEAL   = 0x6,
  WHITE  = 0x7
};


/////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//! Map of the cavern layout.
//
// The first dimension represents the cave number and the second dimension 
// represents the connected caves.  NOTE - C indexing starts at 0, but cave
// number display starts at 1 - so the first line below means cave 1 is 
// connected to caves 3, 9, and 15.
const uint8_t cave[20][3] =
{
  { 2,  8, 14},    //  0
  { 7, 13, 19},    //  1
  {12, 18,  0},    //  2
  {16, 17, 19},    //  3
  {11, 14, 18},    //  4
  {13, 15, 18},    //  5
  { 9, 14, 16},    //  6
  { 1, 15, 17},    //  7
  { 0, 10, 16},    //  8
  { 6, 11, 19},    //  9
  { 8, 12, 17},    // 10
  { 4,  9, 13},    // 11
  { 2, 10, 15},    // 12
  { 1,  5, 11},    // 13
  { 0,  4,  6},    // 14
  { 5,  7, 12},    // 15
  { 3,  6,  8},    // 16
  { 3,  7, 10},    // 17
  { 2,  4,  5},    // 18
  { 1,  3,  9}     // 19
};


//---------------------------------------------------------------------------
//! Array of columns bracketing menu options.
//
// The first dimension is the menu item index, the second column is the
// tuple of columns which bracket the menu item text.
const uint8_t menu_col[4][2] =
{
  {0,  3},
  {3,  6},
  {6,  9},
  {9, 15} 
};


//---------------------------------------------------------------------------
//! Array of custom bitmap icons.
//
// Custom icons created using: http://www.quinapalus.com/hd44780udg.html
const byte icons[4][8] = 
{
  { 0x0e, 0x15, 0x1f, 0x11, 0x0e, 0x11, 0x11 },
  { 0x0a, 0x1f, 0x15, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x11, 0x0a, 0x0a, 0x0a, 0x0a },
  { 0x00, 0x04, 0x02, 0x1f, 0x02, 0x04, 0x00 } 
};


//---------------------------------------------------------------------------
//! Settings
#ifdef WUMPUS_ROBOBRRD
#ifdef WUMPUS_ROBOBRRD_EEPROM

// RoboBrrd pins and values
EEPROM_item<RoboBrrd::Pins>   pins(RoboBrrd::DefaultPins);
EEPROM_item<RoboBrrd::Values> values(RoboBrrd::DefaultValues);

// Name of the RoboBrrd
EEPROM_item<char[17]>         myname;

// Enable 12 hour time display
EEPROM_item<boolean>          enable12h(true);

// Screensaver parameters
#ifdef ROBOBRRD_HAS_GPS
EEPROM_item<int>              screensaver_brightness_threshold(100);
EEPROM_item<unsigned long>    screensaver_clock_timeout(15000);
#endif

#else

#define myname F("RoboBrrd")
#define enable12h (true)

#ifdef ROBOBRRD_HAS_GPS
#define screensaver_brightness_threshold (100)
#define screensaver_clock_timeout (15000)
#endif

#endif
#endif


//---------------------------------------------------------------------------
//! Robobrrd instance
#ifdef WUMPUS_ROBOBRRD
#ifdef WUMPUS_ROBOBRRD_EEPROM
RoboBrrd robobrrd(values, pins);
#else
RoboBrrd robobrrd;
#endif
#endif


//---------------------------------------------------------------------------
//! Boolean variable to enable/disable Robobrrd special effects at runtime
#ifdef WUMPUS_ROBOBRRD
boolean robobrrd_sfx = true;
#endif


//---------------------------------------------------------------------------
//! Variable indicating servos are attached
#ifdef WUMPUS_ROBOBRRD
boolean ServosAttached = false;
#endif


//---------------------------------------------------------------------------
//! Variable to keep track of servo positions
#ifdef WUMPUS_ROBOBRRD
RoboBrrd::Pos lastpos[RoboBrrd::NumServos];
#endif


//---------------------------------------------------------------------------
//! Index in the map of the room the player is in.
uint8_t player_room;


//---------------------------------------------------------------------------
//! Hazards in each room
uint8_t hazards[20];


//---------------------------------------------------------------------------
//! Count of how many arrows the player has left.
uint8_t arrow_count;


//---------------------------------------------------------------------------
//! Index in the map of the room the arrow is shot into.
//
// This index is only valid/current during the state changes
// associated with firing an arrow into a room.
uint8_t arrow_room;


//---------------------------------------------------------------------------
//! The current state.
StateFunc_t *state;


//---------------------------------------------------------------------------
//! The time in milliseconds since the last state change.
unsigned long last_state_change_time;


//---------------------------------------------------------------------------
//! The currently selected menu index.
uint8_t selected_menu_idx;


//---------------------------------------------------------------------------
//! Current time as recorded at beginning of loop
unsigned long time;


//---------------------------------------------------------------------------
//! The LCD display object.
#ifdef WUMPUS_ROBOBRRD
Adafruit_RGBLCDShield &lcd = robobrrd.m_lcd;
#else
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
#endif


//---------------------------------------------------------------------------
//! The bitmask of currently clicked buttons.
uint8_t clicked_buttons;


//---------------------------------------------------------------------------
//! Screen saver data
unsigned long       screensaver_timestamp;
byte                screensaver_light;
bool                screensaver_on;


/////////////////////////////////////////////////////////////////////////////
// ROBOBRRD SPECIAL EFFECTS
/////////////////////////////////////////////////////////////////////////////


#ifdef WUMPUS_ROBOBRRD

//---------------------------------------------------------------------------
//! Attach servos if they're not already attached
void attachservos(void)
{
  if (!ServosAttached)
  {
    robobrrd.AttachServos();
    ServosAttached = true;
  }
}


//---------------------------------------------------------------------------
//! Detach servos
void detachservos(void)
{
  if (ServosAttached)
  {
    delay(300);
    robobrrd.DetachServos();
    ServosAttached = false;
  }
}


//---------------------------------------------------------------------------
//! Move servos
//
// Attaches the servos, moves them and optionally detaches them again
void moveservos(
  RoboBrrd::Servos which,
  RoboBrrd::Pos pos,
  boolean detach = true)
{
  attachservos();

  for (int i = 0; i < RoboBrrd::NumServos; i++)
  {
    if (0 != (which & (1 << i)))
    {
      robobrrd.Move((RoboBrrd::ServoSelect)i, (lastpos[i] = pos));
    }
  }

  if (detach)
  {
    detachservos();
  }
}


//---------------------------------------------------------------------------
//! Flap both wings to opposite positions
void flapwings(boolean detach = true)
{
  for (int i = 0; i < RoboBrrd::NumServos; i++)
  {
    if (i != RoboBrrd::Beak)
    {
      moveservos((RoboBrrd::Servos)(1 << i), 
        (lastpos[i] == RoboBrrd::Upper) ?
        RoboBrrd::Lower : RoboBrrd::Upper, false);
    }
  }

  if (detach)
  {
    detachservos();
  }
}


//---------------------------------------------------------------------------
//! RoboBrrd pretends to fall down a bottomless pit
//
// I tried generating a falling sound at the same time as flapping the wings
// but couldn't get it right...
// So now you hear the sound first, followed by alternately flapping the
// wings.
void sfxfall(void)
{
  // Beak open, wings up
  moveservos(RoboBrrd::ServosAll, RoboBrrd::Upper, false);
  for (int i = 240; i < 600; i += 20)
  {
    robobrrd.Tone(i, 80);
  }

  // Flap the right wing so the wings flap alternately during the loop
  moveservos(RoboBrrd::ServosRight, RoboBrrd::Lower, false);
  for (int i = 0; i < 7; i++)
  {
    flapwings(false);
    delay(200);
  }

  // Close beak and wings down. Splat! Poor Robobrrd :-)
  moveservos(RoboBrrd::ServosAll, RoboBrrd::Lower, true);
}


//---------------------------------------------------------------------------
//! Uh-oh, eaten by Wumpus
//
// Kinda inconsistent to make the Robobrrd pretend that it's eating when it's
// actually BEING eaten. Oh well.
void sfxwumpus(void)
{
  // Wings center, beak open
  moveservos(RoboBrrd::ServosWings, RoboBrrd::Middle, false);
  moveservos(RoboBrrd::ServosBeak,  RoboBrrd::Open,   false);

  // Uh-oh
  delay(200);
  robobrrd.Tone(700, 300);
  delay(150);
  robobrrd.Tone(900, 900);

  for (int i = 0; i < 8; i++)
  {
    moveservos(RoboBrrd::ServosBeak, 
      (lastpos[RoboBrrd::Beak] == RoboBrrd::Open) ? 
      RoboBrrd::Closed : RoboBrrd::Open, false);
    delay(200);
  }

  // Wings down, servos off
  moveservos(RoboBrrd::ServosAll, RoboBrrd::Lower, true);
}


//---------------------------------------------------------------------------
//! Player has won the game. Yay!
void sfxcheers(void)
{
  moveservos(RoboBrrd::ServosAll, RoboBrrd::Upper, true);
  for (int i = 0; i < 200; i++)
  {
    robobrrd.Tone(random(2000) + 200, 20);
  }
}

#endif // WUMPUS_ROBOBRRD


/////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//! Set the background color
//
// When using RoboBrrd, set the color of the eyes too 
void setLight(byte bits) 
{
  screensaver_light = bits;

#ifdef OHPOOPREVERSEDLCDBACKLIGHT
  //                                     0  1  2  3  4  5  6  7
  static const byte reversedcolors[] = { 0, 4, 2, 6, 1, 5, 3, 7 };
#endif

  lcd.setBacklight(
#ifdef OHPOOPREVERSEDLCDBACKLIGHT
    reversedcolors[bits]
#else
    bits
#endif
    );
  ROBOSFX(robobrrd.LedBits(RoboBrrd::SidesBoth, bits));
}


//---------------------------------------------------------------------------
//! Return a bitmask of clicked buttons.
//
// Examine the bitmask of buttons which are currently pressed and compare 
// against the bitmask of which buttons were pressed last time the function
// was called.
// If a button transitions from pressed to released, set it in the bitmask.
void read_button_clicks() 
{
  static uint8_t last_buttons = 0;

  uint8_t buttons = lcd.readButtons();
  clicked_buttons = (last_buttons ^ buttons) & (~buttons);
  last_buttons = buttons;

#if defined(WUMPUS_ROBOBRRD) && defined(ROBOBRRD_HAS_GPS)
  if (clicked_buttons != 0)
  {
    reset_screensaver();
  }
#endif
}


//---------------------------------------------------------------------------
//! Print a cave number to the lcd.
//
// Print a cave number to the lcd by adding one to the index number and left
// padding with a single space if needed to make the cave number take up two
// places.
//
// \param cave_idx the index of the cave in the array
void print_cave_number(uint8_t cave_idx) 
{
  if (cave_idx < 9) 
  {
    lcd.print(' ');
  }
  lcd.print(cave_idx + 1);
}


//---------------------------------------------------------------------------
//! Clear the current menu selection indicator characters
//
// Erase the characters bracketing the current menu selection.
void clear_current_menu() 
{
  lcd.setCursor(menu_col[selected_menu_idx][0], 1);
  lcd.print(' ');
  lcd.setCursor(menu_col[selected_menu_idx][1], 1);
  lcd.print(' ');
}


//---------------------------------------------------------------------------
//! Highlight the current menu selection using indicator characters
//
// Draw characters bracketing the current menu selection.
void highlight_current_menu() 
{
  lcd.setCursor(menu_col[selected_menu_idx][0], 1);
  lcd.write(0x7E);
  lcd.setCursor(menu_col[selected_menu_idx][1], 1);
  lcd.write(0x7F);
}


//---------------------------------------------------------------------------
//! Check for left and right button clicks; update the menu index as needed.
void handle_menu() 
{
  if (clicked_buttons & (BUTTON_LEFT | BUTTON_UP)) 
  {
    selected_menu_idx = (selected_menu_idx > 0) ? selected_menu_idx - 1 : 3;
  }
  else if (clicked_buttons & (BUTTON_RIGHT | BUTTON_DOWN)) 
  {
    selected_menu_idx = (selected_menu_idx < 3) ? selected_menu_idx + 1 : 0;
  } 
}


//---------------------------------------------------------------------------
//! Put the given hazard in a random room
//
// The function generates a random room into which it puts the given hazard.
// It keeps trying until it finds a room that doesn't already have another
// hazard.
NoAutoProto(
  void init_hazard(
  HazardType hazard))
{
  for(;;) 
  {
    int index = random(20);

    if (!hazards[index])
    {
      hazards[index] = (uint8_t)hazard;
      break;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// STATE FUNCTIONS
/////////////////////////////////////////////////////////////////////////////


//===========================================================================
// Start-of-game and moving states
//===========================================================================


//---------------------------------------------------------------------------
//! Initial game state, draw the splash screen.
void begin_splash_screen() 
{
  PRINTLN("begin_splash_screen");

  lcd.clear();
  setLight(TEAL);
  lcd.print(F("HUNT THE WUMPUS"));

  state = animate_splash_screen;
}


//---------------------------------------------------------------------------
//! Animate the splash screen.
//
// Blink the text "PRESS SELECT" and wait for the user to press the select 
// button.
void animate_splash_screen() 
{
  PRINTLN("animate_splash_screen");

  static boolean blink = true;
  static unsigned long last_blink_time;

  if (time - last_blink_time >= 1000) 
  {
    lcd.setCursor(0, 1);
    if (blink) 
    {
      ROBOSFX(robobrrd.LedBits(RoboBrrd::SidesBoth, 0));
      lcd.write(0x7E);
      lcd.print(F(" PRESS SELECT "));
      lcd.write(0x7F);
    }
    else
    {
      ROBOSFX(robobrrd.LedBits(RoboBrrd::SidesBoth, TEAL));
      lcd.print(F("                "));
    }
    blink = !blink;
    last_blink_time = time;
  }

#ifdef WUMPUS_ROBOBRRD 
#ifdef ROBOBRRD_HAS_GPS
  if (time - last_state_change_time > 30000)
  {
    state = show_clock;
  }
#endif

  if (clicked_buttons & BUTTON_UP) 
  {
    robobrrd_sfx = true;
  }
  else if (clicked_buttons & BUTTON_DOWN) 
  {
    robobrrd_sfx = false;
    robobrrd.LedBits(RoboBrrd::SidesBoth, 0);
  }
#endif

  if (clicked_buttons & BUTTON_SELECT) 
  {
    state = start_game;
  }
}


//---------------------------------------------------------------------------
//! Initialize a new game.
//
// Randomize locations and reset variables.
void start_game() 
{
  PRINTLN("start_game");

  lcd.clear();
  setLight(TEAL);
  ROBOSFX(moveservos(RoboBrrd::ServosAll, RoboBrrd::Middle));

  // Use system timer to randomize the random generator
  randomSeed((unsigned)millis());

  for (int i = 0; i < 20; i++) 
  {
    hazards[i] = 0;
  }

  init_hazard(WUMPUS);
  init_hazard(PIT);
  init_hazard(PIT);
  init_hazard(BAT);
  init_hazard(BAT);

  // Make sure the player starts in a room with no hazards.
  // It's not fun to be killed before you make the first move.
  do
  {
    player_room = random(20);
  } while (hazards[player_room]);

  arrow_count = 2;
  selected_menu_idx = 0;

  state = begin_move_room;
}


//===========================================================================
// Bat states
//===========================================================================


//---------------------------------------------------------------------------
//! Start being moved by bats
void begin_bat_move() 
{
  lcd.clear();
  setLight(BLUE);

  ROBOSFX(moveservos(RoboBrrd::ServosAll, RoboBrrd::Upper, false));

  lcd.write(BAT_ICON_IDX);
  lcd.setCursor(5, 0);
  lcd.print(F("Bats!"));
  lcd.setCursor(0, 1);
  lcd.write(BAT_ICON_IDX);

  state = animate_bat_move;
}


//---------------------------------------------------------------------------
//! Bat move in progress
void animate_bat_move() 
{
  static unsigned long last_animate_time;

  if (time - last_animate_time > 200) 
  {
    lcd.scrollDisplayRight();
    last_animate_time = time;
    ROBOSFX(flapwings(false));
  }

  if (time - last_state_change_time >= 3000) 
  {
    state = end_bat_move;
  }
}


//---------------------------------------------------------------------------
//! End of bat move
void end_bat_move() 
{
  hazards[player_room] = 0;
  init_hazard(BAT);

  player_room = random(20);

  state = begin_move_room;

  ROBOSFX(moveservos(RoboBrrd::ServosAll, RoboBrrd::Middle));
}


//===========================================================================
// Moving states / functions
//===========================================================================


//---------------------------------------------------------------------------
//! Delay to show a status before continuing with the room move.
void status_delay() 
{
  if (time - last_state_change_time >= 3000) 
  {
    state = begin_move_room;
  }
}


//---------------------------------------------------------------------------
//! Check for hazards when a player changes rooms
void begin_move_room() 
{
  switch (hazards[player_room]) 
  {
  case BAT:
    state = begin_bat_move;
    break;

  case PIT:
    state = game_over_pit;
    break;

  case WUMPUS:
    state = game_over_wumpus;
    break;

  default:
    state = enter_new_room;
    break;
  }
}


//---------------------------------------------------------------------------
//! Enter a new room
void enter_new_room() 
{
  int adjacent_hazards = NONE;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Room "));
  print_cave_number(player_room);

  for (int i=0; i<3; i++) 
  {
    adjacent_hazards |= hazards[cave[player_room][i]];
  }

  lcd.print(' ');    
  if (adjacent_hazards & WUMPUS) 
  {
    lcd.write(WUMPUS_ICON_IDX);
  }
  else
  {
    lcd.print(' ');
  }
  if (adjacent_hazards & BAT) 
  {
    lcd.write(BAT_ICON_IDX);
  }
  else
  {
    lcd.print(' ');
  }
  if (adjacent_hazards & PIT) 
  {
    lcd.write(PIT_ICON_IDX);
  }
  else
  {
    lcd.print(' ');
  }

  lcd.setCursor(14, 0);
  lcd.write(ARROW_ICON_IDX);
  lcd.print(arrow_count);

  if (adjacent_hazards) 
  {
    setLight(YELLOW);
  }
  else 
  {
    setLight(TEAL);
  }

  lcd.setCursor(1, 1);
  for (int i=0; i<3; i++) 
  {
    print_cave_number(cave[player_room][i]);
    lcd.print(' ');
  }

  selected_menu_idx = 0;
  lcd.setCursor(menu_col[selected_menu_idx][0], 1);
  lcd.print('[');
  lcd.setCursor(menu_col[selected_menu_idx][1], 1);
  lcd.print(']');

  state = begin_input_move;
}


//---------------------------------------------------------------------------
//! Start move
void begin_input_move() 
{
  lcd.setCursor(0, 0);
  lcd.print(F("Room "));
  print_cave_number(player_room);
  lcd.print(' ');

  lcd.setCursor(10, 1);
  lcd.print(F("Shoot"));

  state = input_move;
}


//---------------------------------------------------------------------------
//! Handle moving input
void input_move() 
{
  if (clicked_buttons) 
  {
    clear_current_menu();

    if (clicked_buttons & BUTTON_SELECT) 
    {
      if (selected_menu_idx < 3) 
      {
        player_room = cave[player_room][selected_menu_idx];
        state = begin_move_room;
      }
      else 
      {
        state = begin_input_arrow;
      }
    }
    else 
    {
      handle_menu();
    }

    highlight_current_menu();
  }
}


//===========================================================================
// Arrow shooting states / functions
//===========================================================================


//---------------------------------------------------------------------------
//! Start arrow
void begin_input_arrow() 
{
  setLight(WHITE);
  lcd.setCursor(0, 0);
  lcd.print(F("Shoot at"));

  lcd.setCursor(10, 1);
  lcd.print(F("Move "));

  state = input_arrow;
}


//---------------------------------------------------------------------------
//! Arrow input handler
void input_arrow() 
{
  if (clicked_buttons) 
  {
    clear_current_menu();

    if (clicked_buttons & BUTTON_SELECT) 
    {
      if (selected_menu_idx < 3) 
      {
        arrow_room = cave[player_room][selected_menu_idx];
        state = being_shooting_arrow;
      }
      else 
      {
        state = cancel_input_arrow;
      }
    }
    else 
    {
      handle_menu();
    }

    highlight_current_menu();
  }
}


//---------------------------------------------------------------------------
//! Go back from arrow to move mode
void cancel_input_arrow() 
{
  int adjacent_hazards = NONE;

  for (int i=0; i<3; i++) 
  {
    adjacent_hazards |= hazards[cave[player_room][i]];
  }

  if (adjacent_hazards) 
  {
    setLight(YELLOW);
  }
  else 
  {
    setLight(TEAL);
  }

  state = begin_input_move;
}


//---------------------------------------------------------------------------
//! Begin shooting the arrow 
void being_shooting_arrow() 
{
  lcd.clear();
  setLight(VIOLET);
  lcd.print(F(">-->"));

  arrow_count--;
  state = animate_shooting_arrow;
  ROBOSFX(moveservos(RoboBrrd::ServosLeft, RoboBrrd::Upper, false));
}


//---------------------------------------------------------------------------
//! Shooting arrow in progress
void animate_shooting_arrow() 
{
  static unsigned long last_animate_time;
#ifdef WUMPUS_ROBOBRRD
  static int pos;

  if (time - last_animate_time >= 3000)
  {
    pos = 255; // upper
  }
#endif

  if (time - last_animate_time > 200) 
  {
    lcd.scrollDisplayRight();
    last_animate_time = time;
    ROBOSFX(robobrrd.MoveExact(RoboBrrd::LWing, (pos = (-pos) >> 1)));
  }

  if (time - last_state_change_time >= 3000) 
  {
    if (hazards[arrow_room] == WUMPUS) 
    {
      state = game_over_win;
    }
    else 
    {
      ROBOSFX(detachservos());
      state = arrow_missed;
    }
  }
}


//---------------------------------------------------------------------------
//! The arrow missed the Wumpus
void arrow_missed() 
{
  lcd.clear();
  lcd.print(F("Missed..."));

  if (arrow_count <= 0) 
  {
    state = game_over_arrow;
  }
  else 
  {
    state = status_delay;
  }
}


//===========================================================================
// Game over states / functions
//===========================================================================


//---------------------------------------------------------------------------
//! Game over
void draw_game_over_screen(
  uint8_t backlight, 
  const __FlashStringHelper *message, uint8_t icon) 
{
  lcd.clear();
  setLight(backlight);
  lcd.print(message);
  lcd.setCursor(0, 1);
  lcd.write(icon);
  lcd.setCursor(3, 1);
  lcd.print(F("GAME OVER"));
  lcd.setCursor(15, 1);
  lcd.write(icon);
}


//---------------------------------------------------------------------------
//! Game over, out of arrows
void game_over_arrow() 
{
  draw_game_over_screen(RED, F(" Out of arrows"), ARROW_ICON_IDX);
  ROBOSFX(moveservos(RoboBrrd::ServosAll, RoboBrrd::Lower, true));
  state = game_over_delay;
}


//---------------------------------------------------------------------------
//! Game over, fell into a pit
void game_over_pit() 
{
  draw_game_over_screen(RED, F(" Fell in a pit"), PIT_ICON_IDX);
  ROBOSFX(sfxfall());
  state = game_over_delay;
}


//---------------------------------------------------------------------------
//! Game over, Wumpus ate us
void game_over_wumpus() 
{
  draw_game_over_screen(RED, F("Eaten by Wumpus"), WUMPUS_ICON_IDX);
  ROBOSFX(sfxwumpus());
  state = game_over_delay;
}


//---------------------------------------------------------------------------
//! Game over, shot the Wumpus
void game_over_win() 
{
  draw_game_over_screen(GREEN, F("    You win!"), WUMPUS_ICON_IDX);  
  ROBOSFX(sfxcheers());
  state = game_over_delay;
}


//---------------------------------------------------------------------------
//! Wait for a while after game over
void game_over_delay() 
{
  if (time - last_state_change_time >= 3000) 
  {
    state = begin_splash_screen;
  }
}


/////////////////////////////////////////////////////////////////////////////
// CLOCK STATES
/////////////////////////////////////////////////////////////////////////////


#ifdef WUMPUS_ROBOBRRD
#ifdef ROBOBRRD_HAS_GPS
//---------------------------------------------------------------------------
//! Reset screensaver
void reset_screensaver()
{
  screensaver_timestamp = time;

  if (screensaver_on)
  {
    screensaver_on = false;

    lcd.display();
    setLight(screensaver_light);
  }
}


//---------------------------------------------------------------------------
//! Activate screen saver if time has expired
void check_screensaver(unsigned long duration) 
{
  // If there's enough light, the screensaver should stay off
  if ( ( (robobrrd.GetLDR(RoboBrrd::Left ) >= screensaver_brightness_threshold)
      || (robobrrd.GetLDR(RoboBrrd::Right) >= screensaver_brightness_threshold)))
  {
    if (screensaver_on)
    {
      reset_screensaver();
    }
  }
  else
  {
    if ( (!screensaver_on) 
      && (time - screensaver_timestamp >= duration))
    {
      screensaver_on = true;

      lcd.noDisplay();
      lcd.setBacklight(0); // Don't use setLight here!
      robobrrd.Led(RoboBrrd::SidesBoth, false, false, false);
    }
  }
}


//---------------------------------------------------------------------------
//! Convert hour to AM/PM notation if necessary
byte format_hour(byte hour)
{
  byte result = hour;

  if (enable12h)
  {
    result = (result == 0 ? 12 : (result > 12 ? result - 12 : result));
  }

  return result;
}


//---------------------------------------------------------------------------
//! Show the time on current line
NoAutoProto(
void line_time(tmElements_t &t))
{
  byte h = format_hour(t.Hour);
  lcd << (h < 10 ? F("   ") : F("  ")) << h
    << F(":") << (t.Minute < 10 ? F("0") : F("")) << t.Minute
    << (t.Second < 10 ? F(":0") : F(":")) << t.Second
    << (enable12h ? 
      (t.Hour >= 12 ? F(" PM   ") : F(" AM   ")) : F("      "));

}


//---------------------------------------------------------------------------
//! Show the date on current line
NoAutoProto(
void line_date(tmElements_t &t))
{
  lcd << dayShortStr(t.Wday) 
    << (t.Day < 10 ? F(" 0") : F(" ")) << t.Day 
    << F("-") << monthShortStr(t.Month)
    << F("-") << tmYearToCalendar(t.Year) << F(" ");
}


//---------------------------------------------------------------------------
//! Show the temperature on current line
void line_temperature()
{
  unsigned u = (robobrrd.GetFahrenheit() + 5) / 10;

  lcd << F("Temperature ") << (u < 100 ? F(" ") : F("")) 
    << (u < 10 ? F(" ") : F("")) << u << F("F");
}


//---------------------------------------------------------------------------
//! Show the amount of light (just for debugging and calibrating)
void line_light()
{
  unsigned left = robobrrd.GetLDR(RoboBrrd::Left);
  unsigned right = robobrrd.GetLDR(RoboBrrd::Right);

  lcd << F("Light: ") << left << F("/") << right << "      ";
}


//---------------------------------------------------------------------------
//! Show the GPS status on current line
void line_gps_state()
{
  switch (robobrrd.m_gps_state)
  {
  case RoboBrrd::GPSStateUnknown:   lcd << F("GPS Disconnected"); break;
  case RoboBrrd::GPSStateOnline:    lcd << F("GPS No data yet "); break;
  case RoboBrrd::GPSStateData:      lcd << F("GPS Insuff. data"); break;
  case RoboBrrd::GPSStateValidData: lcd << F("GPS No date/time"); break;
  case RoboBrrd::GPSStateGotTime:   lcd << F("GPS No date yet "); break;
  case RoboBrrd::GPSStateGotDate:   lcd << F("GPS Receiving OK"); break;

  default:                          
    lcd << F("GPS ????????????");
  }
}


//---------------------------------------------------------------------------
//! Show the time
void show_clock()
{
  static time_t prevtime = 0;
  time_t newtime = now();

  if (newtime != prevtime)
  {
    prevtime = newtime;

    tmElements_t t;
    breakTime(newtime, t);

    static byte seq = 0;
    static byte timesdisplayed;

    // If we just changed state, make sure the message is shown long enough
    if (time - last_state_change_time < 1000)
    {
      timesdisplayed = 0;
    }

    lcd.setCursor(0, 0);

    if (++timesdisplayed == 5)
    {
      timesdisplayed = 0;
      seq++;
    }

    do 
    {
      switch(seq)
      {
      case 0: line_date(t);       break;
      case 1: line_temperature(); break;
      case 2: line_light();       break;
      case 3: line_gps_state();   break;
      default:
        seq = 0; continue;
      }
    } while(0);

    lcd.setCursor(0, 1);
    if (timeStatus() == timeSet)
    {
      line_time(t);

      check_screensaver(screensaver_clock_timeout);
    }
    else
    {
      line_gps_state(); 
      delay(1000);
    }
  }

  if (clicked_buttons & BUTTON_SELECT)
  {
    state = begin_splash_screen;
  }
}
#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// ARDUINO TOP-LEVEL FUNCTIONS
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//! Perform one time setup for the game and put it in splash screen state.
void setup() 
{
#ifdef DEBUG
  Serial.begin(9600);
  delay(1000);
  PRINTLN("Hello World!");
#endif

#ifdef WUMPUS_ROBOBRRD
#ifdef WUMPUS_ROBOBRRD_EEPROM
  // If the pins and values are stored in the EEPROM, retrieve them.
  // Otherwise, store the defaults into the EEPROM.
  EEPROM_mgr::Begin();

#endif // WUMPUS_ROBOBRRD_EEPROM
  robobrrd.Attach(false
#ifdef ROBOBRRD_HAS_GPS
    , true
#endif
    );
#else
  // LCD has 16 columns & 2 rows
  lcd.begin(16, 2);
#endif // WUMPUS_ROBOBRRD

  // Define custom icons

  lcd.createChar(WUMPUS_ICON_IDX, icons[WUMPUS_ICON_IDX]);
  lcd.createChar(BAT_ICON_IDX,    icons[BAT_ICON_IDX]);
  lcd.createChar(PIT_ICON_IDX,    icons[PIT_ICON_IDX]);
  lcd.createChar(ARROW_ICON_IDX,  icons[ARROW_ICON_IDX]);

  lcd.clear();

  setLight(TEAL);

  // Initial game state
#if defined(WUMPUS_ROBOBRRD) && defined(ROBOBRRD_HAS_GPS)
  state = show_clock;
#else
  state = begin_splash_screen;
#endif
}


//---------------------------------------------------------------------------
//! Main loop of execution.
void loop() 
{
  static StateFunc_t *last_state;

  time = millis();

  // Record time of state change so animations
  // know when to stop
  if (last_state != state) 
  {
    last_state = state;
    last_state_change_time = time;
#if defined(WUMPUS_ROBOBRRD) && defined(ROBOBRRD_HAS_GPS)
    reset_screensaver();
#endif
  }

  // Read in which buttons were clicked
  read_button_clicks();

  // Call current state function
  state();

  delay(10);
}


/////////////////////////////////////////////////////////////////////////////
// END
/////////////////////////////////////////////////////////////////////////////
