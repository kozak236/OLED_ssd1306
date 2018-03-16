/* Includes ------------------------------------------------------------------*/
#include "SSD1306.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SSD1306_CMD    0
#define SSD1306_DAT    1

#define SSD1306_WIDTH    128
#define SSD1306_HEIGHT   64
uint8_t byte_send = 0;


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t s_chDispalyBuffer[128][8];
uint8_t DMA_oled_refresh[4] = {0x00, 0xB0, 0x02, 0x10};
uint8_t DMA_oled_clear[4] = {0x00, 0xB0, 0x02, 0x10};
const uint8_t DMA_oled_init[29] = {0x00, 0xAE, 0x00, 0x10, 0x40, 0x81, 0xCF, 0xA1, 0xC0, 0xA6, 0xA8, 0x3f, 0xD3, 0x00, 0xd5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB, 0x40, 0x20, 0x02, 0x8D, 0x14, 0xA4, 0xA6, 0xAF};
const uint8_t DMA_oled_on[4] = {0x00, 0x8D, 0x14, 0xAF};
const uint8_t DMA_oled_off[4] = {0x00, 0x8D, 0x10, 0xAE};
volatile uint8_t DMA_refresh_send = 0;
volatile uint8_t refresh_configured = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Refreshs the graphic ram
  *
  * @param  None
  *
  * @retval  None
**/

void ssd1306_refresh_gram(void)
{
	DMA_state_vector[0] = 1;

	if (refresh_configured == 0)
	{
		DMA_state_vector[1] = 0;
		refresh_configured = 1;
		DMA_refresh_data_0[0] = 0x40;
		DMA_refresh_data_1[0] = 0x40;
		DMA_refresh_data_2[0] = 0x40;
		DMA_refresh_data_3[0] = 0x40;
		DMA_refresh_data_4[0] = 0x40;
		DMA_refresh_data_5[0] = 0x40;
		DMA_refresh_data_6[0] = 0x40;
		DMA_refresh_data_7[0] = 0x40;

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_0[i] = s_chDispalyBuffer[i-1][0];

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_1[i] = s_chDispalyBuffer[i-1][1];

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_2[i] = s_chDispalyBuffer[i-1][2];

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_3[i] = s_chDispalyBuffer[i-1][3];

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_4[i] = s_chDispalyBuffer[i-1][4];

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_5[i] = s_chDispalyBuffer[i-1][5];

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_6[i] = s_chDispalyBuffer[i-1][6];

		for (uint8_t i=1; i<129; i++)
			DMA_refresh_data_7[i] = s_chDispalyBuffer[i-1][7];
	}

	if (DMA_refresh_send == 0)
	{
		DMA_oled_refresh[1] = 0xB0 + DMA_state_vector[1];
		DMA_Send_Buffer(DMA_oled_refresh, DMA_oled_refresh_size);
		DMA_refresh_send = 1;
	}
	else
	{
		switch(DMA_state_vector[1])
		{
		case 0:
			DMA_Send_Buffer(DMA_refresh_data_0, DMA_refresh_data_size);
			break;
		case 1:
			DMA_Send_Buffer(DMA_refresh_data_1, DMA_refresh_data_size);
			break;
		case 2:
			DMA_Send_Buffer(DMA_refresh_data_2, DMA_refresh_data_size);
			break;
		case 3:
			DMA_Send_Buffer(DMA_refresh_data_3, DMA_refresh_data_size);
			break;
		case 4:
			DMA_Send_Buffer(DMA_refresh_data_4, DMA_refresh_data_size);
			break;
		case 5:
			DMA_Send_Buffer(DMA_refresh_data_5, DMA_refresh_data_size);
			break;
		case 6:
			DMA_Send_Buffer(DMA_refresh_data_6, DMA_refresh_data_size);
			break;
		case 7:
			DMA_Send_Buffer(DMA_refresh_data_7, DMA_refresh_data_size);
			break;
		default:
			DMA_state_vector[0] = 0;
			DMA_state_vector[1] = 0;
			refresh_configured = 0;
			command_count++;
			program();
			break;
		}
		DMA_refresh_send = 0;
		DMA_state_vector[1]++;
	}
}


/**
  * @brief   Clears the screen
  *
  * @param  None
  *
  * @retval  None
**/

void ssd1306_clear_screen(uint8_t chFill)
{
	DMA_state_vector[0] = 2;
	fill = chFill;

	if(DMA_state_vector[2] < 8)
	{
		DMA_oled_refresh[1] = 0xB0 + DMA_state_vector[2];
		DMA_state_vector[2]++;
		DMA_Send_Buffer(DMA_oled_refresh, DMA_oled_refresh_size);
	}
	else
	{
		for (uint8_t i = 0; i < 8; i++)
			for (uint8_t j = 0; j < 128; j ++)
				s_chDispalyBuffer[j][i] = chFill;

		DMA_state_vector[0] = 0;
		DMA_state_vector[2] = 0;

		command_count++;
		program();
	}
}

