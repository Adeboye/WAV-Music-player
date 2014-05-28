#include "basic_io.h"
#include "LCD.h"
#include "SD_Card.h"
#include "fat.h"
#include "wm8731.h"
#include "inttypes.h"

#include "alt_types.h"  // define types used by Altera code, e.g. alt_u8
#include <stdio.h>
#include <unistd.h>
#include "system.h"  // constants such as the base addresses of any PIOs defined in your hardware
#include "sys/alt_irq.h"  // required when using interrupts
#include <io.h>



#define SIZE 3500
data_file df;
BYTE buffer[512] = {0};
UINT32 length;
int cc[SIZE];
int sector_number = 0;
int lr_counter = 0;
int song = 11;
int16_t buffer_1s[44100];
int end_of_buff = -1;
volatile int stop_flag;  //global variable for stop flag
volatile int play_flag;
volatile int forward_flag = 0;
volatile int backward_flag;
volatile alt_u32 switch_state;
int rising_edge = 0;

void normal_speed()
{

	length = 1+ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));

	LCD_File_Buffering(df.Name);
	build_cluster_chain(cc, length, &df);

	LCD_Display(df.Name,(int)switch_state);


	int j;
	for(j=0; j < length* BPB_SecPerClus; j++)
	{
		get_rel_sector(&df, buffer, cc, j);
		int i;
		for(i = 0; i < 512; )
		{
			UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp

				IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
				//will be processed by the audio CODEC

			i+=2;

			if(stop_flag == 1)
			{
			  IOWR( AUDIO_0_BASE, 0, 0 );
			  stop_flag = 0;
			  return;
			}
		}

	}
}

void half_speed()
{

	length = 1+ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));

	LCD_File_Buffering(df.Name);
	build_cluster_chain(cc, length, &df);
	LCD_Display(df.Name,(int)switch_state);


	int j;
	for(j=0; j < length* BPB_SecPerClus; j++)
	{
		get_rel_sector(&df, buffer, cc, j);
		int i;
		for(i = 0; i < 512; )
		{
			UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full

			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC


			tmp = ( buffer[ i + 3 ] << 8 ) | ( buffer[ i + 2 ] ); //Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			while(IORD( AUD_FULL_BASE, 0 ) ) {}
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC

			//play it again
			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC


			tmp = ( buffer[ i + 3 ] << 8 ) | ( buffer[ i + 2 ] ); //Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			while(IORD( AUD_FULL_BASE, 0 ) ) {}
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC

			i+=4;

		    if(stop_flag == 1)
			{
			  IOWR( AUDIO_0_BASE, 0, 0 );
			  stop_flag = 0;
			  return;
			}
		}

	}

}

void double_speed()
{

	length = 1+ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));

	LCD_File_Buffering(df.Name);
	build_cluster_chain(cc, length, &df);
	LCD_Display(df.Name,(int)switch_state);


	int j;
	for(j=0; j < length* BPB_SecPerClus; j++)
	{
		get_rel_sector(&df, buffer, cc, j);
		int i;
		for(i = 0; i < 512; )
		{
			UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full

			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC


			//To play double speed
			tmp = ( buffer[ i + 3 ] << 8 ) | ( buffer[ i + 2 ] ); //Package 2 8-bit bytes from the
		    //sector buffer array into the
		    //single 16-bit variable tmp
			while(IORD( AUD_FULL_BASE, 0 ) ) {}
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
		    //will be processed by the audio CODEC


			i+=8;
			if(stop_flag == 1)
			{
			  IOWR( AUDIO_0_BASE, 0, 0 );
			  stop_flag = 0;
			  return;
			}
		}
	}

}

void reverse_mode()
{

		length = 1+ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));

		LCD_File_Buffering(df.Name);
		build_cluster_chain(cc, length, &df);
		LCD_Display(df.Name,(int)switch_state);

		int j;
		for(j=length* BPB_SecPerClus; j >0 ; j--)
			{
				get_rel_sector(&df, buffer, cc, j);
				int i;
				for(i = 508; i > 0;)
				{
					UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
					while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
                    tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //Package 2 8-bit bytes from the
					//sector buffer array into the
				    //single 16-bit variable tmp
					IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
				    //will be processed by the audio CODEC

					tmp = ( buffer[ i + 3 ] << 8 ) | ( buffer[ i + 2 ] ); //Package 2 8-bit bytes from the
					//sector buffer array into the
				    //single 16-bit variable tmp
					while(IORD( AUD_FULL_BASE, 0 ) ) {}
					IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO where it
				    //will be processed by the audio CODEC

					i-=4;
					if(stop_flag == 1)
					{
					  IOWR( AUDIO_0_BASE, 0, 0 );
					  stop_flag = 0;
					  return;
					}
				}
			}

}

