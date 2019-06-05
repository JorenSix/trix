/******************************************/
/*
  RTPSpeaker.cpp
  by Joren Six, 2018

  This program decodes Opus encoded audio from an RTP stream. The audio
  is immediately send to a sound device. 
*/
/******************************************/
#include "RtAudio.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <sys/time.h>
#include <ortp/ortp.h>
#include <opus/opus.h>
// uncomment to disable assert()
// #define NDEBUG
#include <cassert>

/*Defines the rtAudio type, request 32bit floats*/
#define FORMAT RTAUDIO_FLOAT32
#define SCALE  1.0;

// Default configuration
int audio_sample_rate= 48000;
int audio_channels =  2;

int opus_bitrate =  112;
int opus_frame_size = 2880;

int udp_port = 5555;
char const *udp_addr = "0.0.0.0";

int rtp_payload_type = 96;

unsigned int rtp_jitter = 16;

OpusDecoder *decoder;
RtpSession *session;

const int buffer_length = 10;
uint8_t packet_buffer[buffer_length][7000] ;
int packet_lengths[buffer_length] ;
int packet_write_index = -1;
int packet_read_index = -1;

void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage:  N fs file <device>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate, \n";
  std::cout << "    device = optional device to use (default = 0),\n";
  exit( 0 );
}

static void timestamp_jump(RtpSession *session, ...)
{
  rtp_session_resync(session);
}

static RtpSession* create_rtp_recv(const char *addr_desc, const int port,
    unsigned int jitter){
  RtpSession *session;

  session = rtp_session_new(RTP_SESSION_RECVONLY);
  rtp_session_set_scheduling_mode(session, TRUE);
  rtp_session_set_blocking_mode(session, FALSE);
  rtp_session_set_local_addr(session, addr_desc, port, -1);
  rtp_session_set_connected_mode(session, FALSE);
  rtp_session_enable_adaptive_jitter_compensation(session, TRUE);
  rtp_session_set_jitter_compensation(session, jitter); /* ms */
  rtp_session_set_time_jump_limit(session, jitter * 16); /* ms */
  if (rtp_session_set_payload_type(session, rtp_payload_type) != 0)
    abort();
  if (rtp_session_signal_connect(session, "timestamp_jump", (RtpCallback) timestamp_jump, 0) != 0)
    abort();
 
  return session;
}

char buf[32768*2];
uint8_t packet[7000];

int ts = 0;

// Interleaved buffers
int output( void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
            double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data ){

  int samples;
  samples =  opus_frame_size;

  int r;

  packet_read_index++;
  packet_read_index = packet_read_index % buffer_length;

  if (packet_read_index == packet_write_index) {
    packet_read_index --;

    r = opus_decode_float(decoder, NULL, 0, (float*)outputBuffer, samples, 1);
    std::cout <<  r << " NULL audio sample frames decoded! " << ts << " ts\n";
  } else {
    int len = packet_lengths[packet_read_index];
    for(int i = 0 ; i <len ; i++){
      packet[i] = packet_buffer[packet_read_index][i];
    }
    r = opus_decode_float(decoder, (const unsigned char*) packet, len, (float*) outputBuffer, samples, 0);     
    std::cout <<  r << " audio sample frames decoded! " << ts << " ts\n";
  }
  if (r < 0) {
    fprintf(stderr, "opus_decode: %s\n", opus_strerror(r));
    return -1;
  }

  return 0;
}

int main( int argc, char *argv[] )
{
  unsigned int bufferFrames, device = 0;

  int decoder_error;
  decoder = opus_decoder_create(audio_sample_rate, audio_channels, &decoder_error);
  if (decoder == NULL) {
    fprintf(stderr, "opus_decoder_create: %s\n",opus_strerror(decoder_error));
    return -1;
  }

  ortp_init();
  ortp_scheduler_init();
  session = create_rtp_recv(udp_addr, udp_port, rtp_jitter);
  assert(session != NULL);

  // minimal command-line checking
  if ( argc < 4 || argc > 6 ){
    usage();
  }

  RtAudio dac;
  if ( dac.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 0 );
  }

  audio_channels = (unsigned int) atoi( argv[1]) ;
  audio_sample_rate = (unsigned int) atoi( argv[2] );
  if ( argc > 3 ){
    device = (unsigned int) atoi( argv[3] );
  }

  // Set our stream parameters for output only.
  bufferFrames = opus_frame_size;
  RtAudio::StreamParameters oParams;
  oParams.deviceId = device;
  oParams.nChannels = audio_channels;

  if ( device == 0 ){
    oParams.deviceId = dac.getDefaultOutputDevice();
  }

  RtAudio::StreamOptions options;
  options.streamName = "AudioToRTP";
  options.numberOfBuffers = 0; // Use default.
  options.flags = RTAUDIO_SCHEDULE_REALTIME;
  options.priority = 70;

  std::cout <<" Waiting for messages\n";

  /*
  float *pcm;
  pcm = (float*) alloca(sizeof(float) * samples * audio_channels);
  */

  bool started = false;

  for (;;) {
    int r, have_more;

    ts += opus_frame_size;
  
    r = rtp_session_recv_with_ts(session, (uint8_t*)buf, sizeof(buf), ts, &have_more);
    assert(r >= 0);
    assert(have_more == 0);
  
    if(r>0)
      std::cout <<  r << " bytes recieved! " << ts << " ts\n";
    if(have_more>0)
      std::cout <<  have_more << " has more recieved! " << ts << " ts\n";
  
    if (r != 0) {
      packet_write_index++;
      packet_write_index = packet_write_index % buffer_length;

      std::cout <<  packet_write_index << " packet_write_index " <<  packet_read_index  << " read index \n";

      if(!started && packet_write_index == buffer_length/2){
        try {
          dac.openStream( &oParams, NULL, FORMAT, audio_sample_rate, &bufferFrames, &output,  &options  );
          dac.startStream();
        } catch ( RtAudioError& e ) {
          std::cout << '\n' << e.getMessage() << '\n' << std::endl;
          goto cleanup;
        }
        started = true;
      }

      for(int i = 0 ; i < r ; i++){
        packet_buffer[packet_write_index][i] = buf[i];
      }
      packet_lengths[packet_write_index] = r;
    }

  }

  cleanup:
  dac.closeStream();
  rtp_session_destroy(session);
  ortp_exit();
  ortp_global_stats_display();

  return 0;
}