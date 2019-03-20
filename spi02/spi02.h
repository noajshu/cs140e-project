void show_text(unsigned int col, char *s);
void show_text_hex(unsigned int col, unsigned int x);
void hex_screen ( unsigned int x );
void ClearScreen ( void );
void oled_init(void);
void spi_init(void);
void SetPageStart ( unsigned int x );
void SetColumn ( unsigned int x );