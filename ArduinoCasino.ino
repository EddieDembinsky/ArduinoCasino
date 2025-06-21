#include <LiquidCrystal.h>
#include <EEPROM.h>

// LCD Configuration (16-pin parallel connection)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Pin Definitions
#define JOYSTICK_VRX A0
#define JOYSTICK_VRY A1
#define JOYSTICK_SW 6
#define BUTTON_SELECT 7
#define BUTTON_BACK 8
#define BUTTON_ACTION1 9
#define BUTTON_ACTION2 10

// EEPROM addresses for persistent storage
#define EEPROM_BALANCE_ADDR 0
#define EEPROM_PLAYER_WINS_ADDR 4
#define EEPROM_DEALER_WINS_ADDR 8
#define EEPROM_PLAYER_BJ_ADDR 12
#define EEPROM_DEALER_BJ_ADDR 16

// Game States
enum GameState {
  WELCOME_SCREEN,
  MAIN_MENU,
  BALANCE_MENU,
  BET_SCREEN,
  BLACKJACK_GAME,
  POKER_GAME,
  SLOTS_GAME,
  ROULETTE_GAME,
  GAME_RESULT
};

// Menu Options
enum MenuOption {
  MENU_BLACKJACK = 0,
  MENU_POKER = 1,
  MENU_SLOTS = 2,
  MENU_ROULETTE = 3,
  MENU_BALANCE = 4,
  MENU_COUNT = 5
};

// Blackjack Game States
enum BlackjackState {
  BJ_BETTING,
  BJ_INITIAL_DEAL,
  BJ_PLAYER_TURN,
  BJ_ACE_CHOICE,
  BJ_DEALER_REVEAL,
  BJ_DEALER_TURN,
  BJ_GAME_OVER
};

// Card Values (for blackjack)
#define JACK 101
#define QUEEN 102
#define KING 103
#define ACE 104

// Global Variables
GameState currentState = WELCOME_SCREEN;
GameState previousState = WELCOME_SCREEN;
MenuOption selectedMenu = MENU_BLACKJACK;
BlackjackState bjState = BJ_BETTING;

// Menu strings
const char* menuItems[] = {
  "Blackjack",
  "Poker", 
  "Slots",
  "Roulette",
  "Balance"
};

// Button debouncing
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

// Joystick variables
int joystickCenter = 512;
int joystickThreshold = 100;
unsigned long lastJoystickMove = 0;

// Balance and betting system
long balance = 1000; // Default starting balance
long currentBet = 0;
long betAmount = 10; // Default bet amount

// Statistics
int playerWins = 0;
int dealerWins = 0; 
int playerBlackjacks = 0;
int dealerBlackjacks = 0;

// Blackjack Game Variables
int playerCards[6];  // Store actual card values
int dealerCards[6];
byte playerCount = 0;
byte dealerCount = 0;
int playerTotal = 0;
int dealerTotal = 0;
int dealerShown = 0;  // First dealer card shown
byte aceCardIndex = 0; // For ace choice
bool gameOver = false;
bool playerBlackjack = false;
bool dealerBlackjack = false;
bool playerBust = false;
bool dealerBust = false;

// Dealer turn timing
unsigned long dealerActionTime = 0;
unsigned long gameResultTime = 0;
byte dealerStep = 0;

// Display variables
unsigned long lastDisplay = 0;
bool needsRedraw = true;

// Slot Machine Variables
const char* slotSymbols[] = {"7", "BAR", "CH", "BELL", "STAR", "HEART"};
const int slotSymbolCount = 6;
int slotReels[3] = {0, 0, 0};
bool slotSpinning = false;
unsigned long slotSpinStart = 0;
const unsigned long slotSpinDuration = 1200; // ms
long slotBet = 10; // Default slot bet

// --- Roulette Game Variables ---
enum RouletteBetType { BET_SINGLE, BET_RED, BET_BLACK, BET_ODD, BET_EVEN, BET_COUNT };
const char* rouletteBetNames[BET_COUNT] = { "Single", "Red", "Black", "Odd", "Even" };
int rouletteBetType = 0;
bool rouletteSpinning = false;
int rouletteBetAmount = 10;
int rouletteBetNumber = 0; // For single number bet

// Helper: Check if a number is red in roulette
bool isRed(int n) {
    int reds[] = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36};
    for (int i = 0; i < 18; i++) if (n == reds[i]) return true;
    return false;
}


