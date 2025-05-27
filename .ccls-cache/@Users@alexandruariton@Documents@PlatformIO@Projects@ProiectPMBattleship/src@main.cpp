#include "defs.hpp"

#define RECT_W 200
#define RECT_H 90
int modulus = 0;
int mode = PUTTING_SHIPS;
int can_put_ship = 1;
int score = 0;
int declanseaza_aratare = 0;
int millis_passed_declanseaza_aratare = 0;
int millis_limit_declanseaza_aratare = 1000;
int millis_init_declanseaza_aratare = 0;
int sw_decl_aratare = 0;
volatile int do_movex = 0;
volatile int do_movey = 0;
volatile int do_orientation = 0;
volatile int do_paste = 0;

int last_drawn_time = 0;
int last_drawn_limit = 200;

int blast_drawn_time = 0;
int blast_drawn_limit = 200;
Ship ships[SHIP_NR + 2];
int8_t ship_len = 0;
Ship pending_ship;


Bomb bombs[BOMB_NR + 2];
int8_t bomb_len = 0;
Bomb pending_bomb;

TFT_eSPI tft = TFT_eSPI();
void tft_draw_pending_ship(Ship ship, int pend, int index, int hide) {
	if (ship.orientation == 44) return;
	uint16_t color;
	int width, height;

	if (pend) {
		if (ship_len % 2 == modulus) {
			if (ship.orientation <= 1) {
    			color = TFT_DARKCYAN;
			} else if (ship.orientation <= 3) {
    			color = TFT_CYAN;
			}
		} else {
			if (ship.orientation <= 1) {
    			color = TFT_ORANGE;
			} else if (ship.orientation <= 3) {
    			color = TFT_BROWN;
			}
		}
	} else {
		if (index % 2 == modulus) {
			color = TFT_BLUE;
		} else {
			color = TFT_RED;
		}
	}

	if (ship.orientation % 2 == 0) {
    	width = SHIP_WIDTH;
    	height = SHIP_HEIGHT;
	} else {
    	width = SHIP_HEIGHT;
    	height = SHIP_WIDTH;
	}
	if(index % 2 != modulus && hide == 1) {

	} else {
		tft.fillRect(ship.pos1, ship.pos2, width, height, color);
	}
}

void tft_draw_ships(int hide) {
	if (millis() - last_drawn_time < last_drawn_limit) {
		return;
	} 
	last_drawn_time = millis();
	tft.fillScreen(TFT_BLACK);
	tft.drawRect(0, 0, RECT_W, RECT_H, TFT_WHITE);
	tft_draw_pending_ship(pending_ship, 1, 0, 0);
	for(int i = 0; i < SHIP_NR; i ++) {
		tft_draw_pending_ship(ships[i], 0, i, hide);
	}
	char scr[65] = {0};
	snprintf(scr, 60, "score: %d, %s. %s: %d/%d", score, (modulus == (mode == PUTTING_SHIPS ? ship_len % 2 : bomb_len % 2) ? "Your turn" : "Wait..."),
						(mode == PUTTING_SHIPS ? "ships" : "bombs"), (mode == PUTTING_SHIPS ? ship_len : bomb_len),
						(mode == PUTTING_SHIPS ? SHIP_NR : BOMB_NR)
					);
	tft.drawString(scr, 10, 120);
}



void tft_draw_pending_bomb(Bomb bomb, int pend, int index) {
	if (bomb.orientation == 44) return;
	uint16_t color;
	if (pend) {
		Serial.println("Orientation:");
		Serial.println(bomb.orientation);
		if (bomb_len % 2 == modulus) {
			if (bomb.orientation <= 1) {
    			color = TFT_GREENYELLOW;
			} else if (bomb.orientation <= 3) {
    			color = TFT_DARKGREEN;
			}
		} else {
			if (bomb.orientation <= 1) {
    			color = TFT_LIGHTGREY;
			} else if (bomb.orientation <= 3) {
    			color = TFT_DARKGREY;
			}
		}
	} else {
		if (index % 2 == modulus) {
			color = TFT_GREEN;
		} else {
			color = TFT_WHITE;
		}
	}


	tft.fillRect(bomb.pos1, bomb.pos2, BOMB_WIDTH, BOMB_HEIGHT, color);
}

