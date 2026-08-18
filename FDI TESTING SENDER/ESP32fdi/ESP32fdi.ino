// P8 LED DOT MATRIX PROGRAM

#define COMP "Restek"
#define SW_VERSION "v1.0.0"  

/* Use a custom Virtual Display class to re-map co-ordinates such that they draw
   correctly on a 32x16 1/4 Scan panel (or chain of such panels).
*/
#include "ESP32-VirtualMatrixPanel-I2S-DMA.h"

/* ================================================== */
// Panel configuration
#define PANEL_RES_X 32 // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 16 // Number of pixels tall of each INDIVIDUAL panel module.


#define NUM_ROWS 1 // Number of rows of chained INDIVIDUAL PANELS
#define NUM_COLS 3 // Number of INDIVIDUAL PANELS per ROW
#define CHAIN_PANEL NUM_ROWS*NUM_COLS

// ^^^ NOTE: DEFAULT EXAMPLE SETUP IS FOR A CHAIN OF TWO x 1/8 SCAN PANELS

// Change this to your needs, for details on VirtualPanel pls read the PDF!
#define SERPENT true
#define TOPDOWN false
#define VIRTUAL_MATRIX_CHAIN_TYPE CHAIN_TOP_RIGHT_DOWN

// placeholder for the matrix object
MatrixPanel_I2S_DMA *dma_display = nullptr;

// placeholder for the virtual display object
EightPxBasePanel   *FourScanPanel = nullptr;

int delayBetweeenAnimations = 40; // How fast it scrolls, Smaller == faster
int scrollXMove = -1; //If positive it would scroll right

int textSize = 2; // adjust text size if needed

int textXPosition = PANEL_RES_X * CHAIN_PANEL;    // Number of pixels wide of each INDIVIDUAL panel module.
// Will start one pixel off screen
int textYPosition = ((PANEL_RES_Y - textSize * 8) / 2); // This will center the text

String text ="PT KERETA API INDONESIA (PERSERO)";

// For scrolling Text
unsigned long isAnimationDue;

// Will be used in getTextBounds.
int16_t xOne, yOne;
uint16_t w, h;

enum FontType {
    SMALL = 1,
    LARGE = 2
};

enum TextColor { HITAM, MERAH, HIJAU, BIRU, ORANYE, PINK, AMBER, PUTIH };

uint16_t myBLACK, myWHITE, myRED, myGREEN, myBLUE, myORANGE, myPINK, myAMBER;
uint16_t currentColor = myRED;

enum AnimationType {
    SCROLL_LEFT = 0,
    SCROLL_UP = 1,
    SCROLL_DOWN = 2,
    BLINK = 3,
    DOWN_BLINK = 4,
    STATIC = 5,
    SPECIAL_SCROLL = 6,
    STATIC_LARGE = 7,
};


FontType currentFontType = LARGE;
AnimationType currentAnimationType = SCROLL_LEFT;

unsigned long lastBlinkTime = 0;
bool displayText = true;

char receivedMessage[256];
bool newMessageAvailable = false;

// Declaration pin for receive data from atmega328p
#define RXp2 16
#define TXp2 17

/******************************************************************************
   Setup!
 ******************************************************************************/
void setup()
{
  delay(250);

  Serial.begin(115200);
  Serial.println(""); Serial.println(""); Serial.println("");
  Serial.println("*****************************************************");
  Serial.println("*         1/4 Scan Panel Demonstration              *");
  Serial.println("*****************************************************");
  Serial2.begin(38400, SERIAL_8N1, RXp2, TXp2);

  /*
       // 62x32 1/8 Scan Panels don't have a D and E pin!

       HUB75_I2S_CFG::i2s_pins _pins = {
        R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN,
        A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
        LAT_PIN, OE_PIN, CLK_PIN
       };
  */
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X * 2,            // DO NOT CHANGE THIS
    PANEL_RES_Y / 2,            // DO NOT CHANGE THIS
    CHAIN_PANEL         // DO NOT CHANGE THIS
    //,_pins            // Uncomment to enable custom pins
  );

  mxconfig.clkphase = false; // Change this if you see pixels showing up shifted wrongly by one column the left or right.
  mxconfig.double_buff = true;

  //mxconfig.driver   = HUB75_I2S_CFG::FM6126A;     // in case that we use panels based on FM6126A chip, we can set it here before creating MatrixPanel_I2S_DMA object

  // OK, now we can create our matrix object
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  // let's adjust default brightness to about 75%
  dma_display->setBrightness8(100);    // range is 0-255, 0 - 0%, 255 - 100%

  // Allocate memory and start DMA display
  if ( not dma_display->begin() )
    Serial.println("****** !KABOOM! I2S memory allocation failed ***********");


  dma_display->clearScreen();
  delay(500);

  // create FourScanPanellay object based on our newly created dma_display object
  FourScanPanel = new EightPxBasePanel ((*dma_display), NUM_ROWS, NUM_COLS, PANEL_RES_X, PANEL_RES_Y, VIRTUAL_MATRIX_CHAIN_TYPE);

  myBLACK = dma_display->color565(0, 0, 0);
  myWHITE = dma_display->color565(255, 255, 255);
  myRED = dma_display->color565(255, 0, 0);
  myGREEN = dma_display->color565(0, 255, 0);
  myBLUE = dma_display->color565(0, 0, 255);
  myORANGE = dma_display->color565(255,165,0);
  myPINK = dma_display->color565(237,79,152);
  myAMBER = dma_display->color565(255,191,0);

  
  // THE IMPORTANT BIT BELOW!
  // FOUR_SCAN_16PX_HIGH
  // FOUR_SCAN_32PX_HIGH