// Custom LCD symbols (for slots)
byte bellChar[8] = {
  0b00100,
  0b01110,
  0b01110,
  0b01110,
  0b11111,
  0b00100,
  0b00000,
  0b00100
};

byte starChar[8] = {
  0b00100,
  0b10101,
  0b01110,
  0b11111,
  0b01110,
  0b10101,
  0b00100,
  0b00000
};

byte heartChar[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

byte leftArrowChar[8] = {
  0b00010,
  0b00110,
  0b01110,
  0b11110,
  0b01110,
  0b00110,
  0b00010,
  0b00000
};

byte rightArrowChar[8] = {
  0b01000,
  0b01100,
  0b01110,
  0b01111,
  0b01110,
  0b01100,
  0b01000,
  0b00000
};

void waitForRelease(){
  while (digitalRead(BUTTON_SELECT) == LOW || digitalRead(JOYSTICK_SW) == LOW) {
        delay(10);
    }
}

void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.createChar(0, bellChar);   // Custom char 0: Bell
  lcd.createChar(1, starChar);   // Custom char 1: Star
  lcd.createChar(2, heartChar);  // Custom char 2: Heart
  lcd.createChar(3, leftArrowChar);  // Custom char 3: Left Arrow
  lcd.createChar(4, rightArrowChar); // Custom char 4: Right Arrow

  // Initialize pins
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_BACK, INPUT_PULLUP);
  pinMode(BUTTON_ACTION1, INPUT_PULLUP);
  pinMode(BUTTON_ACTION2, INPUT_PULLUP);
  pinMode(JOYSTICK_SW, INPUT_PULLUP);
  
  // Load saved data from EEPROM
  loadBalance();
  loadStatistics();
  
  // Seed random number generator with analog noise
  randomSeed(analogRead(A2) + analogRead(A3));
  
  // Show welcome screen
  showWelcomeScreen();
  delay(2000);
  currentState = MAIN_MENU;
  needsRedraw = true;
}

void loop() {
  handleInput();
  
  switch (currentState) {
    case WELCOME_SCREEN:
      // Already handled in setup
      break;
      
    case MAIN_MENU:
      if (needsRedraw) {
        showMainMenu();
        needsRedraw = false;
      }
      break;
      
    case BALANCE_MENU:
      if (needsRedraw) {
        showBalanceMenu();
        needsRedraw = false;
      }
      break;
      
    case BET_SCREEN:
      if (needsRedraw) {
        showBetScreen();
        needsRedraw = false;
      }
      break;
      
    case BLACKJACK_GAME:
      playBlackjack();
      break;
      
    case GAME_RESULT:
      if (needsRedraw) {
        showGameResult();
        needsRedraw = false;
      }
      break;
      
    case POKER_GAME:
      playPoker();
      break;
      
    case SLOTS_GAME:
      playSlots();
      break;
      
    case ROULETTE_GAME:
      playRoulette();
      break;
  }
  
  delay(50); // Small delay to prevent excessive polling
}

void handleInput() {
  unsigned long currentTime = millis();
  
  // Handle joystick movement (menu navigation)
  if ((currentState == MAIN_MENU || currentState == BET_SCREEN) && 
      currentTime - lastJoystickMove > 300) {
    int xValue = analogRead(JOYSTICK_VRX);
    int yValue = analogRead(JOYSTICK_VRY);
    
    if (yValue < joystickCenter - joystickThreshold) {
      // Joystick moved up
      if (currentState == MAIN_MENU) {
        selectedMenu = (MenuOption)((selectedMenu - 1 + MENU_COUNT) % MENU_COUNT);
        needsRedraw = true;
      }lastJoystickMove = currentTime;
    } else if (yValue > joystickCenter + joystickThreshold) {
      // Joystick moved down
      if (currentState == MAIN_MENU) {
        selectedMenu = (MenuOption)((selectedMenu + 1) % MENU_COUNT);
        needsRedraw = true;
      }
      lastJoystickMove = currentTime;
    }

    // Use left/right for bet adjustment
    if (currentState == BET_SCREEN) {
      if (xValue > joystickCenter + joystickThreshold) {
        betAmount = min(betAmount + 10, balance);
        needsRedraw = true;
        lastJoystickMove = currentTime;
      } else if (xValue < joystickCenter - joystickThreshold) {
        betAmount = max(betAmount - 10, 10L);
        needsRedraw = true;
        lastJoystickMove = currentTime;
      }
    }
  }
  
  // Handle button presses
  if (currentTime - lastButtonPress > debounceDelay) {
    if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(JOYSTICK_SW) == LOW) {
      handleSelectButton();
      lastButtonPress = currentTime;
    }
    
    if (digitalRead(BUTTON_BACK) == LOW) {
      handleBackButton();
      lastButtonPress = currentTime;
    }
    
    if (digitalRead(BUTTON_ACTION1) == LOW) {
      handleAction1Button();
      lastButtonPress = currentTime;
    }
    
    if (digitalRead(BUTTON_ACTION2) == LOW) {
      handleAction2Button();
      lastButtonPress = currentTime;
    }
  }
}

