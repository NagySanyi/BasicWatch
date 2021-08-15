// BasicExtensions_TWatch2020.cpp
// Part of  BasicWatch - ttgo t-watch-2020


#include "config.h"
#include "BasicExtensions_TWatch2020.h"
#include <JPEGDecoder.h>
extern TTGOClass *ttgo;

static short int drawcolor = TFT_YELLOW;

void BasicEx_cls()
 {
   ttgo->tft->fillScreen(TFT_BLACK);
   ttgo->tft->setTextFont(2);
   ttgo->tft->setTextSize(2);
   ttgo->tft->setTextColor(TFT_WHITE);
   ttgo->tft->setCursor(0, 0);
 }

void BasicEx_box(short int x0, short int y0, short int x1, short int y1)
{
 ttgo->tft->drawRect(x0, y0, x1, y1, drawcolor);
 }


void BasicEx_line(short int x0, short int y0, short int x1, short int y1)
 {
  ttgo->tft->drawLine(x0, y0, x1, y1, drawcolor);
 }

void BasicEx_circle(short int x0, short int y0, short int r)
 {
  ttgo->tft->drawCircle(x0, y0, r, drawcolor);
 }

void BasicEx_triangle(short int x0, short int y0, short int x1, short int y1, short int x2, short int y2)
 {
 ttgo->tft->drawTriangle(x0, y0, x1, y1, x2, y2, drawcolor);
 }



/******************************************************************************
 * Based on tb_display.cpp
 * Hague Nusseck @ electricidea
 * https://github.com/electricidea/M5StickC-TB_Display
 ******************************************************************************/

#define TEXT_SIZE 2
#define TEXT_HEIGHT 32 // Height of text to be printed

#define TEXT_BUFFER_HEIGHT_MAX 10
#define TEXT_BUFFER_LINE_LENGTH_MAX 60
#define SCREEN_XSTARTPOS 5

char text_buffer[TEXT_BUFFER_HEIGHT_MAX][TEXT_BUFFER_LINE_LENGTH_MAX];

int text_buffer_height;
int text_buffer_line_length;
int text_buffer_write_pointer_x;
int text_buffer_write_pointer_y;
int text_buffer_read_pointer;

int screen_xpos = SCREEN_XSTARTPOS;
// start writing at the last line
int screen_ypos;
// maximum width of the screen
int screen_max;

// Enable or disable Waord Wrap
boolean tb_display_word_wrap = true;
extern TTGOClass *watch;


void tb_display_init(){
   text_buffer_height = 7;
   text_buffer_line_length = 40;
   screen_max = 240-2; 
   ttgo->tft->fillScreen(TFT_BLACK);
   ttgo->tft->setTextFont(2);
   ttgo->tft->setTextSize(2);
   ttgo->tft->setTextColor(TFT_WHITE);
   ttgo->tft->setCursor(0, 0);
    
   tb_display_clear();
   tb_display_show();
}



// =============================================================
// clear the text buffer without refreshing the screen
// =============================================================
void tb_display_clear(){
  for(int line=0; line<TEXT_BUFFER_HEIGHT_MAX; line++){
    for(int charpos=0; charpos<TEXT_BUFFER_LINE_LENGTH_MAX; charpos++){
      text_buffer[line][charpos]='\0';
    }
  }
  text_buffer_read_pointer = 0;
  text_buffer_write_pointer_x = 0;
  text_buffer_write_pointer_y = text_buffer_height-1;
  screen_xpos = SCREEN_XSTARTPOS;
  screen_ypos = TEXT_HEIGHT*(text_buffer_height-1);
}

// =============================================================
// clear the screen and display the text buffer
// =============================================================
void tb_display_show(){
 ttgo->tft->fillScreen(TFT_BLACK);
  int yPos = 0;
  for(int n=0; n<text_buffer_height; n++){
    // modulo operation for line position
    int line = (text_buffer_read_pointer+n) % text_buffer_height;
    int xPos = SCREEN_XSTARTPOS;
    int charpos=0;
    while(xPos < screen_max && text_buffer[line][charpos] != '\0'){
      xPos +=  ttgo->tft->drawChar(text_buffer[line][charpos],xPos,yPos,TEXT_SIZE);
      charpos++;
    }
    yPos = yPos + TEXT_HEIGHT;
  }
  screen_xpos = SCREEN_XSTARTPOS;
}

// =============================================================
// creates a new line and scroll the display upwards
// =============================================================
void tb_display_new_line(){
  text_buffer_write_pointer_x = 0;
  text_buffer_write_pointer_y++;
  text_buffer_read_pointer++;
  // circular buffer...
  if(text_buffer_write_pointer_y >= text_buffer_height)
    text_buffer_write_pointer_y = 0;
  if(text_buffer_read_pointer >= text_buffer_height)
    text_buffer_read_pointer = 0;
  // clear the actual new line for writing (first character a null terminator)
  text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
  tb_display_show();
}