void tft_draw_bombs() {
	if (millis() - blast_drawn_time < blast_drawn_limit) {
		return;
	} 
	blast_drawn_time = millis();
	tft_draw_pending_bomb(pending_bomb, 1, 0);
	for(int i = 0; i < BOMB_NR; i ++) {
		tft_draw_pending_bomb(bombs[i], 0, i);
	}
}

int do_received_command_ship(const char* msg) {
	ParsedCommand c;
	c.pos1 = 0;
	c.type = 'u';
	c.pos2 = 0;
	parse_command(msg, &c);
	if (ship_len >= SHIP_NR) {
		ship_len = 0;
		return -1;
	}
	if(c.type == ADD_SHIP) {
		pending_ship.orientation = c.orientation;
		pending_ship.pos1 = c.pos1;
		pending_ship.pos2 = c.pos2;
	} else if(c.type == MOVE_SHIP) {
		pending_ship.orientation += c.orientation;
		if(c.orientation != 0) {
			pending_ship.orientation = pending_ship.orientation % 4;
		}
		pending_ship.pos1 += SHIP_WIDTH * c.pos1 * (pending_ship.orientation >= 2 ? -1 : 1);
		pending_ship.pos2 += SHIP_WIDTH * c.pos2 * (pending_ship.orientation >= 2 ? -1 : 1);
		Serial.println("Ship pos 1:");
		Serial.print(pending_ship.pos1);
		Serial.println(' ');
	} else if(c.type == PASTE_SHIP) {
		ships[ship_len] = pending_ship;
		ship_len ++;
		declanseaza_aratare = 1;
	} else {
		mode = 1;
		pending_bomb.orientation = 0;
		return -1;
	}
	if (c.type == ADD_SHIP) {
		tft_draw_ships(0);
	} else if(c.type == MOVE_SHIP) {
		tft_draw_ships(1);
	}
	tft_draw_bombs();
	return 0;
}


int do_received_command_bomb(const char* msg) {
	ParsedCommand c;
	c.pos1 = 0;
	c.type = 'u';
	c.pos2 = 0;
	parse_command(msg, &c);
	if (bomb_len >= BOMB_NR) {
		return 9;
	}
	if(c.type == ADD_BOMB) {
		pending_bomb.orientation = c.orientation;
		pending_bomb.pos1 = c.pos1;
		pending_bomb.pos2 = c.pos2;
	} else if(c.type == MOVE_BOMB) {
		pending_bomb.orientation += c.orientation;
		if(c.orientation != 0) {
			pending_bomb.orientation = pending_bomb.orientation % 4;
		}
		pending_bomb.pos1 += BOMB_WIDTH * c.pos1 * (pending_bomb.orientation >= 2 ? -1 : 1);
		pending_bomb.pos2 += BOMB_HEIGHT * c.pos2 * (pending_bomb.orientation >= 2 ? -1 : 1);
		Serial.println("bomb pos 1:");
		Serial.print(pending_bomb.pos1);
		Serial.println(' ');
	} else if(c.type == PASTE_BOMB) {
		bombs[bomb_len] = pending_bomb;
		bomb_len ++;
		declanseaza_aratare = 1;
	}
	if(c.type != PASTE_BOMB) {
		tft_draw_ships(1);
		tft_draw_bombs();
	}
	return 0;
}