void handleSelectButton() {
  if (currentState == MAIN_MENU) {
    switch (selectedMenu) {
      case MENU_BLACKJACK:
        if (balance >= 10) {
          currentState = BET_SCREEN;
          betAmount = min(50L, balance); // Default bet
          needsRedraw = true;
        } else {
          showInsufficientFunds();
        }
        break;
      case MENU_BALANCE:
        currentState = BALANCE_MENU;
        needsRedraw = true;
        break;
      case MENU_POKER:
        currentState = POKER_GAME;
        break;
      case MENU_SLOTS:
        slotBet = min(50L, balance);
        slotSpinning = false;
        currentState = SLOTS_GAME;
        needsRedraw = true;
        break;
      case MENU_ROULETTE:
        currentState = ROULETTE_GAME;
        break;
    }
  } else if (currentState == BET_SCREEN) {
    // Place bet and start game
    currentBet = betAmount;
    balance -= currentBet;
    saveBalance();
    currentState = BLACKJACK_GAME;
    initBlackjack();
  } else if (currentState == GAME_RESULT) {
    currentState = MAIN_MENU;
    needsRedraw = true;
  }
}

void handleBackButton() {
  if (currentState == BALANCE_MENU || currentState == BET_SCREEN) {
    currentState = MAIN_MENU;
    selectedMenu = MENU_BLACKJACK;
    needsRedraw = true;
  } else if (currentState == GAME_RESULT) {
    currentState = MAIN_MENU;
    needsRedraw = true;
  } else if (currentState != MAIN_MENU && currentState != WELCOME_SCREEN) {
    currentState = MAIN_MENU;
    selectedMenu = MENU_BLACKJACK;
    needsRedraw = true;
  }
}

void handleAction1Button() {
  if (currentState == BLACKJACK_GAME) {
    if (bjState == BJ_PLAYER_TURN) {
      // Hit button
      hitPlayer();
    } else if (bjState == BJ_ACE_CHOICE) {
      // Choose Ace as 11
      chooseAce(11);
    }
  }
}

void handleAction2Button() {
  if (currentState == BLACKJACK_GAME) {
    if (bjState == BJ_PLAYER_TURN) {
      // Stand button
      stand();
    } else if (bjState == BJ_ACE_CHOICE) {
      // Choose Ace as 1
      chooseAce(1);
    }
  }
}

void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to");
  lcd.setCursor(0, 1);
  lcd.print("Arduino Casino!");
}

void showMainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Game:");
  lcd.setCursor(0, 1);
  lcd.print(">" + String(menuItems[selectedMenu]));
}

void showBalanceMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Balance: $" + String(balance));
  lcd.setCursor(0, 1);
  lcd.print("W:" + String(playerWins) + " L:" + String(dealerWins) + " BJ:" + String(playerBlackjacks));
}

void showBetScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(3)); // left arrow
  lcd.print("Bet: $" + String(betAmount) + " ");
  lcd.write(byte(4)); // right arrow
  lcd.setCursor(0, 1);
  lcd.print("Up/Down: Amount");
}

void showInsufficientFunds() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Insufficient");
  lcd.setCursor(0, 1);
  lcd.print("Funds!");
  delay(2000);
  currentState = MAIN_MENU;
  needsRedraw = true;
}

// ==================== PERSISTENCE FUNCTIONS ====================

void saveBalance() {
  EEPROM.put(EEPROM_BALANCE_ADDR, balance);
}

