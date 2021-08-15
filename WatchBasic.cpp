 /******************************************************************
  *                                                                *
  *                       WatchBasic.cpp                           *
  *                                                                *
  *      Based on:  https://github.com/BleuLlama/TinyBasicPlus     *
  *                                                                *
  *              Platform :Arduino ESP-32                          *
  *                                                                *
  ******************************************************************/
 /*
  TinyBasic Authors: 
     Gordon Brandly (Tiny Basic for 68000)
     Mike Field <hamster@snap.net.nz> (Arduino Basic) (port to Arduino)
     Scott Lawrence <yorgle@gmail.com> (TinyBasic Plus) (features, etc)
 
  Contributors:
           Brian O'Dell <megamemnon@megamemnon.com> (INPUT)
     (full list tbd)
  
  LICENSING NOTES:
     Mike Field based his C port of Tiny Basic on the 68000 
     Tiny BASIC which carried the following license:               */
 /******************************************************************
  *                                                                *
  *               Tiny BASIC for the Motorola MC68000              *
  *                                                                *
  * Derived from Palo Alto Tiny BASIC as published in the May 1976 *
  * issue of Dr. Dobb's Journal.  Adapted to the 68000 by:         *
  *       Gordon Brandly                                           *
  *       12147 - 51 Street                                        *
  *       Edmonton AB  T5W 3G8                                     *
  *       Canada                                                   *
  *       (updated mailing address for 1996)                       *
  *                                                                *
  * This version is for MEX68KECB Educational Computer Board I/O.  *
  *                                                                *
  ******************************************************************
  *    Copyright (C) 1984 by Gordon Brandly. This program may be   *
  *    freely distributed for personal use only. All commercial    *
  *                      rights are reserved.                      *
  ******************************************************************/

// includes, and settings for Arduino-specific features

 #include "BasicExtensions_TWatch2020.h"

 #define kConsoleBaud  115200
 #define kRamSize  (64*1024-1)  
 #define ENABLE_FILEIO 1
 #define ENABLE_AUTORUN 1
 #define kAutorunFilename  "autorun.bas"

 #undef ENABLE_TONES
   
 int Basicstate = 0; 

#ifdef ENABLE_FILEIO			  //  File io
  #include <SPIFFS.h>
  #include <fs.h> 
  #define kFS_Fail  0
  #define kFS_OK    1
  File fp;

  #define Basic_Folder "/tinybasic/"
  #define Image_Folder "/images/"

  void cmd_Files( void );
  unsigned char * filenameWord(void);
  static boolean FS_is_initialized = false;
#endif
 
#ifndef boolean 
  #define boolean int
  #define true 1
  #define false 0
#endif

#ifndef byte
  typedef unsigned char byte;
#endif

// some catches for AVR based text string stuff...
#ifndef PROGMEM
  #define PROGMEM
#endif
#ifndef pgm_read_byte
  #define pgm_read_byte( A ) *(A)
#endif

////////////////////

// some settings based things

boolean inhibitOutput = false;
static boolean runAfterLoad = false;
static boolean triggerRun = false;

// these will select, at runtime, where IO happens through for load/save
enum {
  kStreamSerial = 0,
  kStreamFile
};
static unsigned char inStream = kStreamSerial;
static unsigned char outStream = kStreamSerial;

static boolean TextBuff_is_initialized = false;


/***********************************************************/

// ASCII Characters
#define CR	'\r'
#define NL	'\n'
#define LF      0x0a
#define TAB	'\t'
#define BELL	'\b'
#define SPACE   ' '
#define SQUOTE  '\''
#define DQUOTE  '\"'
#define CTRLC	0x03
#define CTRLH	0x08
#define CTRLS	0x13
#define CTRLX	0x18


#define ECHO_CHARS 1
typedef short unsigned LINENUM;

static unsigned char program[kRamSize];
static const char *  sentinel = "HELLO";
static unsigned char *txtpos,*list_line, *tmptxtpos;
static unsigned char expression_error;
static unsigned char *tempsp;

/***********************************************************/
// Keyword table and constants - the last character has 0x80 added to it
const static unsigned char keywords[] PROGMEM = {
  'L','I','S','T'+0x80,
  'L','O','A','D'+0x80,
  'N','E','W'+0x80,
  'R','U','N'+0x80,
  'S','A','V','E'+0x80,
  'N','E','X','T'+0x80,
  'L','E','T'+0x80,
  'I','F'+0x80,
  'G','O','T','O'+0x80,
  'G','O','S','U','B'+0x80,
  'R','E','T','U','R','N'+0x80,
  'R','E','M'+0x80,
  'F','O','R'+0x80,
  'I','N','P','U','T'+0x80,
  'P','R','I','N','T'+0x80,
  'P','O','K','E'+0x80,
  'S','T','O','P'+0x80,
  'B','Y','E'+0x80,
  'F','I','L','E','S'+0x80,
  'M','E','M'+0x80,
  '?'+ 0x80,
  '\''+ 0x80,
  'A','W','R','I','T','E'+0x80,
  'D','W','R','I','T','E'+0x80,
  'D','E','L','A','Y'+0x80,
  'E','N','D'+0x80,
  'R','S','E','E','D'+0x80,
  'C','H','A','I','N'+0x80,
#ifdef ENABLE_TONES
  'T','O','N','E','W'+0x80,
  'T','O','N','E'+0x80,
  'N','O','T','O','N','E'+0x80,
#endif

#ifdef BasicExtensions
  'T','E','X','T'+0x80,
  'C','L','S'+0x80,
  'L','I','N','E'+0x80,
  'C','I','R','C','L','E'+0x80,
  'T','R','I','A','N','G','L','E'+0x80,
  'B','O','X'+0x80,
  'I','M','A','G','E'+0x80,
  'B','L','E'+0x80,
  'S','E','N','S','O','R'+0x80, 
#endif
  0
};