void delay_mode()
{
	length = 1+ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));


	LCD_File_Buffering(df.Name);
	build_cluster_chain(cc, length, &df);
	LCD_Display(df.Name,(int)switch_state);

	int j;
	for (j = 0; j<length*BPB_SecPerClus;j++)
	{

		get_rel_sector(&df, buffer, cc, j);
        int i;
		for (i = 0; i<512;)
		{
			//Play left 
			UINT16 tmp;
			while(IORD( AUD_FULL_BASE, 0 ) ) {}
			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] );


			IOWR( AUDIO_0_BASE, 0, tmp ); //output

			//play right
			tmp = ( buffer[ i + 3 ] << 8 ) | ( buffer[ i + 2] );

			//check if at the end of physical buffer
			if(end_of_buff < 44100-1)
			{
				end_of_buff++;
			}
			else
			{
				end_of_buff = 0;	//go back to beginning of buffer_1s
			}
		    //sector buffer array into the
		    //single 16-bit variable tmp
			while(IORD( AUD_FULL_BASE, 0 ) ) {}
			IOWR( AUDIO_0_BASE, 0, buffer_1s[end_of_buff] ); //output end_of_buffer
			buffer_1s[end_of_buff] = tmp;

			i+=4;

			if(stop_flag == 1)
			{
			  IOWR( AUDIO_0_BASE, 0, 0 );
			  stop_flag = 0;
			  return;
			}
		}

	}
	int k;
	for(k = 0; k < 44100; k++ )
	{
		if(end_of_buff < 44100-1)
		{
			end_of_buff++;
		}
		else
		{
			end_of_buff = 0;	//go back to beginning of buffer_1s
		}
		while(IORD( AUD_FULL_BASE, 0 ) ) {}
		IOWR( AUDIO_0_BASE, 0, 0 );
		while(IORD( AUD_FULL_BASE, 0 ) ) {}
		IOWR( AUDIO_0_BASE, 0, buffer_1s[end_of_buff] );
	}

}

//Interupt handler to enabletouchpad inputs
static void buttons_interrupts(void* context, alt_u32 id)
{
	if(rising_edge == 0)
	{
	volatile int button_pressed;
	button_pressed = IORD(BUTTON_PIO_BASE,3) & 0xf;
	switch (button_pressed)
	 {
		case 1:
			stop_flag = 1;
		break;
		case 2:
			play_flag = 1;
		break;
		case 4:
			forward_flag = 1;
		break;
		case 8:
			backward_flag = 1;
		break;
	 }
	  rising_edge = 1;
	}
	else if(rising_edge == 1)
	{
		rising_edge = 0;
	}
	// reset edge capture register to clear interrupt
	IOWR(BUTTON_PIO_BASE, 3, 0x0);
}

//Init interupts
static void buttons_init()
{
	/* set up the interrupt vector */
	alt_irq_register( BUTTON_PIO_IRQ, (void*)0, buttons_interrupts);

	/* reset the edge capture register by writing to it (any value will do) */
	IOWR(BUTTON_PIO_BASE, 3, 0x0);

	/* enable interrupts for all four buttons*/
	IOWR(BUTTON_PIO_BASE, 2, 0xf);
}

static void cycle_backwards()
{

	UINT32 temp_size;
	temp_size = df.FileSize;
	int count = 1;
	search_for_filetype("WAV", &df, 0, 1);
	count ++;

	while(df.FileSize != temp_size)
	{
		search_for_filetype("WAV", &df, 0, 1);
		count ++;
	}

	count = count - 1;
	int m = 1;
	while(m < count)
	{
		search_for_filetype("WAV", &df, 0, 1);
		m++;
	}

}
int main(void)
{
     
	SD_card_init();
	init_mbr();
	init_bs();
	init_audio_codec();
	LCD_Init();
	buttons_init();


	switch_state = IORD(SWITCH_PIO_BASE, 0) & 0x7;
	search_for_filetype("WAV", &df, 0, 1);
	LCD_Display(df.Name,(int)switch_state);

	while(1)
	{
		if (forward_flag == 1)
		{
			search_for_filetype("WAV", &df, 0, 1);
			switch_state = IORD(SWITCH_PIO_BASE, 0) & 0x7;
			LCD_Init();
			LCD_Display(df.Name,(int)switch_state);
			forward_flag = 0;
		}

		if(backward_flag == 1)
		{
			cycle_backwards();
			switch_state = IORD(SWITCH_PIO_BASE, 0) & 0x7;
			LCD_Init();
			LCD_Display(df.Name,(int)switch_state);
			backward_flag = 0;
		}

		if(play_flag == 1)
		{
			switch_state = IORD(SWITCH_PIO_BASE, 0) & 0x7;
			LCD_Init();
			LCD_Display(df.Name,(int)switch_state);
			if(switch_state == 1)
			{
				double_speed();
			}
			else if (switch_state == 2)
			{
				half_speed();
			}
			else if (switch_state == 3)
			{
				delay_mode();
			}
			else if (switch_state == 4)
			{
				reverse_mode();
			}
			else
			{
				normal_speed();
			}
			play_flag = 0;

			//effectively disabling interrupts during the song
			forward_flag = 0;
			backward_flag = 0;
		}
	}
	return(0);
}