int do_received_command(const char* msg) {

	ParsedCommand c;
	c.pos1 = 0;
	c.type = 'u';
	c.pos2 = 0;
	parse_command(msg, &c);
	if(c.type == 's') {
		mode = PUTTING_BOMBS;
		pending_bomb.orientation = 0;
		pending_ship.orientation = 44;
	} else if(c.type == 'w') {
		modulus = 2;
		tft.fillScreen(TFT_BLUE);
		char final[25] = {0};
		snprintf(final, 10, "score: %d", score);
		tft.drawString(final, 30, 30, 2);
	}
	if (mode == PUTTING_SHIPS) {
		return do_received_command_ship(msg);
	} else {
		return do_received_command_bomb(msg);
	}
}
uint8_t MAC_1[] = { 0xA0, 0xDD, 0x6C, 0x6F, 0x85, 0x7C };
uint8_t MAC_2[] = { 0xA0, 0xDD, 0x6C, 0x74, 0x8B, 0x70 };
uint8_t *peer_mac;

volatile int do_received = 0;
char incoming_data_str[50] = {0};

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
	strncpy(incoming_data_str, (char*)incomingData, len);

	// int r = do_received_command(incoming_data_str);
	do_received = 1;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
}

unsigned long debounce_delay = 300;
unsigned long last_movex_time = 0;
unsigned long last_movey_time = 0;
unsigned long last_paste_time = 0;
unsigned long last_orient_time = 0;


int validate() {
	if(mode == PUTTING_SHIPS) {
		if (ship_len % 2 != modulus) {
			return 0;
		}
	} else if(mode == PUTTING_BOMBS) {
		if (bomb_len % 2 != modulus) {
			return 0;
		}
	} else {
		return 0;
	}
	return 1;
}

int movexx = 0;

void IRAM_ATTR onMoveXPlayer1() {
	do_movex = 1;
}

void DO_onMoveXPlayer1() {
	if (!validate()) {
		return;
	}
	Serial.print("Move x: \n");
	if(millis() - last_movex_time > debounce_delay) {
		last_movex_time = millis();
	} else { return; }
	if (mode == PUTTING_SHIPS) {
		const char *msg = "m|1|0|0";
		esp_err_t result =
			esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	} else {
		const char *msg = "B|1|0|0";
		esp_err_t result =
			esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	}
	movexx = 1;
}

void IRAM_ATTR onMoveYPlayer1() {
 do_movey = 1;
}

void DO_onMoveYPlayer1() {
	if (!validate()) {
		return;
	}

	if(millis() - last_movey_time > debounce_delay) {
		last_movey_time = millis();
	} else { return; }
	if (mode == PUTTING_SHIPS) {
		const char *msg = "m|0|1|0";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	} else {
		const char *msg = "B|0|1|0";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	}
	movexx = 1;
}

void IRAM_ATTR onChnageOrientationPlayer1() {
 do_orientation = 1;
}

void DO_onChnageOrientationPlayer1() {
	if (!validate()) {
		return;
	}

	if(millis() - last_orient_time > debounce_delay) {
		last_orient_time = millis();
	} else { return; }

	if (mode == PUTTING_SHIPS) {
		const char *msg = "m|0|0|1";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	} else {
		const char *msg = "B|0|0|1";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	}
	movexx = 1;
}

void onAddPlayer1() {

	if (mode == PUTTING_SHIPS) {
		const char *msg = "a|0|0|0";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	} else {
		const char *msg = "b|0|0|0";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);
	}
}

void IRAM_ATTR onPastePlayer1() {
	do_paste = 1;
}

void DO_onPastePlayer1() {
	if (!validate()) {
		return;
	}

	if(millis() - last_paste_time > debounce_delay) {
		last_paste_time = millis();
	} else { return; }

	if(can_put_ship == 1) {
		return;
	}


	if (mode == PUTTING_SHIPS) {
		const char *msg = "p|0|0|0";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);

		if (ship_len >= SHIP_NR) {
			const char *msg2 = "s|0|0|0";
			esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg2, strlen(msg));
			do_received_command(msg2);
			return;
		}
	} else {
		const char *msg = "P|0|0|0";
		esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg, strlen(msg));
		do_received_command(msg);

		if (bomb_len >= BOMB_NR) {
			const char *msg2 = "w|0|0|0";
			esp_err_t result =
				esp_now_send(peer_mac, (uint8_t *)msg2, strlen(msg));
			do_received_command(msg2);
			return;
		}
	}

	onAddPlayer1();

}


