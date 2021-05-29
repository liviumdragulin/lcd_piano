#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DEBUG

#ifdef DEBUG
#define dprint(s) Serial.print(s)
#define dprintln(s) Serial.println(s)
#else
#define dprint(s) do {} while(0)
#define dprintln(s) do {} while(0)
#endif

// BUTTONS
const unsigned int buttons[] = {
	3,  // DOL
	4,  // RE
	5,  // MI
	6,  // FA
	7,  // SOL
	8,  // LA
	9,  // SI
	10, // DOH
	11, // MODE_PREV
	12  // MODE_NEXT
}; // PINS

const char buttons_char[][10] {
	{"DOL"},
	{"RE"},
	{"MI"},
	{"FA"},
	{"SOL"},
	{"LA"},
	{"SI"},
	{"DOH"},
	{"MODE_PREV"},
	{"MODE_NEXT"}
}; // BUTTONS AS CHAR
const unsigned int buttons_count = sizeof(buttons) / sizeof(buttons[0]);

enum buttons_type {
	DOL = 0,
	RE = 1,
	MI = 2,
	FA = 3,
	SOL = 4,
	LA = 5,
	SI = 6,
	DOH = 7,
	MODE_PREV = 8,
	MODE_NEXT = 9
};


// PIANO
enum piano_type { NORMAL = 0, RECORD = 1, LISTEN = 2 };
const char piano_states[][7] = {
	{"NORMAL"},
	{"RECORD"},
	{"LISTEN"}
};
unsigned int piano_mode = NORMAL;
const unsigned int PIANO_MODES_COUNT = 3;
unsigned int buttons_state[buttons_count] = {0}; // LOW = 0


// BUZZER
const int BUZZER = 2;

const unsigned int note_duration = 1000 / 4;
const unsigned int notes_freqs[] = {
	440, // DOL
	494, // RE
	523, // MI
	587, // FA
	659, // SOL
	698, // LA
	783, // SI
	880  // DOH
};

const unsigned int SONG_CAPACITY = 127;
unsigned int song[SONG_CAPACITY] = {0};
unsigned int song_length = 0;

unsigned long previous_millis = 0;
unsigned int listen_cursor = 0;

unsigned int record_cursor = 0;
bool can_record = false;


// LCD
enum LCD_PROPS { LCD_LINE_LENGTH = 16, LCD_LINE_COUNT = 2, LCD_ADDRESS = 0x27 };
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_LINE_LENGTH, LCD_LINE_COUNT);
bool lcd_line_changed = false;
char lcd_line[LCD_LINE_COUNT][LCD_LINE_LENGTH] = {{"NORMAL"}, {"##"}};


// change the lines of the lcd and
void lcd_change_line(unsigned int button)
{
	switch (button) {
		case DOL:
			strncpy(lcd_line[1], buttons_char[DOL], LCD_LINE_LENGTH);
			break;

		case RE:
			strncpy(lcd_line[1], buttons_char[RE], LCD_LINE_LENGTH);
			break;

		case MI:
			strncpy(lcd_line[1], buttons_char[MI], LCD_LINE_LENGTH);
			break;

		case FA:
			strncpy(lcd_line[1], buttons_char[FA], LCD_LINE_LENGTH);
			break;

		case SOL:
			strncpy(lcd_line[1], buttons_char[SOL], LCD_LINE_LENGTH);
			break;

		case LA:
			strncpy(lcd_line[1], buttons_char[LA], LCD_LINE_LENGTH);
			break;

		case SI:
			strncpy(lcd_line[1], buttons_char[SI], LCD_LINE_LENGTH);
			break;

		case DOH:
			strncpy(lcd_line[1], buttons_char[DOH], LCD_LINE_LENGTH);
			break;

		case MODE_PREV:
			strncpy(lcd_line[0], piano_states[piano_mode], LCD_LINE_LENGTH);
			break;

		case MODE_NEXT:
			strncpy(lcd_line[0], piano_states[piano_mode], LCD_LINE_LENGTH);
			break;

		default:
			strncpy(lcd_line[1], "##", LCD_LINE_LENGTH);
			break;
	}

	dprintln("Changed one of the lcd lines");

	if (piano_mode == LISTEN) {
		strncpy(lcd_line[1], "##", LCD_LINE_LENGTH);
		dprintln("Changed lcd last pressed key to: NONE (LISTEN MODE)");
	}

	lcd_line_changed = true;
}