// =============================================================
// print a single character
// =============================================================
void tb_display_print_char(unsigned char  data){
  // check for LF for new line
  if (data == '\n') {
    // last character in the text_buffer line  should be always a null terminator
  //   text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
 screen_xpos +=  ttgo->tft->drawChar('\n',screen_xpos,screen_ypos,TEXT_SIZE); //////////
    tb_display_new_line();
  }
  // only 'printable' characters
  if (data > 31 && data < 128) {
    // print the character and get the new xpos
    screen_xpos +=  ttgo->tft->drawChar(data,screen_xpos,screen_ypos,TEXT_SIZE);
    // if maximum number of characters reached
    if(text_buffer_write_pointer_x >= text_buffer_line_length-1){
      tb_display_new_line();
      // draw the character again because it was out of the screen last time
      screen_xpos +=  ttgo->tft->drawChar(data,screen_xpos,screen_ypos,TEXT_SIZE);
    }
    // or if line wrap is reached
    if(screen_xpos >= screen_max) {
      // prepare for Word-Wrap stuff...
      // the buffer for storing the last word content
      char Char_buffer[TEXT_BUFFER_LINE_LENGTH_MAX];
      int n = 1;
      Char_buffer[0] = data;
      Char_buffer[n] = '\0';
      // if Word-Wrap, go backwards and get the last "word" by finding the
      // last space character:
      if(tb_display_word_wrap){
        int test_pos = text_buffer_write_pointer_x-1;
        // get backwards and search a space character
        while(test_pos > 0 && text_buffer[text_buffer_write_pointer_y][test_pos] != ' '){
          // store all the characters on the way back to the last space character
          Char_buffer[n] = text_buffer[text_buffer_write_pointer_y][test_pos];
          test_pos--;
          n++;
          Char_buffer[n] = '\0';
        }
        // if there was no space character in the row, Word-Wrap is not possible
        if(test_pos == 0) {
          // don't use the buffer but draw the character passed to the function
          n = 1;
        } else {
          // otherwise use the buffer to print the last found characters of the word
          // but only, if the charachter that causes a word wrap is not a space character
          if(data != ' '){
            // place a \0 at the position of the found space so that the drawing fuction ends here
            text_buffer[text_buffer_write_pointer_y][test_pos] = '\0';
          }
        }
      }
      tb_display_new_line();
      // icharacter passed to the function is a space character, then don't display
      // it as the first character of the new line
      if(data == ' ') 
        // don't use the buffer at all
        n = 0;
      n--;
      while(n >= 0){
        // draw the characters from the buffer back on the screen
        screen_xpos +=  ttgo->tft->drawChar(Char_buffer[n],screen_xpos,screen_ypos,TEXT_SIZE);
        // write the characters into the screen buffer of the new line
        text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = Char_buffer[n];
        text_buffer_write_pointer_x++;
        n--;
      }
      text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
    } else {
      // write the character into the screen buffer
      text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = data;
      text_buffer_write_pointer_x++;
      // following character a null terminator to clear the old characters of the line
      text_buffer[text_buffer_write_pointer_y][text_buffer_write_pointer_x] = '\0';
    }
  }
}

void tb_display_print_String(const char *s, int chr_delay){
  while(*s != 0){
    tb_display_print_char(*s++);
    if(chr_delay > 0)
      delay(chr_delay);
  }
}



////////////////////// drawBmp //////////////////////

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void drawBmp(const char *filename, short int x, short int y) {



Serial.println(filename);




  if ((x >=  ttgo->tft->width()) || (y >=  ttgo->tft->height())) return;

  fs::File bmpFS;

  bmpFS = SPIFFS.open(filename, "r");
  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = ttgo->tft->getSwapBytes();
      ttgo->tft->setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        
        for (uint16_t col = 0; col < w; col++)	// Convert 24 to 16 bit colours
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }
         ttgo->tft->pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      ttgo->tft->setSwapBytes(oldSwapBytes);
      Serial.print("Loaded in "); Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}


//////////////////// drawBmp end //////////////////////


////////////////////// drawJpg //////////////////////

void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = ttgo->tft->getSwapBytes();
  ttgo->tft->setSwapBytes(true);
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;
  max_x += xpos;
  max_y += ypos;

  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= ttgo->tft->width() && ( mcu_y + win_h ) <= ttgo->tft->height())
      ttgo->tft->pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= ttgo->tft->height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  ttgo->tft->setSwapBytes(swapBytes);
}

void  drawJpg(const char *filename,  short int xpos, short int  ypos) {

  fs::File jpegFS;
  jpegFS = SPIFFS.open(filename, "r");
  if (!jpegFS)
  {
   Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }



  boolean decoded = JpegDec.decodeSdFile(jpegFS);      // Pass the SD file handle to the decoder,
  //boolean decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
   //  jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
 jpegFS.close();
}



////////////////////// drawJpg end ////////////////////