volatile int led_state = 0;
int led_limit = 200000; 
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
  if (ship_len % 2 == modulus) {
    portENTER_CRITICAL_ISR(&timerMux);
    led_state = 1 - led_state;
    digitalWrite(PLAYER_LED, led_state ? HIGH : LOW);
    portEXIT_CRITICAL_ISR(&timerMux);
  } else {
    digitalWrite(PLAYER_LED, LOW);
  }
}

void do_all() {
	if (do_received) {
		do_received_command(incoming_data_str);
		do_received = 0;
	}
	if (do_movex) {
		DO_onMoveXPlayer1();
		do_movex = 0;
	}

	if(do_movey) {
		DO_onMoveYPlayer1();
		do_movey = 0;
	}

	if(do_orientation) {
		DO_onChnageOrientationPlayer1();
		do_orientation = 0;
	}

	if(do_paste) {
		DO_onPastePlayer1();
		do_paste = 0;
	}
}

void setup()
{
	score = SHIP_NR / 2;
	pending_ship.orientation = 0;
	pending_ship.pos1 = 0;
	pending_ship.pos2 = 0;
	for (int i = 0; i < SHIP_NR; i ++) {
		ships[i].orientation = 44;
		ships[i].pos1 = 0;
		ships[i].pos2 = 0;
	}
	pending_bomb.orientation = 44;
	pending_bomb.pos1 = 0;
	pending_bomb.pos2 = 0;
	for (int i = 0; i < BOMB_NR; i ++) {
		bombs[i].orientation = 44;
		bombs[i].pos1 = 0;
		bombs[i].pos2 = 0;
	}
	tft.init();
	tft.setRotation(1);
	tft.fillScreen(TFT_BLACK);
	tft.drawString("Start game", 30, 30, 2);
	Serial.begin(115200);
	pinMode(MOVE_1_BUTTONX, INPUT_PULLUP);
	pinMode(MOVE_1_BUTTONY, INPUT_PULLUP);
	pinMode(MOVE_1_BUTTON_ORIENTATION, INPUT_PULLUP);
	pinMode(MOVE_1_DROP_SHIP, INPUT_PULLUP);
	pinMode(PLAYER_LED, OUTPUT);
	pinMode(INVALID_LED, OUTPUT);

	attachInterrupt(digitalPinToInterrupt(MOVE_1_BUTTONX), onMoveXPlayer1, CHANGE);
	attachInterrupt(digitalPinToInterrupt(MOVE_1_BUTTONY), onMoveYPlayer1, CHANGE);
	attachInterrupt(digitalPinToInterrupt(MOVE_1_BUTTON_ORIENTATION), 
								 onChnageOrientationPlayer1, CHANGE);
	attachInterrupt(digitalPinToInterrupt(MOVE_1_DROP_SHIP), onPastePlayer1, CHANGE);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, onTimer, true);
  timerAlarmWrite(timer, led_limit, true); 
  timerAlarmEnable(timer);
	WiFi.mode(WIFI_MODE_STA);

	// Get self MAC and determine peer
	String self_mac = WiFi.macAddress();
	Serial.println("My MAC: " + self_mac);

	if (self_mac.equalsIgnoreCase("A0:DD:6C:6F:85:7C")) {
		peer_mac = MAC_2;
		modulus = 1;
	} else {
		modulus = 0;
		peer_mac = MAC_1;
	}
	
	tft.fillScreen(TFT_BLACK);
	char game_id[15] = {0};



	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}

	esp_now_register_send_cb(OnDataSent);
	esp_now_register_recv_cb(OnDataRecv);

	esp_now_peer_info_t peerInfo = {};
	memcpy(peerInfo.peer_addr, peer_mac, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;

	if (!esp_now_is_peer_exist(peer_mac)) {
		if (esp_now_add_peer(&peerInfo) != ESP_OK) {
			Serial.println("Failed to add peer");
			return;
		}
	}

	tft_draw_ships(1);

}