// update the lines of the lcd
void update_lcd()
{
	lcd.clear();

	lcd.setCursor(5, 0);
	lcd.print(lcd_line[0]);

	lcd.setCursor(7, 1);
	lcd.print(lcd_line[1]);
}


// Play the note received as parameter
void play_note(unsigned int note)
{
	if (note > DOH) {
		dprintln("invalid note");
		return;
	}

	tone(BUZZER, notes_freqs[note], note_duration);
}

// Start the recording of the song
void start_recording(void)
{
	strncpy(lcd_line[1], "oo", LCD_LINE_LENGTH);
	dprintln("Changed one of the lcd lines");

	lcd_line_changed = true;

	memset(song, 0, SONG_CAPACITY * sizeof(int));
	dprintln("Deleted last saved song");

	can_record = true;
}

// Save the song to the EEPROM
void save_song(void)
{
	song_length = record_cursor;

	EEPROM.put(0, song_length);
	dprintln("Saved song length to EEPROM");

	for (unsigned long i = sizeof(int); i < (song_length + 1) * sizeof(int); i += sizeof(int)) {
		EEPROM.put(i, song[i / sizeof(int) - 1]);
	}

	dprintln("Saved entire song to EEPROM");

	for (unsigned long i = (song_length + 1) * sizeof(int); i < EEPROM.length(); i += sizeof(int)) {
		EEPROM.put(i, 0);
	}
}

/*
* Save the last note played and if we reached the capacity of the song
* save the entire song to the EEPROM and put the piano in the next mode
*/
void save_note(unsigned int note)
{
	song[record_cursor++] = note;
	dprint("Saved ");
	dprint(buttons_char[note]);
	dprint(" at position ");
	dprintln(record_cursor - 1);

	if (record_cursor >= SONG_CAPACITY) {
		save_song();

		piano_mode = (piano_mode + 1) % PIANO_MODES_COUNT;
		lcd_change_line(MODE_NEXT);

		dprint("Changed piano mode to: ");
		dprintln(piano_states[piano_mode]);

		record_cursor = 0;
		can_record = false;
	}

	dprintln("Max length of song reached");
}


// read the states (LOW = pressed; HIGH = unpressed) of the buttons
void read_buttons_states(void)
{
	for (int i = 0; i < buttons_count; ++i) {
		buttons_state[i] = digitalRead(buttons[i]);
	}
}