// by moving the command list to an enum, we can easily remove sections 
// above and below simultaneously to selectively obliterate functionality.
enum {
  KW_LIST = 0,
  KW_LOAD, KW_NEW, KW_RUN, KW_SAVE,
  KW_NEXT, KW_LET, KW_IF,
  KW_GOTO, KW_GOSUB, KW_RETURN,
  KW_REM,
  KW_FOR,
  KW_INPUT, KW_PRINT,
  KW_POKE,
  KW_STOP, KW_BYE,
  KW_FILES,
  KW_MEM,
  KW_QMARK, KW_QUOTE,
  KW_AWRITE, KW_DWRITE,
  KW_DELAY,
  KW_END,
  KW_RSEED,
  KW_CHAIN,
#ifdef ENABLE_TONES
  KW_TONEW, KW_TONE, KW_NOTONE,
#endif


#ifdef BasicExtensions
  KW_TEXT, KW_CLS, 
  KW_LINE, KW_CIRCLE, KW_TRIANGLE, KW_BOX, 
  KW_IMAGE,
  KW_BLE, KW_SENSOR,
#endif

  KW_DEFAULT /* always the final one*/
};

struct stack_for_frame {
  char frame_type;
  char for_var;
  short int terminal;
  short int step;
  unsigned char *current_line;
  unsigned char *txtpos;
};

struct stack_gosub_frame {
  char frame_type;
  unsigned char *current_line;
  unsigned char *txtpos;
};

const static unsigned char func_tab[] PROGMEM = {
  'P','E','E','K'+0x80,
  'A','B','S'+0x80,
  'A','R','E','A','D'+0x80,
  'D','R','E','A','D'+0x80,
  'R','N','D'+0x80,
  0
};
#define FUNC_PEEK    0
#define FUNC_ABS     1
#define FUNC_AREAD   2
#define FUNC_DREAD   3
#define FUNC_RND     4
#define FUNC_UNKNOWN 5

const static unsigned char to_tab[] PROGMEM = {
  'T','O'+0x80,
  0
};

const static unsigned char step_tab[] PROGMEM = {
  'S','T','E','P'+0x80,
  0
};

const static unsigned char relop_tab[] PROGMEM = {
  '>','='+0x80,
  '<','>'+0x80,
  '>'+0x80,
  '='+0x80,
  '<','='+0x80,
  '<'+0x80,
  '!','='+0x80,
  0
};

#define RELOP_GE		0
#define RELOP_NE		1
#define RELOP_GT		2
#define RELOP_EQ		3
#define RELOP_LE		4
#define RELOP_LT		5
#define RELOP_NE_BANG		6
#define RELOP_UNKNOWN	7

const static unsigned char highlow_tab[] PROGMEM = { 
  'H','I','G','H'+0x80,
  'H','I'+0x80,
  'L','O','W'+0x80,
  'L','O'+0x80,
  0
};
#define HIGHLOW_HIGH    1
#define HIGHLOW_UNKNOWN 4

#define STACK_SIZE (sizeof(struct stack_for_frame)*5)
#define VAR_SIZE sizeof(short int) // Size of variables in bytes

static unsigned char *stack_limit;
static unsigned char *program_start;
static unsigned char *program_end;
static unsigned char *stack; // Software stack for things that should go on the CPU stack
static unsigned char *variables_begin;
static unsigned char *current_line;
static unsigned char *sp;
#define STACK_GOSUB_FLAG 'G'
#define STACK_FOR_FLAG 'F'
static unsigned char table_index;
static LINENUM linenum;

static const unsigned char okmsg[]            PROGMEM = "OK";
static const unsigned char whatmsg[]          PROGMEM = "What? ";
static const unsigned char howmsg[]           PROGMEM =	"How?";
static const unsigned char sorrymsg[]         PROGMEM = "Sorry!";
static const unsigned char initmsg[]          PROGMEM = "TinyBasic Plus";
static const unsigned char memorymsg[]        PROGMEM = " bytes free.";

static const unsigned char breakmsg[]         PROGMEM = "break!";
static const unsigned char unimplimentedmsg[] PROGMEM = "Unimplemented";
static const unsigned char backspacemsg[]     PROGMEM = "\b \b";
static const unsigned char indentmsg[]        PROGMEM = "    ";
static const unsigned char FSerrormsg[]       PROGMEM = "FS error.";
static const unsigned char FSfilemsg[]        PROGMEM = "File error.";
static const unsigned char dirextmsg[]        PROGMEM = "(dir)";
static const unsigned char slashmsg[]         PROGMEM = "/";
static const unsigned char spacemsg[]         PROGMEM = " ";

static int inchar(void);
static void outchar(unsigned char c);
static void line_terminator(void);
static short int expression(void);
static unsigned char breakcheck(void);





    unsigned char *Sensor_ADDR;
    unsigned char *serviceID;
    unsigned char *charID;


/***************************************************************************/
static void ignore_blanks(void)
{
  while(*txtpos == SPACE || *txtpos == TAB)
    txtpos++;
}


/***************************************************************************/
static void scantable(const unsigned char *table)
{
  int i = 0;
  table_index = 0;
  while(1)
  {
    // Run out of table entries?
    if(pgm_read_byte( table ) == 0)
      return;

    // Do we match this character?
    if(txtpos[i] == pgm_read_byte( table ))
    {
      i++;
      table++;
    }
    else
    {
      // do we match the last character of keywork (with 0x80 added)? If so, return
      if(txtpos[i]+0x80 == pgm_read_byte( table ))
      {
        txtpos += i+1;  // Advance the pointer to following the keyword
        ignore_blanks();
        return;
      }

      // Forward to the end of this keyword
      while((pgm_read_byte( table ) & 0x80) == 0)
        table++;

      // Now move on to the first character of the next word, and reset the position index
      table++;
      table_index++;
      ignore_blanks();
      i = 0;
    }
  }
}