// **** You don't need to set PhysicalPanelScanRate when use your own virtual panel class
  FourScanPanel->fillScreen(myBLACK);
  FourScanPanel->setTextSize(1);
  FourScanPanel->setTextWrap(false);
  FourScanPanel->setTextColor(myRED);
  
  FourScanPanel->setCursor(0,0);
  FourScanPanel->print(String(COMP)+"\n"+String(SW_VERSION));
  delay(500);
  dma_display->clearScreen();
  FourScanPanel->setTextSize(2);
  FourScanPanel->setTextColor(myRED);
}


// Test the pixel mapping - fill the panel pixel by pixel
void loop() {
    readSerial2();    

    if (newMessageAvailable) {
        
            // dma_display->flipDMABuffer();
        newMessageAvailable = false;
        FourScanPanel->setTextColor(currentColor);
        // Initialize positions for new message
        if (currentAnimationType == SCROLL_LEFT) {
            textXPosition = FourScanPanel->width();
            textYPosition = (FourScanPanel->height() - h) / 2;
        } else if (currentAnimationType == SCROLL_UP) {
            textXPosition = (FourScanPanel->width() - w) / 2;
            textYPosition = FourScanPanel->height();
        } else if (currentAnimationType == SCROLL_DOWN) {
            textXPosition = (FourScanPanel->width() - w) / 2;
            textYPosition = -h;
        } else if (currentAnimationType == DOWN_BLINK) {
            textXPosition = (FourScanPanel->width() - w) / 2;
            textYPosition = -h;
        } 
    }

    switch (currentAnimationType) {
        case SCROLL_LEFT:
            scrollTextLeft();
            break;
        case SCROLL_UP:
            scrollTextUp();
            break;
        case SCROLL_DOWN:
            scrollTextDown();
            break;
        case BLINK:
            blinkText();
            break;
        case DOWN_BLINK:
            downBlinkText();
            break;
        case STATIC:
            displayStaticText();
            break;
        case SPECIAL_SCROLL:
            specialScrollText();
            break;
        case STATIC_LARGE:
            displayLargeText();
            break;
        default:
            break;
    }
}

void clearScreenBuffer() {
    FourScanPanel->fillScreen(myBLACK);  // Mengisi layar dengan warna hitam pada buffer aktif
    dma_display->flipDMABuffer();  // Membalik buffer untuk menampilkan pembaruan
}

void readSerial2() {
    static char receivedMessage[256];
    static uint8_t index = 0;

    while (Serial2.available() > 0) {
        char c = Serial2.read();
        
        // Terima jika ada Newline (\n) atau Carriage Return (\r)
        if (c == '\n' || c == '\r') {
            if (index > 0) { // Pastikan ada isi pesan
                receivedMessage[index] = '\0';
                index = 0;
                parseMessage(String(receivedMessage));
            }
        } else {
            if (index < sizeof(receivedMessage) - 1) {
                receivedMessage[index++] = c;
            } else {
                index = 0; // Reset jika buffer penuh
            }
        }
    }
}

void parseMessage(const String& message) {
    // Validasi format $...*
    if (!message.startsWith("$") || !message.endsWith("*")) {
        return;
    }

    String receivedText = message.substring(1, message.length() - 1);

    text = receivedText;
    currentFontType = LARGE;
    currentColor = myRED;
    currentAnimationType = SCROLL_LEFT;

    FourScanPanel->setTextSize(1);
    FourScanPanel->setTextColor(currentColor);

    // Hitung dimensi teks baru
    FourScanPanel->getTextBounds(text, 0, 0, &xOne, &yOne, &w, &h);

    // Reset posisi teks untuk animasi berjalan dari kanan
    textXPosition = FourScanPanel->width();
    textYPosition = (FourScanPanel->height() - h) / 2;

    newMessageAvailable = true;
}