// do the desired action for each button
void compute_buttons_states(void)
{
	if (buttons_state[MODE_NEXT] == LOW) {
		dprint("Pressed key: ");
		dprintln(buttons_char[MODE_NEXT]);

		if (piano_mode == RECORD && can_record) {
			save_song();

			can_record = false;
		}

		piano_mode = (piano_mode + 1) % PIANO_MODES_COUNT;
		lcd_change_line(MODE_NEXT);

		// reset cursors;
		listen_cursor = 0;
		record_cursor = 0;

		dprint("Changed piano mode to: ");
		dprintln(piano_states[piano_mode]);
	}

	if (buttons_state[MODE_PREV] == LOW) {
		dprint("Pressed key: ");
		dprintln(buttons_char[MODE_NEXT]);

		if (piano_mode == NORMAL) {
			piano_mode = LISTEN;
		} else {
			piano_mode = piano_mode - 1;
		}

		lcd_change_line(MODE_PREV);

		// reset cursors;
		listen_cursor = 0;
		record_cursor = 0;

		dprint("Changed piano mode to: ");
		dprintln(piano_states[piano_mode]);
	}

	if (piano_mode != LISTEN) {
		if (buttons_state[DOL] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[DOL]);

			lcd_change_line(DOL);
			play_note(DOL);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(DOL);
				}
			}
		}

		if (buttons_state[RE] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[RE]);

			lcd_change_line(RE);
			play_note(RE);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(RE);
				}
			}
		}

		if (buttons_state[MI] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[MI]);

			lcd_change_line(MI);
			play_note(MI);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(MI);
				}
			}
		}

		if (buttons_state[FA] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[FA]);

			lcd_change_line(FA);
			play_note(FA);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(FA);
				}
			}
		}

		if (buttons_state[SOL] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[SOL]);

			lcd_change_line(SOL);
			play_note(SOL);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(SOL);
				}
			}
		}

		if (buttons_state[LA] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[LA]);

			lcd_change_line(LA);
			play_note(LA);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(LA);
				}
			}
		}

		if (buttons_state[SI] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[SI]);

			lcd_change_line(SI);
			play_note(SI);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(SI);
				}
			}
		}

		if (buttons_state[DOH] == LOW) {
			dprint("Pressed key: ");
			dprintln(buttons_char[DOH]);

			lcd_change_line(DOH);
			play_note(DOH);

			if (piano_mode == RECORD) {
				if (!can_record) {
					start_recording();
				} else {
					save_note(DOH);
				}
			}
		}
	} else {
		/*
		* Piano is in LISTEN mode
		*
		* This means we don't have to play the tones of the buttons,
		* only the tones of the recorded song
		*/
		unsigned long current_millis = millis();

		// Play the next song only if the previous one ended
		if ((song_length > 0) && (current_millis - previous_millis >= note_duration)) {
			previous_millis = current_millis;

			play_note(song[listen_cursor]);

			listen_cursor = (listen_cursor + 1) % song_length;
		}
	}

	for (unsigned int i = 0; i < buttons_count; ++i) {
		/*
		if the piano is in the LISTEN MODE
		playing notes by buttons is disabled
		so we don't need to wait if a button
		is touched
		*/
		if (piano_mode == LISTEN && i <= DOH) {
			continue;
		}

		if (buttons_state[i] == LOW) {
			delay(note_duration);
			break;
		}
	}
}

void setup()
{
	// initialize serial interface
	Serial.begin(9600);

	while (!Serial) {}

	dprintln("Initialized Serial interface");

	// initialize pins
	for (unsigned int i = 0; i < buttons_count; ++i) {
		pinMode(buttons[i], INPUT_PULLUP);
		buttons_state[i] = HIGH;
	}

	dprintln("Initialized pins");


	// load song from EEPROM

	// load the length of the song first
	EEPROM.get(0, song_length);
	dprint("Song length from EEPROM: ");
	dprintln(song_length);

	// load the notes of the song
	dprint("Song notes from EERPOM: ");

	for (unsigned long i = sizeof(int); i < (song_length + 1) * sizeof(int); i += sizeof(int)) {
		EEPROM.get(i, song[i / sizeof(int) - 1]);
		dprint(buttons_char[song[i / sizeof(int) - 1]]);
		dprint(" ");
	}

	dprintln();

	// To show that the buzzer plays the same note with different sounds
	//  song_length = 1;

	// initialize the lcd
	lcd.init();
	lcd.backlight();
	lcd.setCursor(5, 0);
	lcd.print(lcd_line[0]);
	lcd.setCursor(7, 1);
	lcd.print(lcd_line[1]);
}

void loop()
{
	// read buttons states
	read_buttons_states();

	// compute pressed buttons
	compute_buttons_states();

	// if a button is pressed change the display
	if (lcd_line_changed) {
		lcd_line_changed = false;

		update_lcd();
	}
}