/***************************************************************************/
static void pushb(unsigned char b)
{
  sp--;
  *sp = b;
}

/***************************************************************************/
static unsigned char popb()
{
  unsigned char b;
  b = *sp;
  sp++;
  return b;
}

/***************************************************************************/
void printnum(int num)
{
  int digits = 0;

  if(num < 0)
  {
    num = -num;
    outchar('-');
  }
  do {
    pushb(num%10+'0');
    num = num/10;
    digits++;
  }
  while (num > 0);

  while(digits > 0)
  {
    outchar(popb());
    digits--;
  }
}

void printUnum(unsigned int num)
{
  int digits = 0;

  do {
    pushb(num%10+'0');
    num = num/10;
    digits++;
  }
  while (num > 0);

  while(digits > 0)
  {
    outchar(popb());
    digits--;
  }
}

/***************************************************************************/
static unsigned short testnum(void)
{
  unsigned short num = 0;
  ignore_blanks();

  while(*txtpos>= '0' && *txtpos <= '9' )
  {
    // Trap overflows
    if(num >= 0xFFFF/10)
    {
      num = 0xFFFF;
      break;
    }

    num = num *10 + *txtpos - '0';
    txtpos++;
  }
  return	num;
}

/***************************************************************************/
static unsigned char print_quoted_string(void)
{
  int i=0;
  unsigned char delim = *txtpos;
  if(delim != '"' && delim != '\'')
    return 0;
  txtpos++;

  // Check we have a closing delimiter
  while(txtpos[i] != delim)
  {
    if(txtpos[i] == NL)
      return 0;
    i++;
  }

  // Print the characters
  while(*txtpos != delim)
  {
    outchar(*txtpos);
    txtpos++;
  }
  txtpos++; // Skip over the last delimiter

  return 1;
}


/***************************************************************************/
void printmsgNoNL(const unsigned char *msg)
{
  while( pgm_read_byte( msg ) != 0 ) {
    outchar( pgm_read_byte( msg++ ) );
  };
}

/***************************************************************************/
void printmsg(const unsigned char *msg)
{
  printmsgNoNL(msg);
  line_terminator();
}

/***************************************************************************/
static void getln(char prompt)
{
  outchar(prompt);
  txtpos = program_end+sizeof(LINENUM);

  while(1)
  {
    char c = inchar();
    switch(c)
    {
    case NL:
      //break;
    case CR:
      line_terminator();
      // Terminate all strings with a NL
      txtpos[0] = NL;
      return;
    case CTRLH:
      if(txtpos == program_end)
        break;
      txtpos--;

      printmsg(backspacemsg);
      break;
    default:
      // We need to leave at least one space to allow us to shuffle the line into order
      if(txtpos == variables_begin-2)
        outchar(BELL);
      else
      {
        txtpos[0] = c;
        txtpos++;
        outchar(c);
      }
    }
  }
}

/***************************************************************************/
static unsigned char *findline(void)
{
  unsigned char *line = program_start;
  while(1)
  {
    if(line == program_end)
      return line;

    if(((LINENUM *)line)[0] >= linenum)
      return line;

    // Add the line length onto the current address, to get to the next line;
    line += line[sizeof(LINENUM)];
  }
}

/***************************************************************************/
static void toUppercaseBuffer(void)
{
  unsigned char *c = program_end+sizeof(LINENUM);
  unsigned char quote = 0;

  while(*c != NL)
  {
    // Are we in a quoted string?
    if(*c == quote)
      quote = 0;
    else if(*c == '"' || *c == '\'')
      quote = *c;
    else if(quote == 0 && *c >= 'a' && *c <= 'z')
      *c = *c + 'A' - 'a';
    c++;
  }
}

/***************************************************************************/
void printline()
{
  LINENUM line_num;

  line_num = *((LINENUM *)(list_line));
  list_line += sizeof(LINENUM) + sizeof(char);

  // Output the line */
  printnum(line_num);
  outchar(' ');
  while(*list_line != NL)
  {
    outchar(*list_line);
    list_line++;
  }
  list_line++;
#ifdef ALIGN_MEMORY
  // Start looking for next line on even page
  if (ALIGN_UP(list_line) != list_line)
    list_line++;
#endif
  line_terminator();
}

/***************************************************************************/
static short int expr4(void)
{
  // fix provided by Jurg Wullschleger wullschleger@gmail.com
  // fixes whitespace and unary operations
  ignore_blanks();

  if( *txtpos == '-' ) {
    txtpos++;
    return -expr4();
  }
  // end fix

  if(*txtpos == '0')
  {
    txtpos++;
    return 0;
  }

  if(*txtpos >= '1' && *txtpos <= '9')
  {
    short int a = 0;
    do  {
      a = a*10 + *txtpos - '0';
      txtpos++;
    } 
    while(*txtpos >= '0' && *txtpos <= '9');
    return a;
  }

  // Is it a function or variable reference?
  if(txtpos[0] >= 'A' && txtpos[0] <= 'Z')
  {
    short int a;
    // Is it a variable reference (single alpha)
    if(txtpos[1] < 'A' || txtpos[1] > 'Z')
    {
      a = ((short int *)variables_begin)[*txtpos - 'A'];
      txtpos++;
      return a;
    }

    // Is it a function with a single parameter
    scantable(func_tab);
    if(table_index == FUNC_UNKNOWN)
      goto expr4_error;

    unsigned char f = table_index;

    if(*txtpos != '(')
      goto expr4_error;

    txtpos++;
    a = expression();
    if(*txtpos != ')')
      goto expr4_error;
    txtpos++;
    switch(f)
    {
    case FUNC_PEEK:
      return program[a];
      
    case FUNC_ABS:
      if(a < 0) 
        return -a;
      return a;


    case FUNC_AREAD:
      pinMode( a, INPUT );
      return analogRead( a );                        
    case FUNC_DREAD:
      pinMode( a, INPUT );
      return digitalRead( a );


    case FUNC_RND:

      return( random( a ));

    }
  }

  if(*txtpos == '(')
  {
    short int a;
    txtpos++;
    a = expression();
    if(*txtpos != ')')
      goto expr4_error;

    txtpos++;
    return a;
  }

expr4_error:
  expression_error = 1;
  return 0;

}
 