void scrollTextLeft() {
    unsigned long now = millis();
    if (now > isAnimationDue) {
        isAnimationDue = now + 30; // Kecepatan scroll

        // Bersihkan layar penuh pada buffer aktif
        FourScanPanel->fillScreen(myBLACK);

        // Geser posisi teks ke kiri
        textXPosition -= 1;
        
        // Reset posisi jika teks sudah berjalan habis keluar layar kiri
        if (textXPosition + w <= 0) {
            textXPosition = FourScanPanel->width();
        }

        // Gambar teks baru
        FourScanPanel->setCursor(textXPosition, textYPosition + 1);
        FourScanPanel->print(text);

        // Flip buffer ke panel LED
        dma_display->flipDMABuffer();
    }
}

void scrollTextUp() {
    unsigned long now = millis();
    if (now > isAnimationDue) {
        dma_display->flipDMABuffer();
        isAnimationDue = now + 40;
        FourScanPanel->fillRect(textXPosition, textYPosition, w, h, myBLACK);
        textYPosition -= 1;
        FourScanPanel->getTextBounds(text, textXPosition, textYPosition, &xOne, &yOne, &w, &h);
        if (textYPosition + h <= 0) {
            textYPosition = FourScanPanel->height();
        }
        FourScanPanel->setCursor(textXPosition, textYPosition);
        FourScanPanel->print(text);
    }
}

void scrollTextDown() {
    unsigned long now = millis();
    if (now > isAnimationDue) {
        isAnimationDue = now + 40;

        // Hapus area teks sebelumnya sepenuhnya
        FourScanPanel->fillRect(0, 0, FourScanPanel->width(), FourScanPanel->height(), myBLACK);
        
        // Update posisi Y
        textYPosition += 1;

        // Reset posisi teks jika sudah mencapai batas bawah
        if (textYPosition >= FourScanPanel->height()) {
            textYPosition = -h;
        }

        // Set posisi cursor baru dan cetak teks
        FourScanPanel->setCursor(textXPosition, textYPosition);
        FourScanPanel->print(text);
        
        // Flip buffer untuk memperbarui tampilan
        dma_display->flipDMABuffer();
    }
}


void blinkText() {
    unsigned long now = millis();
    if (now - lastBlinkTime >= 1000) {
        lastBlinkTime = now;
        displayText = !displayText;
        dma_display->flipDMABuffer();
        if (displayText) {
            FourScanPanel->fillRect(0, 0, FourScanPanel->width(), FourScanPanel->height(), myBLACK);
            FourScanPanel->setCursor((FourScanPanel->width() - w) / 2, ((FourScanPanel->height() - h) / 2)+ 1) ;
            FourScanPanel->print(text);
        } else {
            FourScanPanel->fillScreen(myBLACK);
        }
    }
}

void downBlinkText() {
    static bool scrollComplete = false; // Flag to track if scrolling is complete
    static unsigned long lastBlinkTime = 0;
    static unsigned long lastScrollTime = 0;
    int delayBetweenAnimations = 35;
    unsigned long now = millis();
    static int scrollYMove = 1;

    if (!scrollComplete) {
        if (now - lastScrollTime >= delayBetweenAnimations) {
            dma_display->flipDMABuffer();
            lastScrollTime = now;

            // Clear the area where the text was previously displayed
            FourScanPanel->fillRect(textXPosition, textYPosition, w, h, myBLACK);

            // Update the text position
            textYPosition += scrollYMove;

            // Check if the text is off-screen at the bottom
            if (textYPosition >= FourScanPanel->height()+1) {
                textYPosition = -h; // Reset position to the top
            }

            // Set cursor and print text
            FourScanPanel->setCursor(textXPosition, textYPosition);
            FourScanPanel->print(text);

            // Check if the text has completed scrolling down (i.e., if it's back at the top)
            if (textYPosition == -h) {
            // if (textYPosition == ((FourScanPanel->height() - h) / 2)+1) {
                scrollComplete = true;
                lastBlinkTime = now; // Reset blink timer
            }
        }
    } else {
        if (now - lastBlinkTime >= 1000) { // Blink interval (1000ms = 1s)
            lastBlinkTime = now;
            displayText = !displayText;

            dma_display->flipDMABuffer();

            if (displayText) {
                // Display the text
                FourScanPanel->fillRect(0, 0, FourScanPanel->width(), FourScanPanel->height(), myBLACK);
                FourScanPanel->setCursor((FourScanPanel->width() - w) / 2, ((FourScanPanel->height() - h) / 2)+1);
                FourScanPanel->print(text);
            } else {
                // Clear the screen
                FourScanPanel->fillScreen(myBLACK);
            }
        }
    }
}