/**
  * @brief  Draws a piont on the screen
  *
  * @param  chXpos: Specifies the X position
  * @param  chYpos: Specifies the Y position
  * @param  chPoint: 0: the point turns off    1: the piont turns on
  *
  * @retval None
**/

void ssd1306_draw_point(uint8_t chXpos, uint8_t chYpos, uint8_t chPoint)
{
	uint8_t chPos, chBx, chTemp = 0;

	if (chXpos > 127 || chYpos > 63) {
		return;
	}
	chPos = 7 - chYpos / 8; //
	chBx = chYpos % 8;
	chTemp = 1 << (7 - chBx);

	if (chPoint) {
		s_chDispalyBuffer[chXpos][chPos] |= chTemp;

	} else {
		s_chDispalyBuffer[chXpos][chPos] &= ~chTemp;
	}
}

/**
  * @brief  Fills a rectangle
  *
  * @param  chXpos1: Specifies the X position 1 (X top left position)
  * @param  chYpos1: Specifies the Y position 1 (Y top left position)
  * @param  chXpos2: Specifies the X position 2 (X bottom right position)
  * @param  chYpos3: Specifies the Y position 2 (Y bottom right position)
  *
  * @retval
**/

void ssd1306_fill_screen(uint8_t chXpos1, uint8_t chYpos1, uint8_t chXpos2, uint8_t chYpos2, uint8_t chDot)
{
	uint8_t chXpos, chYpos;

	for (chXpos = chXpos1; chXpos <= chXpos2; chXpos ++) {
		for (chYpos = chYpos1; chYpos <= chYpos2; chYpos ++) {
			ssd1306_draw_point(chXpos, chYpos, chDot);
		}
	}

	ssd1306_refresh_gram();
}


/**
  * @brief Displays one character at the specified position
  *
  * @param  chXpos: Specifies the X position
  * @param  chYpos: Specifies the Y position
  * @param  chSize:
  * @param  chMode
  * @retval
**/
void ssd1306_display_char(uint8_t chXpos, uint8_t chYpos, uint8_t chChr, uint8_t chSize, uint8_t chMode)
{
	uint8_t i, j;
	uint8_t chTemp, chYpos0 = chYpos;

	chChr = chChr - ' ';
    for (i = 0; i < chSize; i ++) {
		if (chSize == 12) {
			if (chMode) {
				chTemp = c_chFont1206[chChr][i];
			} else {
				chTemp = ~c_chFont1206[chChr][i];
			}
		} else {
			if (chMode) {
				chTemp = c_chFont1608[chChr][i];
			} else {
				chTemp = ~c_chFont1608[chChr][i];
			}
		}

        for (j = 0; j < 8; j ++) {
			if (chTemp & 0x80) {
				ssd1306_draw_point(chXpos, chYpos, 1);
			} else {
				ssd1306_draw_point(chXpos, chYpos, 0);
			}
			chTemp <<= 1;
			chYpos ++;

			if ((chYpos - chYpos0) == chSize) {
				chYpos = chYpos0;
				chXpos ++;
				break;
			}
		}
    }
}
static uint32_t pow(uint8_t m, uint8_t n)
{
	uint32_t result = 1;
	while(n --) result *= m;
	return result;
}


void ssd1306_display_num(uint8_t chXpos, uint8_t chYpos, uint32_t chNum, uint8_t chLen, uint8_t chSize)
{
	uint8_t i;
	uint8_t chTemp, chShow = 0;

	for(i = 0; i < chLen; i ++) {
		chTemp = (chNum / pow(10, chLen - i - 1)) % 10;
		if(chShow == 0 && i < (chLen - 1)) {
			if(chTemp == 0) {
				ssd1306_display_char(chXpos + (chSize / 2) * i, chYpos, ' ', chSize, 1);
				continue;
			} else {
				chShow = 1;
			}
		}
	 	ssd1306_display_char(chXpos + (chSize / 2) * i, chYpos, chTemp + '0', chSize, 1);
	}
}


/**
  * @brief  Displays a string on the screen
  *
  * @param  chXpos: Specifies the X position
  * @param  chYpos: Specifies the Y position
  * @param  pchString: Pointer to a string to display on the screen
  *
  * @retval  None
**/

void ssd1306_display_string(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchString, uint8_t chSize, uint8_t chMode)
{
    while (*pchString != '\0') {
        if (chXpos > (SSD1306_WIDTH - chSize / 2)) {
			chXpos = 0;
			chYpos += chSize;
			if (chYpos > (SSD1306_HEIGHT - chSize)) {
				chYpos = chXpos = 0;
				ssd1306_clear_screen(0x00);
			}
		}

        ssd1306_display_char(chXpos, chYpos, *pchString, chSize, chMode);
        chXpos += chSize / 2;
        pchString ++;
    }
}

