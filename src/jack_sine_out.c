#include <jack/types.h>
#include <stdio.h>
#include <raylib.h>
#include <math.h>
#include <jack/ringbuffer.h>
#include <unistd.h>
#include <jack/jack.h>
#include <math.h>

typedef struct {
  float freq;
  float time_step;
} Oscilator;

typedef struct JackStuff{
  jack_port_t* out_port;
  jack_ringbuffer_t* ringbuffer;
} JackStuff;

void gen_signal_in_buf(float* buf, size_t buf_length, Oscilator* osc) {
  for(size_t i = 0; i < buf_length; ++i) {
    buf[i] = sinf(2*PI*osc->freq*(osc->time_step/48000.0));
    osc->time_step += 1;
  }
}

int process(jack_nframes_t nframes, void* jack_stuff_raw)
{
  JackStuff* jack_stuff = (JackStuff*)jack_stuff_raw;
  float* outputBuffer= (float*)jack_port_get_buffer ( jack_stuff->out_port, nframes);

  if(jack_stuff->ringbuffer){
    size_t num_bytes = jack_ringbuffer_read_space (jack_stuff->ringbuffer);
    if(num_bytes > (nframes* sizeof(float))) {
      jack_ringbuffer_read(jack_stuff->ringbuffer, (char*)outputBuffer, nframes * sizeof(float));
    } else {
      for ( int i = 0; i < (int) nframes; i++)
      {
        outputBuffer[i] = 0.0;
      }
    }
  }
  return 0;
}


int main(void) {
  JackStuff jack_stuff = {
    .out_port = 0,
    .out_port = NULL
  };

  jack_client_t* client = jack_client_open ("JackSineOut",
                                            JackNullOption,
                                            0,
                                            0 );
  jack_stuff.out_port = jack_port_register ( client,
                                    "output",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsOutput,
                                    0 );

  size_t my_size = 192000 * sizeof(float);
  jack_stuff.ringbuffer = jack_ringbuffer_create (my_size);

  jack_set_process_callback  (client, process , (void*)&jack_stuff);

   jack_activate(client);

  float my_data_buf[1024];

  Oscilator my_osci = {.freq = 440, .time_step = 0};

  size_t counter = 0;
  while(counter < 48000 * 10 ){
    size_t num_bytes = jack_ringbuffer_read_space (jack_stuff.ringbuffer);
    if (num_bytes < 96000 * sizeof(float)) {
      gen_signal_in_buf(my_data_buf, 1024, &my_osci);
      jack_ringbuffer_write (jack_stuff.ringbuffer, (void *) my_data_buf, 1024*sizeof(float));
      counter += 1024;
    } else {
      sleep(1);
    }
  }

  jack_deactivate(client);
  jack_client_close(client);
  jack_ringbuffer_free(jack_stuff.ringbuffer);
  return 0;
}