void displayStaticText() {
    static enum { INITIAL, WAIT_INITIAL} state = INITIAL;
    static unsigned long stateStartTime = 0;
    int delayBetweenAnimations = 40;
    unsigned long now = millis();

    switch (state) {
        case INITIAL:
            // Clear the screen            
            FourScanPanel->fillScreen(myBLACK);
            // Set the cursor to the initial position
            FourScanPanel->setCursor(0, 0);
            // Display the text
            FourScanPanel->print(text);

            dma_display->flipDMABuffer();

            // Move to the next state
            state = WAIT_INITIAL;
            stateStartTime = now;
            break;
            
        case WAIT_INITIAL:
            // Wait for 1 second
            if (now - stateStartTime >= 2000) {
                // Move to the scrolling state
                state = INITIAL;
                stateStartTime = now;
            }
            break;
    }
}

void displayLargeText() {
    // Set the cursor position to center the text
    int16_t xCenter = (FourScanPanel->width() - w) / 2;
    int16_t yCenter = (FourScanPanel->height() - h) / 2;
    static enum { INITIAL, WAIT_INITIAL} state = INITIAL;
    static unsigned long stateStartTime = 0;
    int delayBetweenAnimations = 40;
    unsigned long now = millis();

    switch (state) {
        case INITIAL:
            // Clear the screen            
            FourScanPanel->fillScreen(myBLACK);
            // Set the cursor to the initial position
            FourScanPanel->setCursor(xCenter, yCenter+1);
            // Display the text
            FourScanPanel->print(text);

            dma_display->flipDMABuffer();

            // Move to the next state
            state = WAIT_INITIAL;
            stateStartTime = now;
            break;
            
        case WAIT_INITIAL:
            // Wait for 1 second
            if (now - stateStartTime >= 2000) {
                // Move to the scrolling state
                state = INITIAL;
                stateStartTime = now;
            }
            break;
    }
}

void specialScrollText() {
    static enum { INITIAL, WAIT_INITIAL, SCROLL, WAIT_SCROLL } state = INITIAL;
    static unsigned long stateStartTime = 0;
    int delayBetweenAnimations = 40;
    unsigned long now = millis();

    switch (state) {
        case INITIAL:
            // Clear the screen            
            FourScanPanel->fillScreen(myBLACK);
            // Set the cursor to the initial position
            FourScanPanel->setCursor(1, textYPosition + 1);
            // Display the text
            FourScanPanel->print(text);
            // Flip the DMA buffer after drawing
            dma_display->flipDMABuffer();
            // Move to the next state
            state = WAIT_INITIAL;
            stateStartTime = now;
            break;
           
        case WAIT_INITIAL:
            // Wait for 3 seconds
            if (now - stateStartTime >= 3000) {
                // Move to the scrolling state
                state = SCROLL;
                stateStartTime = now;
            }
            break;
           
        case SCROLL:
            // Clear the area where the text was previously displayed
            if (now - stateStartTime >= delayBetweenAnimations) {
                FourScanPanel->fillRect(textXPosition, textYPosition, w, h, myBLACK);
               
                // Update the text position
                textXPosition += scrollXMove;
               
                // Check if the text is off-screen to the left
                if (textXPosition + w - 2 <= FourScanPanel->width()) {
                    FourScanPanel->setCursor(textXPosition, textYPosition+1);
                    FourScanPanel->print(text);
                    // Move to the next state                    
                    state = WAIT_SCROLL;
                    stateStartTime = now;
                } else {
                    // Set cursor and print text
                    FourScanPanel->setCursor(textXPosition, textYPosition + 1);
                    FourScanPanel->print(text);                    
                }
               
                // Flip the DMA buffer after drawing
                dma_display->flipDMABuffer();
                stateStartTime = now;
            }
            break;

        case WAIT_SCROLL:
            // Wait for 1.5 seconds
            if (now - stateStartTime >= 1500) {
                // Reset to the initial state
                state = INITIAL;
                textXPosition = 2;
            }
            break;
    }
}

uint16_t getColorFromParam(int param) {
  switch (param) {
    case HITAM: return myBLACK;
    case MERAH: return myRED;
    case HIJAU: return myGREEN;
    case BIRU: return myBLUE;
    case ORANYE: return myORANGE;
    case PINK: return myPINK;
    case AMBER: return myAMBER;
    case PUTIH: return myWHITE;
    default: return myRED;
  }
}