void ssd1306_draw_1616char(uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
	uint8_t i, j;
	uint8_t chTemp = 0, chYpos0 = chYpos, chMode = 0;

	for (i = 0; i < 32; i ++) {
		chTemp = c_chFont1612[chChar - 0x30][i];
		for (j = 0; j < 8; j ++) {
			chMode = chTemp & 0x80? 1 : 0;
			ssd1306_draw_point(chXpos, chYpos, chMode);
			chTemp <<= 1;
			chYpos ++;
			if ((chYpos - chYpos0) == 16) {
				chYpos = chYpos0;
				chXpos ++;
				break;
			}
		}
	}
}

void ssd1306_draw_3216char(uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
	uint8_t i, j;
	uint8_t chTemp = 0, chYpos0 = chYpos, chMode = 0;

	for (i = 0; i < 64; i ++) {
		chTemp = c_chFont3216[chChar - 0x30][i];
		for (j = 0; j < 8; j ++) {
			chMode = chTemp & 0x80? 1 : 0;
			ssd1306_draw_point(chXpos, chYpos, chMode);
			chTemp <<= 1;
			chYpos ++;
			if ((chYpos - chYpos0) == 32) {
				chYpos = chYpos0;
				chXpos ++;
				break;
			}
		}
	}
}

void ssd1306_draw_bitmap(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchBmp, uint8_t chWidth, uint8_t chHeight)
{
	uint16_t i, j, byteWidth = (chWidth + 7) / 8;

    for(j = 0; j < chHeight; j ++){
        for(i = 0; i < chWidth; i ++ ) {
            if(*(pchBmp + j * byteWidth + i / 8) & (128 >> (i & 7))) {
                ssd1306_draw_point(chXpos + i, chYpos + j, 1);
            }
        }
    }
}

void program()
{
	switch (command_count)
	{
	case 0:
		screen_init = 0;
		DMA_Send_Buffer(DMA_oled_on, DMA_oled_on_off_size);
		break;
	case 1:
		ssd1306_clear_screen(0x00);
		break;
	case 2:
		ssd1306_refresh_gram();
		break;
	case 3:
		ssd1306_clear_screen(0xFF);
		break;
	case 4:
		ssd1306_refresh_gram();
		break;
	case 5:
		ssd1306_clear_screen(0x00);
		break;
	case 6:
		ssd1306_refresh_gram();
		break;
	case 7:
		ssd1306_display_string(18, 0, "1.3inch OLED", 16, 1);
		ssd1306_display_string(0, 16, "0LED", 16, 1);
		ssd1306_refresh_gram();
		break;
	case 8:
		ssd1306_clear_screen(0x00);
		break;
	case 9:
		ssd1306_refresh_gram();
		break;
	case 10:
		ssd1306_draw_bitmap(0, 2, &c_chSingal816[0], 16, 8);
		ssd1306_draw_bitmap(24, 2, &c_chBluetooth88[0], 8, 8);
		ssd1306_draw_bitmap(40, 2, &c_chMsg816[0], 16, 8);
		ssd1306_draw_bitmap(64, 2, &c_chGPRS88[0], 8, 8);
		ssd1306_draw_bitmap(90, 2, &c_chAlarm88[0], 8, 8);
		ssd1306_draw_bitmap(112, 2, &c_chBat816[0], 16, 8);

		ssd1306_draw_3216char(0,16, '2');
		ssd1306_draw_3216char(16,16, '3');
		ssd1306_draw_3216char(32,16, ':');
		ssd1306_draw_3216char(48,16, '5');
		ssd1306_draw_3216char(64,16, '6');
		ssd1306_draw_1616char(80,32, ':');
		ssd1306_draw_1616char(96,32, '4');
		ssd1306_draw_1616char(112,32, '7');
		ssd1306_draw_bitmap(87, 16, &c_chBmp4016[0], 40, 16);

		ssd1306_display_string(0, 52, "MUSIC", 12, 0);
		ssd1306_display_string(52, 52, "MENU", 12, 0);
		ssd1306_display_string(98, 52, "PHONE", 12, 0);

		ssd1306_refresh_gram();
		break;
	default:
		break;
	}
}

extern void DMA_Send_Buffer(uint8_t buffer[], uint16_t buffer_size)
{
	dma.DMA_MemoryBaseAddr = (uint32_t)buffer;
	dma.DMA_BufferSize = buffer_size;
	DMA_Init(DMA1_Channel6, &dma);

	DMA_Cmd(DMA1_Channel6, ENABLE);
	I2C_DMACmd(I2C1, ENABLE);
	I2C_GenerateSTART(I2C1, ENABLE);
}

/*-------------------------------END OF FILE-------------------------------*/