/***************************************************************************/
static short int expr3(void)
{
  short int a,b;

  a = expr4();

  ignore_blanks(); // fix for eg:  100 a = a + 1

  while(1)
  {
    if(*txtpos == '*')
    {
      txtpos++;
      b = expr4();
      a *= b;
    }
    else if(*txtpos == '/')
    {
      txtpos++;
      b = expr4();
      if(b != 0)
        a /= b;
      else
        expression_error = 1;
    }
    else
      return a;
  }
}

/***************************************************************************/
static short int expr2(void)
{
  short int a,b;

  if(*txtpos == '-' || *txtpos == '+')
    a = 0;
  else
    a = expr3();

  while(1)
  {
    if(*txtpos == '-')
    {
      txtpos++;
      b = expr3();
      a -= b;
    }
    else if(*txtpos == '+')
    {
      txtpos++;
      b = expr3();
      a += b;
    }
    else
      return a;
  }
}
/***************************************************************************/
static short int expression(void)
{
  short int a,b;

  a = expr2();

  // Check if we have an error
  if(expression_error)	return a;

  scantable(relop_tab);
  if(table_index == RELOP_UNKNOWN)
    return a;

  switch(table_index)
  {
  case RELOP_GE:
    b = expr2();
    if(a >= b) return 1;
    break;
  case RELOP_NE:
  case RELOP_NE_BANG:
    b = expr2();
    if(a != b) return 1;
    break;
  case RELOP_GT:
    b = expr2();
    if(a > b) return 1;
    break;
  case RELOP_EQ:
    b = expr2();
    if(a == b) return 1;
    break;
  case RELOP_LE:
    b = expr2();
    if(a <= b) return 1;
    break;
  case RELOP_LT:
    b = expr2();
    if(a < b) return 1;
    break;
  }
  return 0;
}

/****************************  Modified     loop()  *****************************/

