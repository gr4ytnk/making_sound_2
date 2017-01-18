#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <time.h>

#include <alsa/asoundlib.h>

// gcc -o alsa_sample alsa_sample.c -lasound -lm

// 現在時刻を返す
double getTime() {
  struct timespec time_spec;
  clock_getres( CLOCK_REALTIME, &time_spec );
  return (double)( time_spec.tv_sec ) + (double)( time_spec.tv_nsec ) * 0.000001;
}

// 指定された時刻までスリープ
void waitUntil( double _until ) {
  struct timespec until;
  until.tv_sec = (time_t)( _until );
  until.tv_nsec = (long)( ( _until - (time_t)( _until ) ) * 1000000.0 );
  clock_nanosleep( CLOCK_REALTIME, TIMER_ABSTIME, &until, NULL );
}

// ここから本体
int main() {

  // 0番目のサウンドデバイス
  const static char device[] = "hw:0";

  // 符号付き16bit
  const static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

  // snd_pcm_readi/snd_pcm_writeiを使って読み書きする
  const static snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;

  // 再生周波数
  const static unsigned int sampling_rate = 48000;

  // モノラル
//  const static unsigned int channels = 1;
  // ステレオ 
  const static unsigned int channels = 2;

  // ALSAがサウンドカードの都合に合わせて勝手に再生周波数を変更することを許す
  const static unsigned int soft_resample = 1;

  // 20ミリ秒分ALSAのバッファに蓄える
  const static unsigned int latency = 20000;

  // 流す音(Hz)
  const static unsigned int tone_freq = 523;

  // 流す時間
  const static unsigned int length = 1;

  snd_pcm_t *pcm;

  // 再生用PCMストリームを開く
  if( snd_pcm_open( &pcm, device, SND_PCM_STREAM_PLAYBACK, 0 ) ) {
    printf( "Unable to open." );
    abort();
  }

  // 再生周波数、フォーマット、バッファのサイズ等を指定する
  if(
    snd_pcm_set_params(
      pcm, format, access,
      channels, sampling_rate, soft_resample, latency
    )
  ) {
    printf( "Unable to set format." );
    snd_pcm_close( pcm );
    abort();
  }

  // 再生するためのサイン波を作っておく
  const unsigned int buffer_size = sampling_rate / tone_freq;

  int16_t buffer[ buffer_size ];

  int buffer_iter;

  for( buffer_iter = 0; buffer_iter != buffer_size; buffer_iter++ ){
    buffer[ buffer_iter ] = sin( 2.0 * M_PI * buffer_iter / buffer_size ) * 32767;
  }

  double system_time = getTime();
  unsigned int beep_counter = 0;

  for( beep_counter = 0; beep_counter != tone_freq * length; beep_counter++ ) {

    printf("[DEBUG]count: %u\n", beep_counter);

    // とりあえず1周期分だけ書き出す
    int write_result = snd_pcm_writei ( pcm, ( const void* )buffer, buffer_size );

    if( write_result < 0 ) {

      // バッファアンダーラン等が発生してストリームが停止した時は回復を試みる
      if( snd_pcm_recover( pcm, write_result, 0 ) < 0 ) {
        printf( "Unable to recover this stream." );
        snd_pcm_close( pcm );
        abort();
      }
    }

    printf("----------[into sleep]----------\n");

    // 少し待ってからまた1周期分書き出す
    // TODO：変数化する
    waitUntil( system_time + (double)buffer_size / 48000.0f );

    system_time = getTime();

  } // end of for


  // 終わったらストリームを閉じる
  snd_pcm_close( pcm );
}
