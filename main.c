/* 
  A Minimal Capture Program
  This program opens an audio interface for capture, configures it for
  stereo, 16 bit, 44.1kHz, interleaved conventional read/write
  access. Then its reads a chunk of random data from it, and exits. It
  isn't meant to be a real program.
  From on Paul David's tutorial : http://equalarea.com/paul/alsa-audio.html
  Fixes rate and buffer problems
  sudo apt-get install libasound2-dev
  gcc -o alsa-record-example -lasound alsa-record-example.c && ./alsa-record-example hw:0
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <time.h>

static unsigned int average = 0;

void update_rolling_avg(int size, int start, int end, unsigned short* buff, unsigned int rate)
{
  int i = start;
  
  do 
  {
    if( i >= (size*2) )
      i = 0;
      
    unsigned int sum = 0;
  
    //Calculate the rolling average
    int n;
    
    if( (i-size+1) < 0 )
      n = size+1-i;
    else
      n = i-size+1;
  
    do
    {
      if( n >= (2*size) )
        n = 0;
      
      sum += buff[n];

    }while( n++ != i );
  	
    average = sum/size;
		//printf("average is: %d\n", average);
		//Slep for 1/rate in us
		//usleep(5);

  } while( i++ != end );

}

int main (int argc, char *argv[])
{
  int err;
  unsigned short *buffer, *b1, *b2;
  int buffer_frames = 12800; //Was 1024
  unsigned int rate = 12800;
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

	//Open the hardware device
  if ((err = snd_pcm_open (&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) 
	{
    fprintf (stderr, "cannot open audio device %s (%s)\n", 
             argv[1],
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "audio interface opened\n");

 	//Allocate memory to the hw parameters structure (Invalid values)
  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
	{
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params allocated\n");
 	
	//Fill params with a full configuration space for a pcm, this means accept all hardware parameters
  if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0)
	{
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params initialized\n");
	
	//Set for reading interleaved buffer
  if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
    fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params access setted\n");
	
	//Signed 16 little endian
  if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0)
	{
    fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params format setted\n");
	
	//44KHz
  if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0)
	{
    fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
    exit (1);
  }
	
  fprintf(stdout, "hw_params rate setted\n");

	//1 channel
  if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params channels setted\n");

	//Period size
  if ((err = snd_pcm_hw_params_set_period_size (capture_handle, hw_params, buffer_frames, 0)) < 0) {

	fprintf (stderr, "cannot set channel period size (%s)\n",
             snd_strerror (err));
		exit(1);
	}

	fprintf(stdout,"period size set\n");
	
	// Install the hardware configuration, brings the device to "prepared" state
  if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params setted\n");
	
	//Free the allocated memory space
  snd_pcm_hw_params_free (hw_params);

  fprintf(stdout, "hw_params freed\n");
	
	//
  if ((err = snd_pcm_prepare (capture_handle)) < 0) 
	{
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "audio interface prepared\n");

	//Dual length buffer to support mimicking rolling buffer
  buffer = malloc(buffer_frames * snd_pcm_format_width(format) / 8 * 2);
	b1 = buffer;
	b2 = buffer + buffer_frames;

  fprintf(stdout, "buffer allocated\n");

	//Init
	//Replace = 0
	int replace = 0;
	//Fill b1 and b2
	int size1 = snd_pcm_readi (capture_handle, b1, buffer_frames);
	int size2 = snd_pcm_readi (capture_handle, b2, buffer_frames);
	if (size1 != buffer_frames || size2 != buffer_frames) 
	{
	  fprintf (stderr, "read from audio interface failed (%s)\n",
	           err, snd_strerror (err));
	  exit (1);
	}

	//Infinite loop
	while(1)
	{
		
		//If replace = 0
		if( replace == 0 )
		{
			//Calculate rolling average from beginning of b2 to end of b2
			update_rolling_avg(buffer_frames, buffer_frames-1, (buffer_frames*2)-1, buffer, rate);
			//Fill b1 with new read
			size1 = snd_pcm_readi (capture_handle, b1, buffer_frames);
			//replace = 1
			replace = 1;
		}	
		//Otherwise, if replace = 1
		else if ( replace == 1 )
		{
			//Calculate rolling average from end of b2 to end of b1
			update_rolling_avg(buffer_frames, (buffer_frames*2)-1, buffer_frames-1, buffer, rate);
			//Fill b2 with new read
			size2 = snd_pcm_readi (capture_handle, b2, buffer_frames);
			//replace = 0
			replace = 0;
		}

		printf("average is: %d\n", average);
	}

  free(buffer);

  fprintf(stdout, "buffer freed\n");
	
  snd_pcm_close (capture_handle);
  fprintf(stdout, "audio interface closed\n");

  exit (0);
}