void WatchBasic_loop()                       
{
  unsigned char *start;
  unsigned char *newEnd;
  unsigned char linelen;
  boolean isDigital;
  boolean alsoWait = false;
  int val;

#ifdef ENABLE_TONES
  noTone( kPiezoPin );
#endif
  program_start = program;
  program_end = program_start;
  sp = program+sizeof(program);  // Needed for printnum
#ifdef ALIGN_MEMORY
  // Ensure these memory blocks start on even pages
  stack_limit = ALIGN_DOWN(program+sizeof(program)-STACK_SIZE);
  variables_begin = ALIGN_DOWN(stack_limit - 27*VAR_SIZE);
#else
  stack_limit = program+sizeof(program)-STACK_SIZE;
  variables_begin = stack_limit - 27*VAR_SIZE;
#endif

  // memory free
  printnum(variables_begin-program_end);
  printmsg(memorymsg);



warmstart:
  // this signifies that it is running in 'direct' mode.
  current_line = 0;
  sp = program+sizeof(program);
  printmsg(okmsg);

prompt:
  if( triggerRun ){
    triggerRun = false;
    current_line = program_start;
    goto execline;
  }

  getln( '>' );
  toUppercaseBuffer();

  txtpos = program_end+sizeof(unsigned short);

  // Find the end of the freshly entered line
  while(*txtpos != NL)
    txtpos++;

  // Move it to the end of program_memory
  {
    unsigned char *dest;
    dest = variables_begin-1;
    while(1)
    {
      *dest = *txtpos;
      if(txtpos == program_end+sizeof(unsigned short))
        break;
      dest--;
      txtpos--;
    }
    txtpos = dest;
  }

  // Now see if we have a line number
  linenum = testnum();
  ignore_blanks();
  if(linenum == 0)
    goto direct;

  if(linenum == 0xFFFF)
    goto qhow;

  // Find the length of what is left, including the (yet-to-be-populated) line header
  linelen = 0;
  while(txtpos[linelen] != NL)
    linelen++;
  linelen++; // Include the NL in the line length
  linelen += sizeof(unsigned short)+sizeof(char); // Add space for the line number and line length

  // Now we have the number, add the line header.
  txtpos -= 3;

#ifdef ALIGN_MEMORY
  // Line starts should always be on 16-bit pages
  if (ALIGN_DOWN(txtpos) != txtpos)
  {
    txtpos--;
    linelen++;
    // As the start of the line has moved, the data should move as well
    unsigned char *tomove;
    tomove = txtpos + 3;
    while (tomove < txtpos + linelen - 1)
    {
      *tomove = *(tomove + 1);
      tomove++;
    }
  }
#endif

  *((unsigned short *)txtpos) = linenum;
  txtpos[sizeof(LINENUM)] = linelen;


  // Merge it into the rest of the program
  start = findline();

  // If a line with that number exists, then remove it
  if(start != program_end && *((LINENUM *)start) == linenum)
  {
    unsigned char *dest, *from;
    unsigned tomove;

    from = start + start[sizeof(LINENUM)];
    dest = start;

    tomove = program_end - from;
    while( tomove > 0)
    {
      *dest = *from;
      from++;
      dest++;
      tomove--;
    }	
    program_end = dest;
  }

  if(txtpos[sizeof(LINENUM)+sizeof(char)] == NL) // If the line has no txt, it was just a delete
    goto prompt;



  // Make room for the new line, either all in one hit or lots of little shuffles
  while(linelen > 0)
  {	
    unsigned int tomove;
    unsigned char *from,*dest;
    unsigned int space_to_make;

    space_to_make = txtpos - program_end;

    if(space_to_make > linelen)
      space_to_make = linelen;
    newEnd = program_end+space_to_make;
    tomove = program_end - start;


    // Source and destination - as these areas may overlap we need to move bottom up
    from = program_end;
    dest = newEnd;
    while(tomove > 0)
    {
      from--;
      dest--;
      *dest = *from;
      tomove--;
    }

    // Copy over the bytes into the new space
    for(tomove = 0; tomove < space_to_make; tomove++)
    {
      *start = *txtpos;
      txtpos++;
      start++;
      linelen--;
    }
    program_end = newEnd;
  }
  goto prompt;

unimplemented:
  printmsg(unimplimentedmsg);
  goto prompt;

qhow:	
  printmsg(howmsg);
  goto prompt;

qwhat:	
  printmsgNoNL(whatmsg);
  if(current_line != NULL)
  {
    unsigned char tmp = *txtpos;
    if(*txtpos != NL)
      *txtpos = '^';
    list_line = current_line;
    printline();
    *txtpos = tmp;
  }
  line_terminator();
  goto prompt;

qsorry:	
  printmsg(sorrymsg);
  goto warmstart;

run_next_statement:
  while(*txtpos == ':')
    txtpos++;
  ignore_blanks();
  if(*txtpos == NL)
    goto execnextline;
  goto interperateAtTxtpos;

direct: 
  txtpos = program_end+sizeof(LINENUM);
  if(*txtpos == NL)
    goto prompt;

interperateAtTxtpos:
  if(breakcheck())
  {
    printmsg(breakmsg);
    goto warmstart;
  }

  scantable(keywords);

  switch(table_index)
  {
  case KW_DELAY:
    {

      expression_error = 0;
      val = expression();
      delay( val );
      goto execnextline;

    }

  case KW_FILES:
    goto files;
  case KW_LIST:
    goto list;
  case KW_CHAIN:
    goto chain;
  case KW_LOAD:
    goto load;
  case KW_MEM:
    goto mem;
  case KW_NEW:
    if(txtpos[0] != NL)
      goto qwhat;
    program_end = program_start;
    goto prompt;
  case KW_RUN:
    current_line = program_start;
    goto execline;
  case KW_SAVE:
    goto save;
  case KW_NEXT:
    goto next;
  case KW_LET:
    goto assignment;
  case KW_IF:
    short int val;
    expression_error = 0;
    val = expression();
    if(expression_error || *txtpos == NL)
      goto qhow;
    if(val != 0)
      goto interperateAtTxtpos;
    goto execnextline;

  case KW_GOTO:
    expression_error = 0;
    linenum = expression();
    if(expression_error || *txtpos != NL)
      goto qhow;
    current_line = findline();
    goto execline;

  case KW_GOSUB:
    goto gosub;
  case KW_RETURN:
    goto gosub_return; 
  case KW_REM:
  case KW_QUOTE:
    goto execnextline;	// Ignore line completely
  case KW_FOR:
    goto forloop; 
  case KW_INPUT:
    goto input; 
  case KW_PRINT:
  case KW_QMARK:
    goto print;
  case KW_POKE:
    goto poke;
  case KW_END:
  case KW_STOP:
    // This is the easy way to end - set the current line to the end of program attempt to run it
    if(txtpos[0] != NL)
      goto qwhat;
    current_line = program_end;
    goto execline;
  case KW_BYE:		//  Modified leave the basic interperater
   BasicEx_cls(); 
   Basicstate = -1;   
   ESP.restart();       // ?
   return;              // ?

  case KW_AWRITE:  // AWRITE <pin>, HIGH|LOW
    isDigital = false;
    goto awrite;
  case KW_DWRITE:  // DWRITE <pin>, HIGH|LOW
    isDigital = true;
    goto dwrite;

  case KW_RSEED:
    goto rseed;

#ifdef ENABLE_TONES
  case KW_TONEW:
  case KW_TONE:
  case KW_NOTONE:
     goto unimplemented;

#endif



#ifdef BasicExtensions            // New 
 
 case  KW_TEXT:
    goto textmode;

 case KW_CLS:
    goto cls;

 case  KW_LINE:
    goto line;

 case  KW_CIRCLE:
    goto circle;
 
 case  KW_TRIANGLE:
    goto triangle;

 case KW_BOX:
    goto box;

 case KW_IMAGE:
     goto image;

  case KW_BLE:
  case KW_SENSOR:
    goto unimplemented;

#endif

  case KW_DEFAULT:
    goto assignment;
  default:
    break;
  }

execnextline:
  if(current_line == NULL)		// Processing direct commands?
    goto prompt;
  current_line +=	 current_line[sizeof(LINENUM)];

execline:
  if(current_line == program_end) // Out of lines to run
    goto warmstart;
  txtpos = current_line+sizeof(LINENUM)+sizeof(char);
  goto interperateAtTxtpos;

input:
  {
    unsigned char var;
    int value;
    ignore_blanks();
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qwhat;
    var = *txtpos;
    txtpos++;
    ignore_blanks();
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
inputagain:
    tmptxtpos = txtpos;
    getln( '?' );
    toUppercaseBuffer();
    txtpos = program_end+sizeof(unsigned short);
    ignore_blanks();
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto inputagain;
    ((short int *)variables_begin)[var-'A'] = value;
    txtpos = tmptxtpos;

    goto run_next_statement;
  }

forloop:
  {
    unsigned char var;
    short int initial, step, terminal;
    ignore_blanks();
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qwhat;
    var = *txtpos;
    txtpos++;
    ignore_blanks();
    if(*txtpos != '=')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    expression_error = 0;
    initial = expression();
    if(expression_error)
      goto qwhat;

    scantable(to_tab);
    if(table_index != 0)
      goto qwhat;

    terminal = expression();
    if(expression_error)
      goto qwhat;

    scantable(step_tab);
    if(table_index == 0)
    {
      step = expression();
      if(expression_error)
        goto qwhat;
    }
    else
      step = 1;
    ignore_blanks();
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;


    if(!expression_error && *txtpos == NL)
    {
      struct stack_for_frame *f;
      if(sp + sizeof(struct stack_for_frame) < stack_limit)
        goto qsorry;

      sp -= sizeof(struct stack_for_frame);
      f = (struct stack_for_frame *)sp;
      ((short int *)variables_begin)[var-'A'] = initial;
      f->frame_type = STACK_FOR_FLAG;
      f->for_var = var;
      f->terminal = terminal;
      f->step     = step;
      f->txtpos   = txtpos;
      f->current_line = current_line;
      goto run_next_statement;
    }
  }
  goto qhow;

gosub:
  expression_error = 0;
  linenum = expression();
  if(!expression_error && *txtpos == NL)
  {
    struct stack_gosub_frame *f;
    if(sp + sizeof(struct stack_gosub_frame) < stack_limit)
      goto qsorry;

    sp -= sizeof(struct stack_gosub_frame);
    f = (struct stack_gosub_frame *)sp;
    f->frame_type = STACK_GOSUB_FLAG;
    f->txtpos = txtpos;
    f->current_line = current_line;
    current_line = findline();
    goto execline;
  }
  goto qhow;

next:
  // Fnd the variable name
  ignore_blanks();
  if(*txtpos < 'A' || *txtpos > 'Z')
    goto qhow;
  txtpos++;
  ignore_blanks();
  if(*txtpos != ':' && *txtpos != NL)
    goto qwhat;

gosub_return:
  // Now walk up the stack frames and find the frame we want, if present
  tempsp = sp;
  while(tempsp < program+sizeof(program)-1)
  {
    switch(tempsp[0])
    {
    case STACK_GOSUB_FLAG:
      if(table_index == KW_RETURN)
      {
        struct stack_gosub_frame *f = (struct stack_gosub_frame *)tempsp;
        current_line	= f->current_line;
        txtpos			= f->txtpos;
        sp += sizeof(struct stack_gosub_frame);
        goto run_next_statement;
      }
      // This is not the loop you are looking for... so Walk back up the stack
      tempsp += sizeof(struct stack_gosub_frame);
      break;
    case STACK_FOR_FLAG:
      // Flag, Var, Final, Step
      if(table_index == KW_NEXT)
      {
        struct stack_for_frame *f = (struct stack_for_frame *)tempsp;
        // Is the the variable we are looking for?
        if(txtpos[-1] == f->for_var)
        {
          short int *varaddr = ((short int *)variables_begin) + txtpos[-1] - 'A'; 
          *varaddr = *varaddr + f->step;
          // Use a different test depending on the sign of the step increment
          if((f->step > 0 && *varaddr <= f->terminal) || (f->step < 0 && *varaddr >= f->terminal))
          {
            // We have to loop so don't pop the stack
            txtpos = f->txtpos;
            current_line = f->current_line;
            goto run_next_statement;
          }
          // We've run to the end of the loop. drop out of the loop, popping the stack
          sp = tempsp + sizeof(struct stack_for_frame);
          goto run_next_statement;
        }
      }
      // This is not the loop you are looking for... so Walk back up the stack
      tempsp += sizeof(struct stack_for_frame);
      break;
    default:
      //printf("Stack is stuffed!\n");
      goto warmstart;
    }
  }
  // Didn't find the variable we've been looking for
  goto qhow;

assignment:
  {
    short int value;
    short int *var;

    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qhow;
    var = (short int *)variables_begin + *txtpos - 'A';
    txtpos++;

    ignore_blanks();

    if (*txtpos != '=')
      goto qwhat;
    txtpos++;
    ignore_blanks();
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    // Check that we are at the end of the statement
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
    *var = value;
  }
  goto run_next_statement;
poke:
  {
    short int value;
    unsigned char *address;

    // Work out where to put it
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    address = (unsigned char *)value;

    // check for a comma
    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    // Now get the value to assign
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    //printf("Poke %p value %i\n",address, (unsigned char)value);
    // Check that we are at the end of the statement
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
  }
  goto run_next_statement;

list:
  linenum = testnum(); // Retuns 0 if no line found.

  // Should be EOL
  if(txtpos[0] != NL)
    goto qwhat;

  // Find the line
  list_line = findline();
  while(list_line != program_end)
    printline();
  goto warmstart;

print:
  // If we have an empty list then just put out a NL
  if(*txtpos == ':' )
  {
    line_terminator();
    txtpos++;
    goto run_next_statement;
  }
  if(*txtpos == NL)
  {
    goto execnextline;
  }

  while(1)
  {
    ignore_blanks();
    if(print_quoted_string())
    {
      ;
    }
    else if(*txtpos == '"' || *txtpos == '\'')
      goto qwhat;
    else
    {
      short int e;
      expression_error = 0;
      e = expression();
      if(expression_error)
        goto qwhat;
      printnum(e);
    }

    // At this point we have three options, a comma or a new line
    if(*txtpos == ',')
      txtpos++;	// Skip the comma and move onto the next
    else if(txtpos[0] == ';' && (txtpos[1] == NL || txtpos[1] == ':'))
    {
      txtpos++; // This has to be the end of the print - no newline
      break;
    }
    else if(*txtpos == NL || *txtpos == ':')
    {
      line_terminator();	// The end of the print statement
      break;
    }
    else
      goto qwhat;	
  }
  goto run_next_statement;

mem:
  // memory free
  printnum(variables_begin-program_end);
  printmsg(memorymsg);



  goto run_next_statement;


  /*************************************************/

awrite: // AWRITE <pin>,val
dwrite:
 
  goto unimplemented;


rseed:
  {
    short int value;

    //Get the pin number
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;

    randomSeed( value );

    goto run_next_statement;
  }

#ifdef ENABLE_TONES

tonestop:
tonegen:

 goto unimplemented;

#endif /* ENABLE_TONES */

 /************************* Modified! *************************/
files:
  // display a listing of files on the device.
  // version 1: no support for subdirectories

#ifdef ENABLE_FILEIO
  cmd_Files();
  goto warmstart;
#else
  goto unimplemented;
#endif // ENABLE_FILEIO


chain:
  runAfterLoad = true;

load:
  // clear the program
  program_end = program_start;

  // load from a file into memory
#ifdef ENABLE_FILEIO
  {
    unsigned char *filename;

    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;

    String f = Basic_Folder;
    f.concat((char *)filename);
    // Arduino specific
    if( !SPIFFS.exists( f.c_str() ))
    {
      printmsg( FSfilemsg );
    } 
    else {

      fp = SPIFFS.open( (const char *)f.c_str() );
      inStream = kStreamFile;
      inhibitOutput = true;
    }

  }
  goto warmstart;
#else // ENABLE_FILEIO
  goto unimplemented;
#endif // ENABLE_FILEIO



save:
  // save from memory out to a file
#ifdef ENABLE_FILEIO
  {
    unsigned char *filename;

    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;


    String f = Basic_Folder;
    f.concat((char *)filename);
    // remove the old file if it exists
    if( SPIFFS.exists( f.c_str() )) {
      SPIFFS.remove( f.c_str() );
    }
    if(program_start == program_end) goto warmstart;

    // open the file, switch over to file output
    fp = SPIFFS.open( f.c_str(), FILE_WRITE );
    outStream = kStreamFile;

    // copied from "List"
    list_line = findline();
    while(list_line != program_end)
      printline();

    // go back to standard output, close the file
    outStream = kStreamSerial;

    fp.close();

    goto warmstart;
  }
#else // ENABLE_FILEIO
  goto unimplemented;
#endif // ENABLE_FILEIO

/************************* End modified! *************************/

/*********** New Basic Extensions for t-watch-2020 ***************/

#ifdef BasicExtensions    

cls: 
  { 
  if( TextBuff_is_initialized == true )  
   { 
   tb_display_clear();
   TextBuff_is_initialized = false;
   }
  BasicEx_cls();
  }
goto execnextline;


textmode: 
  { 
  if( TextBuff_is_initialized == false )  
   { 
   tb_display_init();
   TextBuff_is_initialized = true;
   }
  BasicEx_cls();
  }
goto execnextline;

line:					
  { 
    short int x0, y0, x1, y1, value= 0;
   //Get the x0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x0 = value;
    else x0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

  //Get the y0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y0 = value;
    else y0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

 //Get the x1
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x1 = value;
    else x1 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

 //Get the y1
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y1 = value;
    else y1 = 0;

    BasicEx_line(x0, y0, x1, y1);
  }
goto execnextline;


box:					
  { 
    short int x0, y0, x1, y1, value= 0;
   //Get the x0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x0 = value;
    else x0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

  //Get the y0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y0 = value;
    else y0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

 //Get the x1
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x1 = value;
    else x1 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

 //Get the y1
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y1 = value;
    else y1 = 0;

    BasicEx_box(x0, y0, x1, y1);
  }
goto execnextline;



circle:					
  { 
      short int x0, y0,  r, value= 0;
   //Get the x0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x0 = value;
    else x0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

  //Get the y0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y0 = value;
    else y0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

  //Get the r
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 120)) r = value;
    else r = 0;

    BasicEx_circle(x0, y0, r);
  }