void loadBalance() {
  EEPROM.get(EEPROM_BALANCE_ADDR, balance);
  if (balance < 0 || balance > 1000000) {
    balance = 1000; // Reset to default if invalid
    saveBalance();
  }
}

void saveStatistics() {
  EEPROM.put(EEPROM_PLAYER_WINS_ADDR, playerWins);
  EEPROM.put(EEPROM_DEALER_WINS_ADDR, dealerWins);
  EEPROM.put(EEPROM_PLAYER_BJ_ADDR, playerBlackjacks);
  EEPROM.put(EEPROM_DEALER_BJ_ADDR, dealerBlackjacks);
}

void loadStatistics() {
  EEPROM.get(EEPROM_PLAYER_WINS_ADDR, playerWins);
  EEPROM.get(EEPROM_DEALER_WINS_ADDR, dealerWins);
  EEPROM.get(EEPROM_PLAYER_BJ_ADDR, playerBlackjacks);
  EEPROM.get(EEPROM_DEALER_BJ_ADDR, dealerBlackjacks);
  
  // Validate loaded data
  if (playerWins < 0 || playerWins > 10000) playerWins = 0;
  if (dealerWins < 0 || dealerWins > 10000) dealerWins = 0;
  if (playerBlackjacks < 0 || playerBlackjacks > 1000) playerBlackjacks = 0;
  if (dealerBlackjacks < 0 || dealerBlackjacks > 1000) dealerBlackjacks = 0;
}

// ==================== BLACKJACK GAME ====================

// Card generation
int getCard() {
  int randNum = random(2, 15);   // card range: 2–14
  
  switch(randNum) {
    case 11: return JACK;
    case 12: return QUEEN;
    case 13: return KING;
    case 14: return ACE;
    default: return randNum;
  }
}

void initBlackjack() {
  // Reset game variables
  playerCount = 0;
  dealerCount = 0;
  playerTotal = 0;
  dealerTotal = 0;
  gameOver = false;
  playerBlackjack = false;
  dealerBlackjack = false;
  playerBust = false;
  dealerBust = false;
  dealerStep = 0;
  bjState = BJ_INITIAL_DEAL;
  
  // Deal initial cards
  playerCards[0] = getCard();
  dealerCards[0] = getCard();
  playerCards[1] = getCard();
  dealerCards[1] = getCard();
  
  playerCount = 2;
  dealerCount = 2;
  
  // Calculate dealer's shown card
  dealerShown = getCardDisplayValue(dealerCards[0]);
  
  // Check for aces in initial player hand
  for (byte i = 0; i < 2; i++) {
    if (playerCards[i] == ACE) {
      aceCardIndex = i;
      bjState = BJ_ACE_CHOICE;
      return;
    }
  }
  
  calculatePlayerTotal();
  
  // Check for player blackjack
  if (playerTotal == 21) {
    playerBlackjack = true;
    bjState = BJ_DEALER_REVEAL;
    dealerActionTime = millis();
  } else {
    bjState = BJ_PLAYER_TURN;
  }
  
  needsRedraw = true;
}

void playBlackjack() {
  while (digitalRead(BUTTON_SELECT) == LOW || digitalRead(JOYSTICK_SW) == LOW) {
      delay(10);
  }

  
  switch (bjState) {
    case BJ_INITIAL_DEAL:
      displayBlackjackGame();
      break;
      
    case BJ_PLAYER_TURN:
      displayBlackjackGame();
      break;
      
    case BJ_ACE_CHOICE:
      displayAceChoice();
      break;
      
    case BJ_DEALER_REVEAL:
      dealerReveal();
      break;
      
    case BJ_DEALER_TURN:
      playDealerTurn();
      break;
      
    case BJ_GAME_OVER:
      finishGame();
      break;
  }
}

void hitPlayer() {
  if (playerCount < 6) {
    int newCard = getCard();
    playerCards[playerCount] = newCard;
    playerCount++;
    
    // Handle ace
    if (newCard == ACE) {
      aceCardIndex = playerCount - 1;
      bjState = BJ_ACE_CHOICE;
      return;
    }
    
    calculatePlayerTotal();
    needsRedraw = true;
    
    // Check bust
    if (playerTotal > 21) {
      playerBust = true;
      bjState = BJ_GAME_OVER;
    }
  }
}

