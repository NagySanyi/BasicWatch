
#ifndef Extensions_TWatch2020_H_
#define Extensions_TWatch2020_H_

#define BasicExtensions 1		

void BasicEx_cls();
void BasicEx_triangle(short int x0, short int y0, short int x1, short int y1, short int x2, short int y2);
void BasicEx_box(short int x0, short int y0, short int x1, short int y1);
void BasicEx_line(short int x0, short int y0, short int x1, short int y1);
void BasicEx_circle(short int x0, short int y0, short int r);

// BasicEx_image(unsigned char filename, short int x0, short int y0);

void drawBmp(const char *filename, short int x, short int y);
void drawJpg(const char *filename, short int x, short int y);


// Based on tb_display* Hague Nusseck @ electricidea
// https://github.com/electricidea/M5StickC-TB_Display

//extern boolean tb_display_word_wrap;
void tb_display_init(void);
void tb_display_show();
void tb_display_clear();
void tb_display_new_line();
void tb_display_print_char(unsigned char  data);

#endif /* Extensions_TWatch2020_H_ */ 
