# jack_sine_out.c
- generates a sinewave on output
- uses a jack_ringbuffer object between audio generation and output
  - decouple sound generation (maybe non realtime) and output (realtime)
  - normal useful to have generator tread
- ![Alt text](documentation/images/jack_sine_out.jpg?raw=true "overview over ringbuffer interaction")
## ringbuffer pattern
### in generator thread
```C
// check if data low in ringbuffer (->need for new data to generate)
if (num_bytes < low_limit * sizeof(float)) {
  gen_signal_in_buf(my_data_buf, ...);
  jack_ringbuffer_write (jack_stuff.ringbuffer, (void *) my_data_buf, 1024*sizeof(float));
  ...
```
### in process function
```C
// check if data available
if(num_bytes >= (nframes* sizeof(float))) {
  jack_ringbuffer_read(jack_stuff->ringbuffer, (char*)outputBuffer, nframes * sizeof(float));
  ...
```