void stand() {
  bjState = BJ_DEALER_REVEAL;
  dealerActionTime = millis();
  dealerStep = 0;
}

void chooseAce(int value) {
  playerCards[aceCardIndex] = value;
  calculatePlayerTotal();
  needsRedraw = true;
  
  if (playerTotal > 21) {
    playerBust = true;
    bjState = BJ_GAME_OVER;
  } else if (playerTotal == 21 && playerCount == 2) {
    // Blackjack
    playerBlackjack = true;
    bjState = BJ_DEALER_REVEAL;
    dealerActionTime = millis();
  } else {
    bjState = BJ_PLAYER_TURN;
  }
}

void calculatePlayerTotal() {
  playerTotal = 0;
  for (byte i = 0; i < playerCount; i++) {
    playerTotal += getCardValue(playerCards[i]);
  }
}

void calculateDealerTotal() {
  dealerTotal = 0;
  int aces = 0;
  
  // Count cards and aces
  for (byte i = 0; i < dealerCount; i++) {
    int card = dealerCards[i];
    if (card == ACE) {
      aces++;
      dealerTotal += 11;
    } else {
      dealerTotal += getCardValue(card);
    }
  }
  
  // Adjust aces if over 21
  while (dealerTotal > 21 && aces > 0) {
    dealerTotal -= 10;
    aces--;
  }
}

int getCardValue(int card) {
  switch(card) {
    case JACK:
    case QUEEN:
    case KING:
      return 10;
    case ACE:
      return 11; // Default, adjusted elsewhere
    default:
      return card;
  }
}

int getCardDisplayValue(int card) {
  switch(card) {
    case JACK:
    case QUEEN:
    case KING:
      return 10;
    case ACE:
      return 11;
    default:
      return card;
  }
}

void displayBlackjackGame() {
  if (!needsRedraw) return;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P:" + String(playerTotal) + " D:" + String(dealerShown) + " $" + String(currentBet));
  lcd.setCursor(0, 1);
  
  if (playerBlackjack) {
    lcd.print("BLACKJACK!");
  } else {
    lcd.print("Hit(3) Stand(4)");
  }
  
  needsRedraw = false;
}

void displayAceChoice() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("You drew an ACE");
  lcd.setCursor(0, 1);
  lcd.print("11(3) or 1(4)?");
}

void dealerReveal() {
  calculateDealerTotal();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dealer has " + String(dealerTotal));
  lcd.setCursor(0, 1);
  
  if (dealerTotal == 21 && dealerCount == 2) {
    dealerBlackjack = true;
    lcd.print("Dealer Blackjack!");
  } else if (dealerTotal < 17) {
    lcd.print("Dealer must hit");
  } else {
    lcd.print("Dealer stands");
  }
  
  delay(2000);
  
  if (dealerTotal < 17) {
    bjState = BJ_DEALER_TURN;
    dealerActionTime = millis();
    dealerStep = 0;
  } else {
    bjState = BJ_GAME_OVER;
  }
}

void playDealerTurn() {
  unsigned long now = millis();
  
  if (now - dealerActionTime < 1500) return; // Wait between actions
  
  calculateDealerTotal();
  
  if (dealerTotal < 17 && dealerCount < 6) {
    // Dealer must hit
    int newCard = getCard();
    dealerCards[dealerCount] = newCard;
    dealerCount++;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dealer drew ");
    printCardName(newCard);
    
    calculateDealerTotal();
    lcd.setCursor(0, 1);
    lcd.print("Total: " + String(dealerTotal));
    
    dealerActionTime = now;
    
    if (dealerTotal >= 17 || dealerTotal > 21) {
      delay(1500);
      bjState = BJ_GAME_OVER;
    }
  } else {
    bjState = BJ_GAME_OVER;
  }
}

void printCardName(int card) {
  switch(card) {
    case JACK: lcd.print("J"); break;
    case QUEEN: lcd.print("Q"); break;
    case KING: lcd.print("K"); break;
    case ACE: lcd.print("A"); break;
    default: lcd.print(card); break;
  }
}