goto execnextline;

triangle:					
  { 
    short int x0, y0, x1, y1, x2, y2, value= 0;
   //Get the x0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x0 = value;
    else x0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

  //Get the y0
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y0 = value;
    else y0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();


//Get the x1
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x1 = value;
    else x1 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

//Get the y1
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y1 = value;
    else y1 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

 //Get the x2
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x2 = value;
    else x2 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();

 //Get the y2
    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y2 = value;
    else y2 = 0;

    BasicEx_triangle(x0, y0, x1, y1, x2, y2);
  }
goto execnextline;

image:
{
    unsigned char *filename;
    short int x0, y0, value= 0;

    expression_error = 0;
    filename = filenameWord();
    if(expression_error) goto qwhat;
    ignore_blanks();
  
    if (*txtpos != ',') {
		 x0 = 0;
		 y0 = 0;
	    }
    else    {
    txtpos++;
    ignore_blanks();

    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) x0 = value;
    else x0 = 0;
    ignore_blanks();
    if (*txtpos != ',') goto qwhat;
    txtpos++;
    ignore_blanks();


    expression_error = 0;
    value = expression();
    if(expression_error) goto qwhat;
    if((value >= 0)&&(value < 240)) y0 = value;
    else y0 = 0;

 }

    String f = Image_Folder;
    f.concat((char *)filename);
    // Arduino specific
    if( !SPIFFS.exists( f.c_str() ))
    {
      printmsg( FSfilemsg );
    } 
    else {

      
 //  printmsgNoNL(f.c_str() );
   printmsgNoNL( spacemsg );
   printnum(x0);
   printmsgNoNL( spacemsg );
   printnum(y0);
  
     if (f.endsWith(".jpg")) 
         drawJpg((const char *)f.c_str() ,x0, y0);
   	} 
     if (f.endsWith(".bmp")) {
         drawBmp((const char *)f.c_str() ,x0, y0);
	 } 
}
    goto execnextline;