int validate_ship_in_screen() {
		Ship ship = pending_ship;
		int width, height;
		int shipposx = ship.pos1;
		int shipposy = ship.pos2;
		if (ship.orientation % 2 == 0) {
    		width = SHIP_WIDTH;
    		height = SHIP_HEIGHT;
		} else {
    		width = SHIP_HEIGHT;
    		height = SHIP_WIDTH;
		}
		if (shipposx >= 0 &&
    shipposy >= 0 &&
    shipposx + width <= RECT_W &&
    shipposy + height <= RECT_H) {
			Serial.println("On screen");
			digitalWrite(INVALID_LED, LOW);	
			return 0;
    // Nava este complet cuprinsă în dreptunghi
		} else {
		// aprinde ledul
			Serial.println("Off screen");
			digitalWrite(INVALID_LED, HIGH);	
			return 1;
		}

}
int validate_bomb_in_screen() {
		Bomb bomb = pending_bomb;
		int width, height;
		int bombposx = bomb.pos1;
		int bombposy = bomb.pos2;
		if (bomb.orientation % 2 == 0) {
    		width = BOMB_WIDTH;
    		height = BOMB_HEIGHT;
		} else {
    		width = BOMB_HEIGHT;
    		height = BOMB_WIDTH;
		}
		if (bombposx >= 0 &&
    bombposy >= 0 &&
    bombposx + width <= RECT_W &&
    bombposy + height <= RECT_H) {

		  digitalWrite(INVALID_LED, LOW);	
		return 0;
    // Nava este complet cuprinsă în dreptunghi
		} else {
		// aprinde ledul
		  digitalWrite(INVALID_LED, HIGH);
		return 1;
		}

}

int validate_all() {
	if (mode == PUTTING_SHIPS) {
		return validate_ship_in_screen();
	} else {
		return validate_bomb_in_screen();
	}
}


void loop()
{
	do_all();
	if(movexx) {
		can_put_ship = validate_all();
		movexx = 0;
	}
	if (declanseaza_aratare == 1) {

		if (sw_decl_aratare == 0) {
			millis_init_declanseaza_aratare = millis();
			sw_decl_aratare = 1;

			if (mode == PUTTING_BOMBS) {
				for (int ship_index = 0; ship_index < ship_len; ship_index ++) {
					for (int bomb_index = 0; bomb_index < bomb_len; bomb_index ++) {
						Ship ship = ships[ship_index];
						int width, height;
						int shipposx = ship.pos1;
						int shipposy = ship.pos2;
						if (ship.orientation % 2 == 0) {
    						width = SHIP_WIDTH;
    						height = SHIP_HEIGHT;
						} else {
    						width = SHIP_HEIGHT;
    						height = SHIP_WIDTH;
						}
						Bomb bomb = bombs[bomb_index];
						int bombposx = bomb.pos1;
						int bombposy = bomb.pos2;
						int bomb_width = BOMB_WIDTH;
						int bomb_height = BOMB_HEIGHT;
						int collision = !(
            	bombposx + bomb_width  <= shipposx ||
            	bombposx >= shipposx + width ||
            	bombposy + bomb_height <= shipposy ||
            	bombposy >= shipposy + height
        		);

        		if(ship_index % 2 == modulus && bomb_index % 2 != modulus) {
        			if(ship.orientation <= 4 && bomb.orientation <= 4) {
        				if(collision) {
									score -= 1;
									ships[ship_index].orientation = 44;
								}
        			}
        		}
					}
				}
			}
		}
		if (millis() - millis_init_declanseaza_aratare > millis_limit_declanseaza_aratare) {
			// hide ships
			// validate ships:
			if (modulus < 2) {
				tft_draw_ships(1);
				if (mode == PUTTING_BOMBS) {
					tft_draw_bombs();
				}
				declanseaza_aratare = 0;
				sw_decl_aratare = 0;
				millis_init_declanseaza_aratare = 0;
			}
		}
	}
}