void finishGame() {
  // Determine winner and update balance/stats
  if (playerBlackjack && dealerBlackjack) {
    // Push - both blackjack
    balance += currentBet;
  } else if (playerBlackjack) {
    // Player blackjack wins
    balance += (long)(currentBet * 2.5);
    playerWins++;
    playerBlackjacks++;
  } else if (dealerBlackjack) {
    // Dealer blackjack wins
    dealerWins++;
    dealerBlackjacks++;
  } else if (playerBust) {
    // Player bust - dealer wins
    dealerWins++;
  } else if (dealerTotal > 21) {
    // Dealer bust - player wins
    balance += currentBet * 2;
    playerWins++;
  } else if (playerTotal > dealerTotal) {
    // Player wins
    balance += currentBet * 2;
    playerWins++;
  } else if (dealerTotal > playerTotal) {
    // Dealer wins
    dealerWins++;
  } else {
    // Push - tie
    balance += currentBet;
  }
  
  saveBalance();
  saveStatistics();
  
  currentState = GAME_RESULT;
  gameResultTime = millis();
  needsRedraw = true;
}

void showGameResult() {
  lcd.clear();
  lcd.setCursor(0, 0);
  
  if (playerBlackjack && dealerBlackjack) {
    lcd.print("Both BJ - Push");
  } else if (playerBlackjack) {
    lcd.print("BLACKJACK WIN!");
  } else if (dealerBlackjack) {
    lcd.print("Dealer BJ");
  } else if (playerBust) {
    lcd.print("Bust - You Lose");
  } else if (dealerTotal > 21) {
    lcd.print("Dealer Bust-Win!");
  } else if (playerTotal > dealerTotal) {
    lcd.print("You Win!");
  } else if (dealerTotal > playerTotal) {
    lcd.print("Dealer Wins");
  } else {
    lcd.print("Push - Tie");
  }
  
  lcd.setCursor(0, 1);
  lcd.print("$" + String(balance) + " Select:Menu");
  
  // Auto return to menu after 5 seconds
  if (millis() - gameResultTime > 5000) {
    currentState = MAIN_MENU;
    needsRedraw = true;
  }
}

// ==================== PLACEHOLDER POKER ====================

void playPoker() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Poker Game");
  lcd.setCursor(0, 1);
  lcd.print("Coming Soon!");
  
  delay(2000);
  currentState = MAIN_MENU;
  needsRedraw = true;
}

