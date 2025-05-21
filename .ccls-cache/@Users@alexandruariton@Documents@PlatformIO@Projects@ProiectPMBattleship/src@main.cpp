#include "defs.hpp"

int modulus = 0;
int mode = PUTTING_SHIPS;


Ship ships[SHIP_NR + 2];
int8_t ship_len = 0;
Ship pending_ship;


Bomb bombs[BOMB_NR + 2];
int8_t bomb_len = 0;
Bomb pending_bomb;

TFT_eSPI tft = TFT_eSPI();
void tft_draw_pending_ship(Ship ship, int pend, int index) {
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

	tft.fillRect(ship.pos1, ship.pos2, width, height, color);
}

void tft_draw_ships() {
	tft_draw_pending_ship(pending_ship, 1, 0);
	for(int i = 0; i < SHIP_NR; i ++) {
		tft_draw_pending_ship(ships[i], 0, i);
	}
}



void tft_draw_pending_bomb(Bomb bomb, int pend, int index) {
	if (bomb.orientation == 44) return;
	uint16_t color;
	if (pend) {
		if (bomb_len % 2 == modulus) {
    		color = TFT_DARKGREEN;
		} else {
    		color = TFT_DARKGREY;
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
	} else {
		mode = 1;
		pending_bomb.orientation = 0;
		return -1;
	}
	tft.fillScreen(TFT_BLACK);
	tft_draw_ships();
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
		pending_bomb.pos1 += BOMB_WIDTH * c.pos1;
		pending_bomb.pos2 += BOMB_HEIGHT * c.pos2;
		Serial.println("bomb pos 1:");
		Serial.print(pending_bomb.pos1);
		Serial.println(' ');
	} else if(c.type == PASTE_BOMB) {
		bombs[bomb_len] = pending_bomb;
		bomb_len ++;
	}
	tft.fillScreen(TFT_BLACK);
	tft_draw_ships();
	tft_draw_bombs();
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
	} else if(c.type == 'w') {
		modulus = 2;
		tft.fillScreen(TFT_BLUE);
		tft.drawString("Fin.", 30, 30, 2);
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


void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
	Serial.print("Received from: ");
	for (int i = 0; i < 6; i++) {
		Serial.print(mac[i], HEX);
		if (i < 5)
			Serial.print(":");
	}
	Serial.print(" | Data: ");
	Serial.write(incomingData, len);
	Serial.print("Finished_data");
	Serial.print(len);
	Serial.print("Finished len");

	char* incoming_data_str = (char*)calloc(len + 1, sizeof(char));

	strncpy(incoming_data_str, (char*)incomingData, len);
	int r = do_received_command(incoming_data_str);
	Serial.println(incoming_data_str);
	free(incoming_data_str);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
	Serial.print("Last Packet Send Status: ");
	Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

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

void IRAM_ATTR onMoveXPlayer1() {
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
}

void IRAM_ATTR onMoveYPlayer1() {
	if (!validate()) {
		return;
	}

	Serial.print("Move y: \n");
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
}

void IRAM_ATTR onChnageOrientationPlayer1() {
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
	}
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
	if (!validate()) {
		return;
	}

	if(millis() - last_paste_time > debounce_delay) {
		last_paste_time = millis();
	} else { return; }
	

	Serial.print("paste: \n");

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


void setup()
{
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
	attachInterrupt(digitalPinToInterrupt(MOVE_1_BUTTONX), onMoveXPlayer1, CHANGE);
	attachInterrupt(digitalPinToInterrupt(MOVE_1_BUTTONY), onMoveYPlayer1, CHANGE);
	attachInterrupt(digitalPinToInterrupt(MOVE_1_BUTTON_ORIENTATION), 
								 onChnageOrientationPlayer1, CHANGE);
	attachInterrupt(digitalPinToInterrupt(MOVE_1_DROP_SHIP), onPastePlayer1, CHANGE);


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
	snprintf(game_id, 14, "g Andu ID %d", modulus);
	tft.drawString(game_id, 30, 30, 2);


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


	tft_draw_ships();
}


void loop()
{

}
