/******************************************/
/*
  RTPMicrophone.cpp
  by Joren Six, 2018

  This program records audio from a sound device, encodes the
  audio using the Opus encoder and sends it over an RTP session
  to a configured host.
*/
/******************************************/
#include "RtAudio.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <ortp/ortp.h>
#include <opus/opus.h>
// uncomment to disable assert()
// #define NDEBUG
#include <cassert>

/*Defines the rtAudio type, request 32bit floats*/
#define FORMAT RTAUDIO_FLOAT32

// Default configuration
int audio_sample_rate= 48000;
int audio_channels =  2;

int opus_bitrate =  112;
int opus_frame_size = 2880;

int udp_port = 5555;
char const *udp_addr = "81.11.161.28"; 

int rtp_payload_type = 96;


//the rtp session object
RtpSession *session;

// The opus encoder 
OpusEncoder *encoder;

// The number of bytes per opus frame
int bytes_per_frame;

unsigned int sample_frame_time_stamp  = 0;

//FILE *test_fd;

static RtpSession* create_rtp_send(const char *addr_desc, const int port){
  RtpSession *session;

  session = rtp_session_new(RTP_SESSION_SENDONLY);

  assert(session != NULL);

  rtp_session_set_scheduling_mode(session, 0);
  rtp_session_set_blocking_mode(session, 0);
  rtp_session_set_connected_mode(session, FALSE);

  if (rtp_session_set_remote_addr(session, addr_desc, port) != 0)
    abort();

  //use payload type 96 = dynamic payload type
  if (rtp_session_set_payload_type(session, rtp_payload_type) != 0)
    abort();

  if (rtp_session_set_multicast_ttl(session, 16) != 0)
    abort();

  return session;
}


void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage:rtp_microphone N fs <device>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate,\n";
  std::cout << "    device = optional device to use (default = 0),\n";
  exit( 0 );
}

// Interleaved buffers
int input( void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data ){

  unsigned char* packet;
  ssize_t packet_length_in_bytes = 0; 

  //allocate a packet on the stack: max bytes =  bytes per frame
  packet = ( unsigned char*) alloca(bytes_per_frame);
  
  /* Read from file */
  //fseek(test_fd, sample_frame_time_stamp * sizeof( float ) * audio_channels , SEEK_SET);
  //fread(inputBuffer, audio_channels * sizeof( float ), opus_frame_size, test_fd);

  //The length of the encoded packet (in bytes) on success or a negative error code (see Error codes) on failure.
  packet_length_in_bytes = opus_encode_float(encoder, (const float*) inputBuffer, opus_frame_size, packet, bytes_per_frame);
  if (packet_length_in_bytes < 0) {
    fprintf(stderr, "opus_encode_float: %s\n", opus_strerror(packet_length_in_bytes));
    return -1;
  }
  rtp_session_send_with_ts(session, packet , packet_length_in_bytes, sample_frame_time_stamp);

  //increment sample frame counter
  sample_frame_time_stamp = sample_frame_time_stamp + opus_frame_size;

  std::cout << sample_frame_time_stamp <<"\n";

  return 0;
}


int main( int argc, char *argv[] )
{
  unsigned int device = 0;

  // minimal command-line checking
  if ( argc < 3 || argc > 6 ) usage();

  RtAudio adc;
  if ( adc.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 1 );
  }

  audio_channels = (unsigned int) atoi( argv[1] );

  audio_sample_rate = (unsigned int) atoi( argv[2] );

  if ( argc > 3 )
    device = (unsigned int) atoi( argv[3] );

  // Let RtAudio print messages to stderr.
  adc.showWarnings( true );

  // Set our stream parameters for input only.
  RtAudio::StreamParameters iParams;
  if ( device == 0 )
    iParams.deviceId = adc.getDefaultInputDevice();
  else
    iParams.deviceId = device;

  iParams.nChannels = audio_channels;
  iParams.firstChannel = 0;

  RtAudio::StreamOptions options;
  options.streamName = "AudioToRTP";
  options.numberOfBuffers = 0; // Use default.
  options.flags = RTAUDIO_SCHEDULE_REALTIME;
  options.priority = 70;
  options.flags |= RTAUDIO_MINIMIZE_LATENCY;

  char const *addr = udp_addr; 
  int port = udp_port;
  ortp_init();
  ortp_scheduler_init();
  //ortp_set_log_level_mask(ORTP_WARNING|ORTP_ERROR);
  session = create_rtp_send(addr, port);
  assert(session != NULL);

  int encoder_error;

  encoder = opus_encoder_create(audio_sample_rate, audio_channels, OPUS_APPLICATION_AUDIO, &encoder_error);
  if (encoder == NULL) {
    fprintf(stderr, "opus_encoder_create: %s\n",
      opus_strerror(encoder_error));
    return -1;
  }

  bytes_per_frame = opus_bitrate * 1024 * opus_frame_size / audio_sample_rate / 8; 

  //test_fd = fopen( "waste_f_mono_48000.raw", "rb" ); 

  unsigned int audio_block_size = 1 * opus_frame_size;

  try {
    adc.openStream( NULL, &iParams, FORMAT, audio_sample_rate, &audio_block_size, &input,&options);
    std::cout << audio_block_size << "  internal buffer size in sample frames,\n";
    std::cout << adc.getStreamLatency() << "  Stream latency,\n";
  } catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

  try {
    adc.startStream();
  } catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

  while ( adc.isStreamRunning() ) {
    sleep( 100 ); // wake every 100 ms to check if we're done
  }

  rtp_session_destroy(session);
  ortp_exit();
  ortp_global_stats_display();

  opus_encoder_destroy(encoder);

 cleanup:
  if ( adc.isStreamOpen() ) adc.closeStream();

  return 0;
}