initble:
blesensor:

goto unimplemented;


#endif /* BasicExtensions */

/******************* End Basic Extensions  *******************/
}

// returns 1 if the character is valid in a filename
static int isValidFnChar( char c )
{
  if( c >= '0' && c <= '9' ) return 1; // number
  if( c >= 'A' && c <= 'Z' ) return 1; // LETTER
  if( c >= 'a' && c <= 'z' ) return 1; // letter (for completeness)
  if( c == '_' ) return 1;
  if( c == '+' ) return 1;
  if( c == '.' ) return 1;
  if( c == '~' ) return 1;  // Window~1.txt

  return 0;
}

unsigned char * filenameWord(void)
{
  // SDL - I wasn't sure if this functionality existed above, so I figured i'd put it here
  unsigned char * ret = txtpos;
  expression_error = 0;

  // make sure there are no quotes or spaces, search for valid characters
  //while(*txtpos == SPACE || *txtpos == TAB || *txtpos == SQUOTE || *txtpos == DQUOTE ) txtpos++;
  while( !isValidFnChar( *txtpos )) txtpos++;
  ret = txtpos;

  if( *ret == '\0' ) {
    expression_error = 1;
    return ret;
  }

  // now, find the next nonfnchar
  txtpos++;
  while( isValidFnChar( *txtpos )) txtpos++;
  if( txtpos != ret ) *txtpos = '\0';

  // set the error code if we've got no string
  if( *ret == '\0' ) {
    expression_error = 1;
  }

  return ret;
}

