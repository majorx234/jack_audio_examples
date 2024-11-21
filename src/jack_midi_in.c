#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <jack/types.h>
#include <jack/ringbuffer.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <threads.h>
#include <unistd.h>

typedef struct JackStuff{
  jack_client_t* client;
  jack_port_t* midi_in_port;
  jack_ringbuffer_t* midi_in_ringbuffer;
} JackStuff;

int process(jack_nframes_t nframes, void* jack_stuff_raw)
{
  JackStuff* jack_stuff = (JackStuff*)jack_stuff_raw;
  // midi_in event handling:
  void* jack_midi_in_buffer =  jack_port_get_buffer ( jack_stuff->midi_in_port, nframes);
  int event_count = jack_midi_get_event_count(jack_midi_in_buffer);
  jack_nframes_t last_frame_time = jack_last_frame_time(jack_stuff->client);

  for (int i = 0;i<event_count;i++) {
    jack_midi_event_t ev;
    bool ok  = false;
    int read = jack_midi_event_get(&ev, jack_midi_in_buffer, i);
    if(ev.size == 3) {
      size_t space = jack_ringbuffer_write_space(jack_stuff->midi_in_ringbuffer);
      if (space>3) {
        int written = jack_ringbuffer_write(jack_stuff->midi_in_ringbuffer, (char*)&ev, 3);
      }
    }
  }
  return 0;
}

JackStuff* create_jack_stuff(char* client_name){
  JackStuff* jack_stuff = (JackStuff*)malloc(sizeof(JackStuff));

  jack_stuff->midi_in_port = NULL;
  jack_stuff->midi_in_ringbuffer = NULL;
  jack_stuff->client = NULL;

  jack_stuff->client = jack_client_open (client_name,
                                         JackNullOption,
                                         0,
                                         0 );
  const size_t ringbuffer_size = 4096 * sizeof(float);
  jack_stuff->midi_in_port  = jack_port_register (jack_stuff->client,
                                                  "midi_input",
                                                  JACK_DEFAULT_MIDI_TYPE,
                                                  JackPortIsInput, 0);
  jack_stuff->midi_in_ringbuffer = jack_ringbuffer_create(1024);

  jack_set_process_callback(jack_stuff->client, process, (void*)jack_stuff);
  //client.set_sample_rate(48000);
  jack_activate(jack_stuff->client);
  return jack_stuff;
}

void jack_stuff_clear(JackStuff* jack_stuff) {
  jack_deactivate(jack_stuff->client);
  jack_client_close(jack_stuff->client);
  jack_ringbuffer_free(jack_stuff->midi_in_ringbuffer);
  free(jack_stuff);
}

typedef struct ThreadStuff {
  bool* running;
  jack_ringbuffer_t* ringbuffer;
} ThreadStuff;

typedef struct ThreadResult {
  int midi_msg_count;
} ThreadResult;

void* midi_print_thread_fct(void* thread_stuff_raw) {
  ThreadStuff* thread_stuff = (ThreadStuff*) thread_stuff_raw;
  ThreadResult* result = (ThreadResult*)malloc(sizeof( ThreadResult));
  result->midi_msg_count = 0;
  while(result->midi_msg_count < 20 && *(thread_stuff->running)){
    
  }
  return result;
}

int main(){
  JackStuff* jack_stuff = create_jack_stuff("JackMidiIn");
  pthread_t midi_print_thread;
  bool running = true;
  ThreadStuff thread_stuff = {
    .running = &running,
    .ringbuffer = jack_stuff->midi_in_ringbuffer
  };
  pthread_create(&midi_print_thread, NULL, midi_print_thread_fct,(void *) &thread_stuff);
  sleep(20);
  ThreadResult* result = NULL;
  pthread_join(midi_print_thread, (void**)&result);
  jack_stuff_clear(jack_stuff);
  printf("received %d messages\n",result->midi_msg_count);
  free(result);
}
