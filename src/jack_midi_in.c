#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#include <jack/types.h>
#include <jack/ringbuffer.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <time.h>
#include <unistd.h>

typedef struct JackStuff{
  jack_client_t* client;
  jack_port_t* midi_in_port;
  jack_ringbuffer_t* midi_in_ringbuffer;
  pthread_mutex_t midi_event_thread_lock;
  pthread_cond_t data_ready;
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
      if (space > 3 + sizeof(jack_nframes_t)) {
        int written1 = jack_ringbuffer_write(jack_stuff->midi_in_ringbuffer, (char*)ev.buffer, 3);
        jack_nframes_t current_time_frame = jack_last_frame_time(jack_stuff->client);
        ev.time += current_time_frame;
        int written2 = jack_ringbuffer_write(jack_stuff->midi_in_ringbuffer, (char*)&ev.time, sizeof(jack_nframes_t));
        if (pthread_mutex_trylock (&jack_stuff->midi_event_thread_lock) == 0) {
          pthread_cond_signal (&jack_stuff->data_ready);
          pthread_mutex_unlock (&jack_stuff->midi_event_thread_lock);
        }
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
  pthread_mutex_init(&jack_stuff->midi_event_thread_lock, NULL);
  pthread_cond_init(&jack_stuff->data_ready, NULL);

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
  bool running;
  jack_ringbuffer_t* ringbuffer;
  pthread_mutex_t* midi_event_thread_lock;
  pthread_cond_t* data_ready;
} ThreadStuff;

typedef struct ThreadResult {
  int midi_msg_count;
} ThreadResult;

void* midi_print_thread_fct(void* thread_stuff_raw) {
  ThreadStuff* thread_stuff = (ThreadStuff*) thread_stuff_raw;
  ThreadResult* result = (ThreadResult*)malloc(sizeof( ThreadResult));
  result->midi_msg_count = 0;
  while(result->midi_msg_count < 60 && thread_stuff->running){
    size_t num_bytes = jack_ringbuffer_read_space (thread_stuff->ringbuffer);
    if(num_bytes < 7){
      pthread_cond_wait (thread_stuff->data_ready, thread_stuff->midi_event_thread_lock);
      continue;
    }
    jack_midi_event_t ev;
    ev.buffer = calloc (3, sizeof (jack_midi_data_t));
    size_t received_bytes1 = jack_ringbuffer_read(thread_stuff->ringbuffer, (char*)ev.buffer, 3*sizeof(char));
    size_t received_bytes2 = jack_ringbuffer_read(thread_stuff->ringbuffer, (char*)&ev.time, sizeof(jack_nframes_t));
    ev.size=3;
    printf("received: %d bytes\n", received_bytes1 + received_bytes2);

    if(received_bytes1 + received_bytes2 == 3 + sizeof(jack_nframes_t)){
      uint8_t status_byte = ev.buffer[0] >> 4;
      uint8_t channel = ev.buffer[0] & 0x0f;
      uint8_t param1 = ev.buffer[1] & 0x7f;
      uint8_t param2 = ev.buffer[2] & 0x7f;
      printf("received midi message:\n");
      printf("  status_byte: %d\n", status_byte);
      printf("  channel %d:\n", channel);
      printf("  param1: %d\n", param1);
      printf("  param2: %d\n", param2);
      printf("  time: %d\n", ev.time);
      result->midi_msg_count+=1;
    }
  }
  printf("finish printing thread\n");
  return result;
}

int main(){
  JackStuff* jack_stuff = create_jack_stuff("JackMidiIn");
  pthread_t midi_print_thread;

  ThreadStuff thread_stuff = {
    .running = true,
    .ringbuffer = jack_stuff->midi_in_ringbuffer,
    .midi_event_thread_lock = &jack_stuff->midi_event_thread_lock,
    .data_ready = &jack_stuff->data_ready
  };
  pthread_create(&midi_print_thread, NULL, midi_print_thread_fct,(void *) &thread_stuff);
  // TODO build in signal handler
  sleep(20);
  ThreadResult* result = NULL;
  thread_stuff.running = false;
  if (pthread_mutex_trylock (&jack_stuff->midi_event_thread_lock) == 0) {
    pthread_cond_signal (&jack_stuff->data_ready);
    pthread_mutex_unlock (&jack_stuff->midi_event_thread_lock);
  }

  pthread_join(midi_print_thread, (void**)&result);
  printf("received %d messages\n",result->midi_msg_count);
  jack_stuff_clear(jack_stuff);
  free(result);
}