/***************************************************************************/
static void line_terminator(void)
{
  outchar(NL);
  outchar(CR);
}


/***********************************************************/
static unsigned char breakcheck(void)
{

  if(Serial.available())
    return Serial.read() == CTRLC;
  return 0;

}
/***********************************************************/
static int inchar()
{
  int v;

  
  switch( inStream ) {
  case( kStreamFile ):
#ifdef ENABLE_FILEIO
    v = fp.read();
    if( v == NL ) v=CR; // file translate
    if( !fp.available() ) {
      fp.close();
      goto inchar_loadfinish;
    }
    return v;    
#else
#endif
     break;

  case( kStreamSerial ):
  default:
    while(1)
    {
      if(Serial.available())
        return Serial.read();
    }
  }
  
inchar_loadfinish:
  inStream = kStreamSerial;
  inhibitOutput = false;

  if( runAfterLoad ) {
    runAfterLoad = false;
    triggerRun = true;
  }
  return NL; // trigger a prompt.
  

}







/************************* Modified! ***************************/


static void outchar(unsigned char c)
{
  if( inhibitOutput ) return;


  #ifdef ENABLE_FILEIO
    if( outStream == kStreamFile ) {
      // output to a file
      fp.write( c );
    } 
    else
  #endif



 
     Serial.write(c);
   if( TextBuff_is_initialized == true )   tb_display_print_char(c);

}


/* FS helpers */


static int initFS( void )
{
  
  if( FS_is_initialized == true ) return kFS_OK;

  if(!SPIFFS.begin()){
    // failed
    printmsg( FSerrormsg );
    return kFS_Fail;
  }
  // success - quietly return 0
  FS_is_initialized = true;

  // and our file redirection flags
  outStream = kStreamSerial;
  inStream = kStreamSerial;
  inhibitOutput = false;

  return kFS_OK;
}


#if ENABLE_FILEIO
void cmd_Files( void )
{
  File dir = SPIFFS.open( "/" );
  dir.seek(0);

  while( true ) {
    File entry = dir.openNextFile();
    if( !entry ) {
      entry.close();
      break;
    }
    String f = entry.name();
    if(!f.startsWith(Basic_Folder)) continue;
    f.replace(Basic_Folder, "");

    // common header
    printmsgNoNL( indentmsg );
    printmsgNoNL( (const unsigned char *)f.c_str() );
/*    if( entry.iFSirectory() ) {
      printmsgNoNL( slashmsg );
    }

    if( entry.iFSirectory() ) {
      // directory ending
      for( int i=f.length() ; i<16 ; i++ ) {
        printmsgNoNL( spacemsg );
      }
      printmsgNoNL( dirextmsg );
    }
    else {*/
      // file ending
      for( int i=f.length() ; i<17 ; i++ ) {
        printmsgNoNL( spacemsg );
      }
      printUnum( entry.size() );
//    }
    line_terminator();
    entry.close();
  }
  dir.close();
}
#endif


void WatchBasic_init()
{
#ifdef ENABLE_FILEIO
  initFS();
 
#ifdef ENABLE_AUTORUN
  if( SPIFFS.exists( kAutorunFilename )) {
    tb_display_init();                       //
    program_end = program_start;
    fp = SPIFFS.open( kAutorunFilename );
    inStream = kStreamFile;
    inhibitOutput = true;
    runAfterLoad = true;
  }
#endif /* ENABLE_AUTORUN */
#endif /* ENABLE_FILEIO */
}


void Basic()
{
   WatchBasic_init();
   tb_display_init();
   TextBuff_is_initialized = true;
do {
WatchBasic_loop();
} while (Basicstate != -1);
}

/************************* End modified ************************/