void playSlots() { // ------------------------------------------------------------------------------------------------------------------------------------------------
  waitForRelease();
  if (!slotSpinning) {
    // Show slot bet screen
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(byte(3)); // Left arrow
    lcd.print("Slot Bet: $" + String(slotBet) + " ");
    lcd.write(byte(4)); // Right arrow
    lcd.setCursor(0, 1);
    lcd.print("Dn:Spin Up/Down:Bet");

    // Joystick left/right to change bet
    int xValue = analogRead(JOYSTICK_VRX);
    if (xValue > joystickCenter + joystickThreshold) {
      slotBet = min(slotBet + 10, balance);
      delay(200);
    } else if (xValue < joystickCenter - joystickThreshold) {
      slotBet = max(slotBet - 10, 10L);
      delay(200);
    }

    // Pull joystick down to spin
    int yValue = analogRead(JOYSTICK_VRY);
    if (yValue > joystickCenter + joystickThreshold + 200 && balance >= slotBet) {
      balance -= slotBet;
      saveBalance();
      slotSpinning = true;
      slotSpinStart = millis();
      // Randomize initial reels
      for (int i = 0; i < 3; i++) {
        slotReels[i] = random(slotSymbolCount);
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Spinning...");
      delay(200);
    } else if (yValue > joystickCenter + joystickThreshold + 200 && balance < slotBet) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Not enough $!");
      delay(1000);
      currentState = MAIN_MENU;
      needsRedraw = true;
    }
    return;
  }

  // Animation: spin reels for slotSpinDuration ms
  if (slotSpinning) {
    unsigned long now = millis();
    if (now - slotSpinStart < slotSpinDuration) {
      for (int i = 0; i < 3; i++) {
        slotReels[i] = random(slotSymbolCount);
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      for (int i = 0; i < 3; i++) {
        if (slotReels[i] == 0) lcd.print("7");
        else if (slotReels[i] == 1) { lcd.print("B"); lcd.print("A"); lcd.print("R"); }
        else if (slotReels[i] == 2) lcd.print("CH");
        else if (slotReels[i] == 3) lcd.write(byte(0));
        else if (slotReels[i] == 4) lcd.write(byte(1));
        else if (slotReels[i] == 5) lcd.write(byte(2));
        if (i < 2) lcd.setCursor(5 * (i + 1), 0);
      }
      lcd.setCursor(0, 1);
      lcd.print("Spinning...");
      delay(120);
    } else {
      // Stop spinning, show result
      for (int i = 0; i < 3; i++) {
        slotReels[i] = random(slotSymbolCount);
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      for (int i = 0; i < 3; i++) {
        if (slotReels[i] == 0) lcd.print("7");
        else if (slotReels[i] == 1) { lcd.print("B"); lcd.print("A"); lcd.print("R"); }
        else if (slotReels[i] == 2) lcd.print("CH");
        else if (slotReels[i] == 3) lcd.write(byte(0));
        else if (slotReels[i] == 4) lcd.write(byte(1));
        else if (slotReels[i] == 5) lcd.write(byte(2));
        if (i < 2) lcd.setCursor(5 * (i + 1), 0);
      }
      lcd.setCursor(0, 1);

      // Determine payout
      int payout = 0;
      if (slotReels[0] == slotReels[1] && slotReels[1] == slotReels[2]) {
        // Three of a kind
        if (slotReels[0] == 0) payout = slotBet * 10; // 7
        else if (slotReels[0] == 1) payout = slotBet * 5; // BAR
        else payout = slotBet * 3; // Other symbols
        lcd.print("JACKPOT! +$" + String(payout));
      } else if (slotReels[0] == slotReels[1] || slotReels[1] == slotReels[2] || slotReels[0] == slotReels[2]) {
        payout = slotBet * 2;
        lcd.print("Pair! +$" + String(payout));
      } else {
        lcd.print("No win");
      }

      if (payout > 0) {
        balance += payout;
        saveBalance();
      }

      delay(2000);
      slotSpinning = false;
      currentState = MAIN_MENU;
      needsRedraw = true;
    }
  }
}

int rouletteStart = 1;
// --- Main Roulette Game Function ---
void playRoulette() { // --------------------------------------------------------------------------------------------------------------------------------------------------
    // --- 1. Bet Amount Selection ---
    waitForRelease();

    if (rouletteSpinning == false) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write(byte(3)); // Left arrow
        lcd.print(" Roulette: $" + String(rouletteBetAmount) + " ");
        lcd.write(byte(4)); // Right arrow
        lcd.setCursor(0, 1);
        lcd.print("L/R:Bet Sel:Spin");

        int xValue = analogRead(JOYSTICK_VRX);
        if (xValue > joystickCenter + joystickThreshold) {
            rouletteBetAmount = min(rouletteBetAmount + 10, balance);
            lcd.setCursor(12, 0);
            lcd.print("     ");
            lcd.setCursor(12, 0);
            lcd.print("$" + String(rouletteBetAmount));
            delay(200);
        } else if (xValue < joystickCenter - joystickThreshold) {
            rouletteBetAmount = max(rouletteBetAmount - 10, 10L);
            lcd.setCursor(12, 0);
            lcd.print("     ");
            lcd.setCursor(12, 0);
            lcd.print("$" + String(rouletteBetAmount));
            delay(200);
        }
        // Start spin if select or joystick pressed
        if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(JOYSTICK_SW) == LOW) {
            delay(250);
            rouletteSpinning = true;
        }
        if (digitalRead(BUTTON_BACK) == LOW) {
            delay(250);
            currentState = MAIN_MENU;
            needsRedraw = true;
            return;
        }
    }

    // --- 2. Bet Type Selection ---
    if (rouletteSpinning == true){
      rouletteBetType = 0;
      while (true) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Type: ");
          lcd.print(rouletteBetNames[rouletteBetType]);
          lcd.setCursor(0, 1);
          lcd.print("Up/Dn:Type Sel:OK");

          bool typeChosen = false;
          while (!typeChosen) {
              int yValue = analogRead(JOYSTICK_VRY);
              if (yValue < joystickCenter - joystickThreshold) {
                  rouletteBetType = (rouletteBetType + BET_COUNT - 1) % BET_COUNT;
                  lcd.setCursor(6, 0);
                  lcd.print("         ");
                  lcd.setCursor(6, 0);
                  lcd.print(rouletteBetNames[rouletteBetType]);
                  delay(200);
              } else if (yValue > joystickCenter + joystickThreshold) {
                  rouletteBetType = (rouletteBetType + 1) % BET_COUNT;
                  lcd.setCursor(6, 0);
                  lcd.print("         ");
                  lcd.setCursor(6, 0);
                  lcd.print(rouletteBetNames[rouletteBetType]);
                  delay(200);
              }
              if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(JOYSTICK_SW) == LOW) {
                  delay(250);
                  typeChosen = true;
              }
              if (digitalRead(BUTTON_BACK) == LOW) {
                  delay(250);
                  currentState = MAIN_MENU;
                  needsRedraw = true;
                  return;
              }
          }
          break;
      }

      // --- 3. Single Number Selection (if needed) ---
      if (rouletteBetType == BET_SINGLE) {
          rouletteBetNumber = 0;
          while (true) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Number: ");
              lcd.print(rouletteBetNumber);
              lcd.setCursor(0, 1);
              lcd.print("L/R:Num Sel:OK");

              bool numChosen = false;
              while (!numChosen) {
                  int xValue = analogRead(JOYSTICK_VRX);
                  if (xValue > joystickCenter + joystickThreshold) {
                      rouletteBetNumber = (rouletteBetNumber + 1) % 37;
                      lcd.setCursor(8, 0);
                      lcd.print("   ");
                      lcd.setCursor(8, 0);
                      lcd.print(rouletteBetNumber);
                      delay(150);
                  } else if (xValue < joystickCenter - joystickThreshold) {
                      rouletteBetNumber = (rouletteBetNumber + 36) % 37;
                      lcd.setCursor(8, 0);
                      lcd.print("   ");
                      lcd.setCursor(8, 0);
                      lcd.print(rouletteBetNumber);
                      delay(150);
                  }
                  if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(JOYSTICK_SW) == LOW) {
                      delay(250);
                      numChosen = true;
                  }
                  if (digitalRead(BUTTON_BACK) == LOW) {
                      delay(250);
                      currentState = MAIN_MENU;
                      needsRedraw = true;
                      return;
                  }
              }
              break;
          }
      }

      // --- 4. Deduct Bet and Spin Animation ---
      balance -= rouletteBetAmount;
      saveBalance();

      unsigned long spinStart = millis();
      int spinResult = 0;
      int spinColor = 2; // 0: Red, 1: Black, 2: Green

      while (millis() - spinStart < 2000) {
          int fakeNum = random(0, 37);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Spinning...");
          lcd.setCursor(0, 1);
          lcd.print("Ball: ");
          if (fakeNum == 0) {
              lcd.print("0 (Green)");
          } else {
              lcd.print(String(fakeNum));
              lcd.print(isRed(fakeNum) ? " (Red)" : " (Blk)");
          }
          delay(80);
      }

      // --- 5. Final Result ---
      spinResult = random(0, 37);
      if (spinResult == 0) spinColor = 2;
      else if (isRed(spinResult)) spinColor = 0;
      else spinColor = 1;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Result: ");
      if (spinResult == 0) lcd.print("0 (Green)");
      else {
          lcd.print(String(spinResult));
          lcd.print(spinColor == 0 ? " (Red)" : " (Blk)");
      }
      lcd.setCursor(0, 1);

      // --- 6. Payout Calculation ---
      bool win = false;
      int payout = 0;
      switch (rouletteBetType) {
          case BET_SINGLE:
              if (spinResult == rouletteBetNumber) { win = true; payout = rouletteBetAmount * 35; }
              break;
          case BET_RED:
              if (spinColor == 0) { win = true; payout = rouletteBetAmount * 2; }
              break;
          case BET_BLACK:
              if (spinColor == 1) { win = true; payout = rouletteBetAmount * 2; }
              break;
          case BET_ODD:
              if (spinResult != 0 && (spinResult % 2 == 1)) { win = true; payout = rouletteBetAmount * 2; }
              break;
          case BET_EVEN:
              if (spinResult != 0 && (spinResult % 2 == 0)) { win = true; payout = rouletteBetAmount * 2; }
              break;
      }

      if (win) {
          lcd.print("You win! +$");
          lcd.print(payout);
          balance += payout;
          saveBalance();
      } else {
          lcd.print("You lose!");
      }

      rouletteStart = 1;
      rouletteSpinning = false;

      delay(2500);
      // Always return to main menu after game ends
      currentState = MAIN_MENU;
      needsRedraw = true;
      return;
  }
}
