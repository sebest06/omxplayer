/*
 *
 *      Copyright (C) 2012 Edgar Hucek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <string.h>

#define AV_NOWARN_DEPRECATED

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

#include "OMXStreamInfo.h"

#include "utils/log.h"

#include "DllAvUtil.h"
#include "DllAvFormat.h"
#include "DllAvFilter.h"
#include "DllAvCodec.h"
#include "linux/RBP.h"

#include "OMXVideo.h"
#include "OMXAudioCodecOMX.h"
#include "utils/PCMRemap.h"
#include "OMXClock.h"
#include "OMXAudio.h"
#include "OMXReader.h"
#include "OMXPlayerVideo.h"
#include "OMXPlayerAudio.h"
#include "OMXPlayerSubtitles.h"
#include "DllOMX.h"
#include "Srt.h"
#include "KeyConfig.h"
#include "utils/Strprintf.h"
#include "tcpcliente.h"

#include <string>
#include <utility>

#include "version.h"

// when we repeatedly seek, rather than play continuously
#define TRICKPLAY(speed) (speed < 0 || speed > 4 * DVD_PLAYSPEED_NORMAL)

//#define DISPLAY_TEXT(text, ms) if(m_osd) m_player_subtitles.DisplayText(text, ms)

//#define DISPLAY_TEXT_SHORT(text) DISPLAY_TEXT(text, 1000)
//#define DISPLAY_TEXT_LONG(text) DISPLAY_TEXT(text, 2000)

typedef enum {CONF_FLAGS_FORMAT_NONE, CONF_FLAGS_FORMAT_SBS, CONF_FLAGS_FORMAT_TB } FORMAT_3D_T;
enum PCMChannels  *m_pChannelMap        = NULL;
volatile sig_atomic_t g_abort           = false;
bool              m_passthrough         = false;
long              m_Volume              = 0;
long              m_Amplification       = 0;
bool              m_Deinterlace         = false;
bool              m_NoDeinterlace       = false;
bool              m_NativeDeinterlace   = false;
OMX_IMAGEFILTERANAGLYPHTYPE m_anaglyph  = OMX_ImageFilterAnaglyphNone;
bool              m_HWDecode            = false;
std::string       deviceString          = "";
int               m_use_hw_audio        = false;
bool              m_osd                 = false;
bool              m_no_keys             = false;
std::string       m_external_subtitles_path;
bool              m_has_external_subtitles = false;
std::string       m_font_path           = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";
std::string       m_italic_font_path    = "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf";
std::string       m_dbus_name           = "org.mpris.MediaPlayer2.omxplayer";
bool              m_asked_for_font      = false;
bool              m_asked_for_italic_font = false;
float             m_font_size           = 0.055f;
bool              m_centered            = false;
bool              m_ghost_box           = true;
unsigned int      m_subtitle_lines      = 3;
bool              m_Pause               = false;
OMXReader         m_omx_reader;
int               m_audio_index_use     = 0;
bool              m_thread_player       = false;
OMXClock          *m_av_clock           = NULL;
COMXStreamInfo    m_hints_audio;
COMXStreamInfo    m_hints_video;
OMXPacket         *m_omx_pkt            = NULL;
bool              m_hdmi_clock_sync     = false;
bool              m_no_hdmi_clock_sync  = false;
bool              m_stop                = false;
int               m_subtitle_index      = -1;
DllBcmHost        m_BcmHost;
OMXPlayerVideo    m_player_video;
OMXPlayerAudio    m_player_audio;
OMXPlayerSubtitles  m_player_subtitles;
int               m_tv_show_info        = 0;
bool              m_has_video           = false;
bool              m_has_audio           = false;
bool              m_has_subtitle        = false;
float             m_display_aspect      = 0.0f;
bool              m_boost_on_downmix    = true;
bool              m_gen_log             = false;
bool              m_loop                = false;
int               m_layer               = 0;


  bool                  m_stats               = false;
  bool                  m_refresh             = false;
  TV_DISPLAY_STATE_T   tv_state;
  COMXCore              g_OMX;
  CRBP                  g_RBP;

void do_exit();
enum{ERROR=-1,SUCCESS,ONEBYTE};

enum Controles{
    playVideo = 1,
    stopVideo = 4,
    pauseVideo = 8,
};
char* buffer;


void inicializate(void)
{
//enum PCMChannels  *m_pChannelMap        = NULL;
g_abort               = false;
m_passthrough         = false;
m_Volume              = 0;
m_Amplification       = 0;
m_Deinterlace         = false;
m_NoDeinterlace       = false;
m_NativeDeinterlace   = false;
m_anaglyph            = OMX_ImageFilterAnaglyphNone;
m_HWDecode            = false;
deviceString          = "";
m_use_hw_audio        = false;
m_osd                 = false;
m_no_keys             = false;
m_has_external_subtitles = false;
m_font_path           = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";
m_italic_font_path    = "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf";
m_dbus_name           = "org.mpris.MediaPlayer2.omxplayer";
m_asked_for_font      = false;
m_asked_for_italic_font = false;
m_font_size           = 0.055f;
m_centered            = false;
m_ghost_box           = true;
m_subtitle_lines      = 3;
m_Pause               = false;
m_audio_index_use     = 0;
m_thread_player       = false;
m_hdmi_clock_sync     = false;
m_no_hdmi_clock_sync  = false;
m_stop                = false;
m_subtitle_index      = -1;
m_tv_show_info        = 0;
m_has_video           = false;
m_has_audio           = false;
m_has_subtitle        = false;
m_display_aspect      = 0.0f;
m_boost_on_downmix    = true;
m_gen_log             = false;
m_loop                = false;
m_layer               = 0;
m_stats               = false;
m_refresh             = false;

}
void sig_handler(int s)
{
  if (s==SIGINT && !g_abort)
  {
     signal(SIGINT, SIG_DFL);
     g_abort = true;
     return;
  }
  signal(SIGABRT, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
  signal(SIGFPE, SIG_DFL);
  abort();
}

static void FlushStreams(double pts);

static void SetSpeed(int iSpeed)
{
  if(!m_av_clock)
    return;

  m_omx_reader.SetSpeed(iSpeed);

  // flush when in trickplay mode
  if (TRICKPLAY(iSpeed) || TRICKPLAY(m_av_clock->OMXPlaySpeed()))
    FlushStreams(DVD_NOPTS_VALUE);

  m_av_clock->OMXSetSpeed(iSpeed);
}

static float get_display_aspect_ratio(HDMI_ASPECT_T aspect)
{
  float display_aspect;
  switch (aspect) {
    case HDMI_ASPECT_4_3:   display_aspect = 4.0/3.0;   break;
    case HDMI_ASPECT_14_9:  display_aspect = 14.0/9.0;  break;
    case HDMI_ASPECT_16_9:  display_aspect = 16.0/9.0;  break;
    case HDMI_ASPECT_5_4:   display_aspect = 5.0/4.0;   break;
    case HDMI_ASPECT_16_10: display_aspect = 16.0/10.0; break;
    case HDMI_ASPECT_15_9:  display_aspect = 15.0/9.0;  break;
    case HDMI_ASPECT_64_27: display_aspect = 64.0/27.0; break;
    default:                display_aspect = 16.0/9.0;  break;
  }
  return display_aspect;
}

static float get_display_aspect_ratio(SDTV_ASPECT_T aspect)
{
  float display_aspect;
  switch (aspect) {
    case SDTV_ASPECT_4_3:  display_aspect = 4.0/3.0;  break;
    case SDTV_ASPECT_14_9: display_aspect = 14.0/9.0; break;
    case SDTV_ASPECT_16_9: display_aspect = 16.0/9.0; break;
    default:               display_aspect = 4.0/3.0;  break;
  }
  return display_aspect;
}

static void FlushStreams(double pts)
{
  m_av_clock->OMXStop();
  m_av_clock->OMXPause();

  if(m_has_video)
    m_player_video.Flush();

  if(m_has_audio)
    m_player_audio.Flush();

  if(pts != DVD_NOPTS_VALUE)
    m_av_clock->OMXMediaTime(0.0);


  if(m_omx_pkt)
  {
    m_omx_reader.FreePacket(m_omx_pkt);
    m_omx_pkt = NULL;
  }
}

static void CallbackTvServiceCallback(void *userdata, uint32_t reason, uint32_t param1, uint32_t param2)
{
  sem_t *tv_synced = (sem_t *)userdata;
  switch(reason)
  {
  case VC_HDMI_UNPLUGGED:
    break;
  case VC_HDMI_STANDBY:
    break;
  case VC_SDTV_NTSC:
  case VC_SDTV_PAL:
  case VC_HDMI_HDMI:
  case VC_HDMI_DVI:
    // Signal we are ready now
    sem_post(tv_synced);
    break;
  default:
     break;
  }
}

void SetVideoMode(int width, int height, int fpsrate, int fpsscale, FORMAT_3D_T is3d)
{
  int32_t num_modes = 0;
  int i;
  HDMI_RES_GROUP_T prefer_group;
  HDMI_RES_GROUP_T group = HDMI_RES_GROUP_CEA;
  float fps = 60.0f; // better to force to higher rate if no information is known
  uint32_t prefer_mode;

  if (fpsrate && fpsscale)
    fps = DVD_TIME_BASE / OMXReader::NormalizeFrameduration((double)DVD_TIME_BASE * fpsscale / fpsrate);

  //Supported HDMI CEA/DMT resolutions, preferred resolution will be returned
  TV_SUPPORTED_MODE_NEW_T *supported_modes = NULL;
  // query the number of modes first
  int max_supported_modes = m_BcmHost.vc_tv_hdmi_get_supported_modes_new(group, NULL, 0, &prefer_group, &prefer_mode);

  if (max_supported_modes > 0)
    supported_modes = new TV_SUPPORTED_MODE_NEW_T[max_supported_modes];

  if (supported_modes)
  {
    num_modes = m_BcmHost.vc_tv_hdmi_get_supported_modes_new(group,
        supported_modes, max_supported_modes, &prefer_group, &prefer_mode);

    if(m_gen_log) {
    CLog::Log(LOGDEBUG, "EGL get supported modes (%d) = %d, prefer_group=%x, prefer_mode=%x\n",
        group, num_modes, prefer_group, prefer_mode);
    }
  }

  TV_SUPPORTED_MODE_NEW_T *tv_found = NULL;

  if (num_modes > 0 && prefer_group != HDMI_RES_GROUP_INVALID)
  {
    uint32_t best_score = 1<<30;
    uint32_t scan_mode = m_NativeDeinterlace;

    for (i=0; i<num_modes; i++)
    {
      TV_SUPPORTED_MODE_NEW_T *tv = supported_modes + i;
      uint32_t score = 0;
      uint32_t w = tv->width;
      uint32_t h = tv->height;
      uint32_t r = tv->frame_rate;

      /* Check if frame rate match (equal or exact multiple) */
      if(fabs(r - 1.0f*fps) / fps < 0.002f)
  score += 0;
      else if(fabs(r - 2.0f*fps) / fps < 0.002f)
  score += 1<<8;
      else
  score += (1<<16) + (1<<20)/r; // bad - but prefer higher framerate

      /* Check size too, only choose, bigger resolutions */
      if(width && height)
      {
        /* cost of too small a resolution is high */
        score += max((int)(width -w), 0) * (1<<16);
        score += max((int)(height-h), 0) * (1<<16);
        /* cost of too high a resolution is lower */
        score += max((int)(w-width ), 0) * (1<<4);
        score += max((int)(h-height), 0) * (1<<4);
      }

      // native is good
      if (!tv->native)
        score += 1<<16;

      // interlace is bad
      if (scan_mode != tv->scan_mode)
        score += (1<<16);

      // wanting 3D but not getting it is a negative
      if (is3d == CONF_FLAGS_FORMAT_SBS && !(tv->struct_3d_mask & HDMI_3D_STRUCT_SIDE_BY_SIDE_HALF_HORIZONTAL))
        score += 1<<18;
      if (is3d == CONF_FLAGS_FORMAT_TB  && !(tv->struct_3d_mask & HDMI_3D_STRUCT_TOP_AND_BOTTOM))
        score += 1<<18;

      // prefer square pixels modes
      float par = get_display_aspect_ratio((HDMI_ASPECT_T)tv->aspect_ratio)*(float)tv->height/(float)tv->width;
      score += fabs(par - 1.0f) * (1<<12);

      /*printf("mode %dx%d@%d %s%s:%x par=%.2f score=%d\n", tv->width, tv->height,
             tv->frame_rate, tv->native?"N":"", tv->scan_mode?"I":"", tv->code, par, score);*/

      if (score < best_score)
      {
        tv_found = tv;
        best_score = score;
      }
    }
  }

  if(tv_found)
  {
    char response[80];
    printf("Output mode %d: %dx%d@%d %s%s:%x\n", tv_found->code, tv_found->width, tv_found->height,
           tv_found->frame_rate, tv_found->native?"N":"", tv_found->scan_mode?"I":"", tv_found->code);
    if (m_NativeDeinterlace && tv_found->scan_mode)
      vc_gencmd(response, sizeof response, "hvs_update_fields %d", 1);

    // if we are closer to ntsc version of framerate, let gpu know
    int ifps = (int)(fps+0.5f);
    bool ntsc_freq = fabs(fps*1001.0f/1000.0f - ifps) < fabs(fps-ifps);
    vc_gencmd(response, sizeof response, "hdmi_ntsc_freqs %d", ntsc_freq);

    /* inform TV of any 3D settings. Note this property just applies to next hdmi mode change, so no need to call for 2D modes */
    HDMI_PROPERTY_PARAM_T property;
    property.property = HDMI_PROPERTY_3D_STRUCTURE;
    property.param1 = HDMI_3D_FORMAT_NONE;
    property.param2 = 0;
    if (is3d != CONF_FLAGS_FORMAT_NONE)
    {
      if (is3d == CONF_FLAGS_FORMAT_SBS && tv_found->struct_3d_mask & HDMI_3D_STRUCT_SIDE_BY_SIDE_HALF_HORIZONTAL)
        property.param1 = HDMI_3D_FORMAT_SBS_HALF;
      else if (is3d == CONF_FLAGS_FORMAT_TB && tv_found->struct_3d_mask & HDMI_3D_STRUCT_TOP_AND_BOTTOM)
        property.param1 = HDMI_3D_FORMAT_TB_HALF;
      m_BcmHost.vc_tv_hdmi_set_property(&property);
    }

    printf("ntsc_freq:%d %s%s\n", ntsc_freq, property.param1 == HDMI_3D_FORMAT_SBS_HALF ? "3DSBS":"", property.param1 == HDMI_3D_FORMAT_TB_HALF ? "3DTB":"");
    sem_t tv_synced;
    sem_init(&tv_synced, 0, 0);
    m_BcmHost.vc_tv_register_callback(CallbackTvServiceCallback, &tv_synced);
    int success = m_BcmHost.vc_tv_hdmi_power_on_explicit_new(HDMI_MODE_HDMI, (HDMI_RES_GROUP_T)group, tv_found->code);
    if (success == 0)
      sem_wait(&tv_synced);
    m_BcmHost.vc_tv_unregister_callback(CallbackTvServiceCallback);
    sem_destroy(&tv_synced);
  }
  if (supported_modes)
    delete[] supported_modes;
}

static int get_mem_gpu(void)
{
   char response[80] = "";
   int gpu_mem = 0;
   if (vc_gencmd(response, sizeof response, "get_mem gpu") == 0)
      vc_gencmd_number_property(response, "gpu", &gpu_mem);
   return gpu_mem;
}

int main(int argc, char *argv[])
{
  signal(SIGSEGV, sig_handler);
  signal(SIGABRT, sig_handler);
  signal(SIGFPE, sig_handler);
  signal(SIGINT, sig_handler);

  printf("%s\n",argv[1]);

  TcpCliente cliente;
  int n;
  cliente.conn(argv[1] , 30666);
  cliente.send_data("/visor pantalla1");

  bool                  m_send_eos            = false;
  bool                  m_packet_after_seek   = false;
  bool                  m_seek_flush          = false;
  bool                  m_new_win_pos         = false;
  std::string           m_filename;
  double                m_incr                = 0;
  double                m_loop_from           = 0;
  bool                  m_dump_format         = false;
  bool                  m_dump_format_exit    = false;
  FORMAT_3D_T           m_3d                  = CONF_FLAGS_FORMAT_NONE;
  double                startpts              = 0;
  CRect                 DestRect              = {0,0,0,0};
  bool sentStarted = false;
  float audio_fifo_size = 0.0; // zero means use default
  float video_fifo_size = 0.0;
  float audio_queue_size = 0.0;
  float video_queue_size = 0.0;
  float m_threshold      = -1.0f; // amount of audio/video required to come out of buffering
  float m_timeout        = 10.0f; // amount of time file/network operation can stall for before timing out
  int m_orientation      = -1; // unset
  float m_fps            = 0.0f; // unset
  bool m_live            = false; // set to true for live tv or vod for low buffering
  enum PCMLayout m_layout = PCM_LAYOUT_2_0;
  //TV_DISPLAY_STATE_T   tv_state;
  double last_seek_pos = 0;
  bool idle = false;

  #define S(x) (int)(DVD_PLAYSPEED_NORMAL*(x))
  int playspeeds[] = {S(0), S(1/16.0), S(1/8.0), S(1/4.0), S(1/2.0), S(0.975), S(1.0), S(1.125), S(-32.0), S(-16.0), S(-8.0), S(-4), S(-2), S(-1), S(1), S(2.0), S(4.0), S(8.0), S(16.0), S(32.0)};
  const int playspeed_slow_min = 0, playspeed_slow_max = 7, playspeed_rew_max = 8, playspeed_rew_min = 13, playspeed_normal = 14, playspeed_ff_min = 15, playspeed_ff_max = 19;
  int playspeed_current = playspeed_normal;
  double m_last_check_time = 0.0;
  float m_latency = 0.0f;
  int c;
  std::string mode;

  /*if (optind >= argc) {
    print_usage();
    return 0;
  }*/
//Lee el archivo por primera vez

  int gpu_mem = get_mem_gpu();
  int min_gpu_mem = 64;
  if (gpu_mem > 0 && gpu_mem < min_gpu_mem)
  {
    printf("Only %dM of gpu_mem is configured. Try running \"sudo raspi-config\" and ensure that \"memory_split\" has a value of %d or greater\n", gpu_mem, min_gpu_mem);
  }

  double now;
  bool update;
  buffer = new char[100];

    const CStdString m_musicExtensions = ".nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.mod|.amf|.669|.dmf|.dsm|.far|.gdm|"
                 ".imf|.it|.m15|.med|.okt|.s3m|.stm|.sfx|.ult|.uni|.xm|.sid|.ac3|.dts|.cue|.aif|.aiff|.wpl|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.rar|"
                 ".wv|.nsf|.spc|.gym|.adx|.dsp|.adp|.ymf|.ast|.afc|.hps|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.mid|.kar|.sap|"
                 ".cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url|.pxml|.tta|.rss|.cm3|.cms|.dlt|.brstm|.wtv|.mka";

bool m_audio_extension;



      // get display aspect
  TV_DISPLAY_STATE_T current_tv_state;
  memset(&current_tv_state, 0, sizeof(TV_DISPLAY_STATE_T));
  m_BcmHost.vc_tv_get_display_state(&current_tv_state);
  if(current_tv_state.state & ( VC_HDMI_HDMI | VC_HDMI_DVI )) {
    //HDMI or DVI on
    m_display_aspect = get_display_aspect_ratio((HDMI_ASPECT_T)current_tv_state.display.hdmi.aspect_ratio);
  } else {
    //composite on
    m_display_aspect = get_display_aspect_ratio((SDTV_ASPECT_T)current_tv_state.display.sdtv.display_options.aspect);
  }
  m_display_aspect *= (float)current_tv_state.display.hdmi.height/(float)current_tv_state.display.hdmi.width;

  if (m_orientation >= 0)
    m_hints_video.orientation = m_orientation;

while(1)
{

m_send_eos            = false;
m_packet_after_seek   = false;
m_seek_flush          = false;
m_new_win_pos         = false;
m_incr                = 0;
m_loop_from           = 0;
m_dump_format         = false;
m_dump_format_exit    = false;
m_3d                  = CONF_FLAGS_FORMAT_NONE;
startpts              = 0;
DestRect              = {0,0,0,0};
sentStarted = false;
audio_fifo_size = 0.0; // zero means use default
video_fifo_size = 0.0;
audio_queue_size = 0.0;
video_queue_size = 0.0;
m_threshold      = -1.0f; // amount of audio/video required to come out of buffering
m_timeout        = 10.0f; // amount of time file/network operation can stall for before timing out
m_orientation      = -1; // unset
m_fps            = 0.0f; // unset
m_live            = false; // set to true for live tv or vod for low buffering
PCMLayout m_layout = PCM_LAYOUT_2_0;
  //TV_DISPLAY_STATE_T   tv_state;
last_seek_pos = 0;
idle = false;

playspeed_current = playspeed_normal;
m_last_check_time = 0.0;
m_latency = 0.0f;



    inicializate();

  m_filename = "";//argv[optind];

    memset(buffer, 0, sizeof buffer);
    n = (cliente.recibir(buffer,100));
    if(n>0){
        //printf("size %d, %s\n",n,buffer+1);
        //m_filename = "../../videos/Micayala_DivX1080p_ASP.divx";
        if(buffer[0]==playVideo)
        {
        m_filename = buffer + 1;
        m_filename = m_filename.substr(0,n-1);
        }
    }
    if(n==0){return 1;}



if(m_filename != "")
{
  printf("%s\n",m_filename.c_str()); //Imprimo el nombre del archivo por el tema del path

  m_audio_extension = false;
  if (m_filename.find_last_of(".") != string::npos)
  {
    CStdString extension = m_filename.substr(m_filename.find_last_of("."));
    if (!extension.IsEmpty() && m_musicExtensions.Find(extension.ToLower()) != -1)
      m_audio_extension = true;
  }

  g_RBP.Initialize();
  g_OMX.Initialize();


  m_av_clock = new OMXClock();


  m_thread_player = true;


  if(!m_omx_reader.Open(m_filename.c_str(), m_dump_format, m_live, m_timeout))
  {
    do_exit();
    continue;
    //return 1;
  }

  if (m_dump_format_exit)
  {
      do_exit();
      //return 1;
  }

  m_has_video     = m_omx_reader.VideoStreamCount();
  m_has_audio     = m_audio_index_use < 0 ? false : m_omx_reader.AudioStreamCount();

  if (m_audio_extension)
  {
    CLog::Log(LOGWARNING, "%s - Ignoring video in audio filetype:%s", __FUNCTION__, m_filename.c_str());
    m_has_video = false;
  }

  // you really don't want want to match refresh rate without hdmi clock sync
  if ((m_refresh || m_NativeDeinterlace) && !m_no_hdmi_clock_sync)
    m_hdmi_clock_sync = true;


  if(!m_av_clock->OMXInitialize())
  {
    do_exit();
    //return 1;
  }

  if(m_hdmi_clock_sync && !m_av_clock->HDMIClockSync())
  {
    do_exit();
    //return 1;
  }

  m_av_clock->OMXStateIdle();
  m_av_clock->OMXStop();
  m_av_clock->OMXPause();
  m_omx_reader.GetHints(OMXSTREAM_AUDIO, m_hints_audio);
  m_omx_reader.GetHints(OMXSTREAM_VIDEO, m_hints_video);
  if (m_fps > 0.0f)
    m_hints_video.fpsrate = m_fps * DVD_TIME_BASE, m_hints_video.fpsscale = DVD_TIME_BASE;

  if(m_audio_index_use > 0)
    m_omx_reader.SetActiveStream(OMXSTREAM_AUDIO, m_audio_index_use-1);
  if(m_has_video && m_refresh)
  {
    memset(&tv_state, 0, sizeof(TV_DISPLAY_STATE_T));
    m_BcmHost.vc_tv_get_display_state(&tv_state);

    SetVideoMode(m_hints_video.width, m_hints_video.height, m_hints_video.fpsrate, m_hints_video.fpsscale, m_3d);
  }

  if(m_has_video && !m_player_video.Open(m_hints_video, m_av_clock, DestRect, m_Deinterlace ? VS_DEINTERLACEMODE_FORCE:m_NoDeinterlace ? VS_DEINTERLACEMODE_OFF:VS_DEINTERLACEMODE_AUTO,
                                         m_anaglyph, m_hdmi_clock_sync, m_thread_player, m_display_aspect, m_layer, video_queue_size, video_fifo_size))
                                         {
                                             do_exit();
                                                //return 1;
                                         }


  m_omx_reader.GetHints(OMXSTREAM_AUDIO, m_hints_audio);


  if (deviceString == "")
  {
    if (m_BcmHost.vc_tv_hdmi_audio_supported(EDID_AudioFormat_ePCM, 2, EDID_AudioSampleRate_e44KHz, EDID_AudioSampleSize_16bit ) == 0)
      deviceString = "omx:hdmi";
    else
      deviceString = "omx:local";
  }


  if ((m_hints_audio.codec == CODEC_ID_AC3 || m_hints_audio.codec == CODEC_ID_EAC3) &&
      m_BcmHost.vc_tv_hdmi_audio_supported(EDID_AudioFormat_eAC3, 2, EDID_AudioSampleRate_e44KHz, EDID_AudioSampleSize_16bit ) != 0)
    m_passthrough = false;
  if (m_hints_audio.codec == CODEC_ID_DTS &&
      m_BcmHost.vc_tv_hdmi_audio_supported(EDID_AudioFormat_eDTS, 2, EDID_AudioSampleRate_e44KHz, EDID_AudioSampleSize_16bit ) != 0)
    m_passthrough = false;

  if(m_has_audio && !m_player_audio.Open(m_hints_audio, m_av_clock, &m_omx_reader, deviceString,
                                         m_passthrough, m_use_hw_audio,
                                         m_boost_on_downmix, m_thread_player, m_live, m_layout, audio_queue_size, audio_fifo_size))
                                         {
                                             do_exit();
                                             //return 1;
                                         }

  if(m_has_audio)
  {
    m_player_audio.SetVolume(pow(10, m_Volume / 2000.0));
    if (m_Amplification)
      m_player_audio.SetDynamicRangeCompression(m_Amplification);
  }
  if (m_threshold < 0.0f)
    m_threshold = m_live ? 0.7f : 0.2f;

  m_av_clock->OMXReset(m_has_video, m_has_audio);
  m_av_clock->OMXStateExecute();
  sentStarted = true;

  while(!m_stop)
  {


    if(g_abort)
    {
        do_exit();
        //return 1;
    }

    now = m_av_clock->GetAbsoluteClock();
    update = false;

    if (m_last_check_time == 0.0 || m_last_check_time + DVD_MSEC_TO_TIME(20) <= now)
    {
      update = true;
      m_last_check_time = now;
    }
     if (update) {
    // Agregado
    memset(buffer, 0, sizeof buffer);
    n = (cliente.recibir(buffer,100));
    if(n>0){
        printf("size %d, %s\n",n,buffer);
    }
    if(n==0){return 1;}
    switch(buffer[0])
    {
    case pauseVideo:
        m_Pause = !m_Pause;
        if (m_av_clock->OMXPlaySpeed() != DVD_PLAYSPEED_NORMAL && m_av_clock->OMXPlaySpeed() != DVD_PLAYSPEED_PAUSE)
        {
          printf("resume\n");
          playspeed_current = playspeed_normal;
          SetSpeed(playspeeds[playspeed_current]);
          m_seek_flush = true;
        }
        break;
    case stopVideo:
        m_stop = true;
        //do_exit();
        //return 1;
        break;
    }
        //
    }

    if (idle)
    {
      usleep(10000);
      continue;
    }

    /* player got in an error state */
    if(m_player_audio.Error())
    {
      printf("audio player error. emergency exit!!!\n");
      do_exit();
      //return 1;
    }

    if (update)
    {
      /* when the video/audio fifos are low, we pause clock, when high we resume */
      double stamp = m_av_clock->OMXMediaTime();
      double audio_pts = m_player_audio.GetCurrentPTS();
      double video_pts = m_player_video.GetCurrentPTS();

      if (0 && m_av_clock->OMXIsPaused())
      {
        double old_stamp = stamp;
        if (audio_pts != DVD_NOPTS_VALUE && (stamp == 0 || audio_pts < stamp))
          stamp = audio_pts;
        if (video_pts != DVD_NOPTS_VALUE && (stamp == 0 || video_pts < stamp))
          stamp = video_pts;
        if (old_stamp != stamp)
        {
          m_av_clock->OMXMediaTime(stamp);
          stamp = m_av_clock->OMXMediaTime();
        }
      }

      float audio_fifo = audio_pts == DVD_NOPTS_VALUE ? 0.0f : audio_pts / DVD_TIME_BASE - stamp * 1e-6;
      float video_fifo = video_pts == DVD_NOPTS_VALUE ? 0.0f : video_pts / DVD_TIME_BASE - stamp * 1e-6;
      float threshold = std::min(0.1f, (float)m_player_audio.GetCacheTotal() * 0.1f);
      bool audio_fifo_low = false, video_fifo_low = false, audio_fifo_high = false, video_fifo_high = false;

      if(m_stats)
      {
        static int count;
        if ((count++ & 7) == 0)
           printf("M:%8.0f V:%6.2fs %6dk/%6dk A:%6.2f %6.02fs/%6.02fs Cv:%6dk Ca:%6dk                            \r", stamp,
               video_fifo, (m_player_video.GetDecoderBufferSize()-m_player_video.GetDecoderFreeSpace())>>10, m_player_video.GetDecoderBufferSize()>>10,
               audio_fifo, m_player_audio.GetDelay(), m_player_audio.GetCacheTotal(),
               m_player_video.GetCached()>>10, m_player_audio.GetCached()>>10);
      }

      if(m_tv_show_info)
      {
        static unsigned count;
        if ((count++ & 7) == 0)
        {
          char response[80];
          if (m_player_video.GetDecoderBufferSize() && m_player_audio.GetCacheTotal())
            vc_gencmd(response, sizeof response, "render_bar 4 video_fifo %d %d %d %d",
                (int)(100.0*m_player_video.GetDecoderBufferSize()-m_player_video.GetDecoderFreeSpace())/m_player_video.GetDecoderBufferSize(),
                (int)(100.0*video_fifo/m_player_audio.GetCacheTotal()),
                0, 100);
          if (m_player_audio.GetCacheTotal())
            vc_gencmd(response, sizeof response, "render_bar 5 audio_fifo %d %d %d %d",
                (int)(100.0*audio_fifo/m_player_audio.GetCacheTotal()),
                (int)(100.0*m_player_audio.GetDelay()/m_player_audio.GetCacheTotal()),
                0, 100);
          vc_gencmd(response, sizeof response, "render_bar 6 video_queue %d %d %d %d",
                m_player_video.GetLevel(), 0, 0, 100);
          vc_gencmd(response, sizeof response, "render_bar 7 audio_queue %d %d %d %d",
                m_player_audio.GetLevel(), 0, 0, 100);
        }
      }

      if (audio_pts != DVD_NOPTS_VALUE)
      {
        audio_fifo_low = m_has_audio && audio_fifo < threshold;
        audio_fifo_high = !m_has_audio || (audio_pts != DVD_NOPTS_VALUE && audio_fifo > m_threshold);
      }
      if (video_pts != DVD_NOPTS_VALUE)
      {
        video_fifo_low = m_has_video && video_fifo < threshold;
        video_fifo_high = !m_has_video || (video_pts != DVD_NOPTS_VALUE && video_fifo > m_threshold);
      }
      CLog::Log(LOGDEBUG, "Normal M:%.0f (A:%.0f V:%.0f) P:%d A:%.2f V:%.2f/T:%.2f (%d,%d,%d,%d) A:%d%% V:%d%% (%.2f,%.2f)\n", stamp, audio_pts, video_pts, m_av_clock->OMXIsPaused(),
        audio_pts == DVD_NOPTS_VALUE ? 0.0:audio_fifo, video_pts == DVD_NOPTS_VALUE ? 0.0:video_fifo, m_threshold, audio_fifo_low, video_fifo_low, audio_fifo_high, video_fifo_high,
        m_player_audio.GetLevel(), m_player_video.GetLevel(), m_player_audio.GetDelay(), (float)m_player_audio.GetCacheTotal());

      // keep latency under control by adjusting clock (and so resampling audio)
      if (m_live)
      {
        float latency = DVD_NOPTS_VALUE;
        if (m_has_audio && audio_pts != DVD_NOPTS_VALUE)
          latency = audio_fifo;
        else if (!m_has_audio && m_has_video && video_pts != DVD_NOPTS_VALUE)
          latency = video_fifo;
        if (!m_Pause && latency != DVD_NOPTS_VALUE)
        {
          if (m_av_clock->OMXIsPaused())
          {
            if (latency > m_threshold)
            {
              CLog::Log(LOGDEBUG, "Resume %.2f,%.2f (%d,%d,%d,%d) EOF:%d PKT:%p\n", audio_fifo, video_fifo, audio_fifo_low, video_fifo_low, audio_fifo_high, video_fifo_high, m_omx_reader.IsEof(), m_omx_pkt);
              m_av_clock->OMXResume();
              m_latency = latency;
            }
          }
          else
          {
            m_latency = m_latency*0.99f + latency*0.01f;
            float speed = 1.0f;
            if (m_latency < 0.5f*m_threshold)
              speed = 0.990f;
            else if (m_latency < 0.9f*m_threshold)
              speed = 0.999f;
            else if (m_latency > 2.0f*m_threshold)
              speed = 1.010f;
            else if (m_latency > 1.1f*m_threshold)
              speed = 1.001f;

            m_av_clock->OMXSetSpeed(S(speed));
            m_av_clock->OMXSetSpeed(S(speed), true, true);
            CLog::Log(LOGDEBUG, "Live: %.2f (%.2f) S:%.3f T:%.2f\n", m_latency, latency, speed, m_threshold);
          }
        }
      }
      else if(!m_Pause && (m_omx_reader.IsEof() || m_omx_pkt || TRICKPLAY(m_av_clock->OMXPlaySpeed()) || (audio_fifo_high && video_fifo_high)))
      {
        if (m_av_clock->OMXIsPaused())
        {
          CLog::Log(LOGDEBUG, "Resume %.2f,%.2f (%d,%d,%d,%d) EOF:%d PKT:%p\n", audio_fifo, video_fifo, audio_fifo_low, video_fifo_low, audio_fifo_high, video_fifo_high, m_omx_reader.IsEof(), m_omx_pkt);
          m_av_clock->OMXResume();
        }
      }
      else if (m_Pause || audio_fifo_low || video_fifo_low)
      {
        if (!m_av_clock->OMXIsPaused())
        {
          if (!m_Pause)
            m_threshold = std::min(2.0f*m_threshold, 16.0f);
          CLog::Log(LOGDEBUG, "Pause %.2f,%.2f (%d,%d,%d,%d) %.2f\n", audio_fifo, video_fifo, audio_fifo_low, video_fifo_low, audio_fifo_high, video_fifo_high, m_threshold);
          m_av_clock->OMXPause();
        }
      }
    }
    if (!sentStarted)
    {
      CLog::Log(LOGDEBUG, "COMXPlayer::HandleMessages - player started RESET");
      m_av_clock->OMXReset(m_has_video, m_has_audio);
      sentStarted = true;
    }

    if(!m_omx_pkt)
      m_omx_pkt = m_omx_reader.Read();

    if(m_omx_pkt)
      m_send_eos = false;

    if(m_omx_reader.IsEof() && !m_omx_pkt)
    {
      // demuxer EOF, but may have not played out data yet
      if ( (m_has_video && m_player_video.GetCached()) ||
           (m_has_audio && m_player_audio.GetCached()) )
      {
        OMXClock::OMXSleep(10);
        continue;
      }
      if (!m_send_eos && m_has_video)
        m_player_video.SubmitEOS();
      if (!m_send_eos && m_has_audio)
        m_player_audio.SubmitEOS();
      m_send_eos = true;
      if ( (m_has_video && !m_player_video.IsEOS()) ||
           (m_has_audio && !m_player_audio.IsEOS()) )
      {
        OMXClock::OMXSleep(10);
        continue;
      }
      break;
    }

    if(m_has_video && m_omx_pkt && m_omx_reader.IsActive(OMXSTREAM_VIDEO, m_omx_pkt->stream_index))
    {
      if (TRICKPLAY(m_av_clock->OMXPlaySpeed()))
      {
         m_packet_after_seek = true;
      }
      if(m_player_video.AddPacket(m_omx_pkt))
        m_omx_pkt = NULL;
      else
        OMXClock::OMXSleep(10);
    }
    else if(m_has_audio && m_omx_pkt && !TRICKPLAY(m_av_clock->OMXPlaySpeed()) && m_omx_pkt->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      if(m_player_audio.AddPacket(m_omx_pkt))
        m_omx_pkt = NULL;
      else
        OMXClock::OMXSleep(10);
    }
    else if( m_omx_pkt && !TRICKPLAY(m_av_clock->OMXPlaySpeed()) &&
            m_omx_pkt->codec_type == AVMEDIA_TYPE_SUBTITLE)
    {
      auto result = m_player_subtitles.AddPacket(m_omx_pkt,
                      m_omx_reader.GetRelativeIndex(m_omx_pkt->stream_index));
      if (result)
        m_omx_pkt = NULL;
      else
        OMXClock::OMXSleep(10);
    }
    else
    {
      if(m_omx_pkt)
      {
        m_omx_reader.FreePacket(m_omx_pkt);
        m_omx_pkt = NULL;
      }
      else
        OMXClock::OMXSleep(10);
    }

  }

    do_exit();
    m_filename = "";
}
usleep(1000);
}
    return 1;
}
void do_exit()
{
  if (m_stats)
    printf("\n");

  if (m_stop)
  {
    unsigned t = (unsigned)(m_av_clock->OMXMediaTime()*1e-6);
    printf("Stopped at: %02d:%02d:%02d\n", (t/3600), (t/60)%60, t%60);
  }

/*  if (m_NativeDeinterlace)
  {
    char response[80];
    vc_gencmd(response, sizeof response, "hvs_update_fields %d", 0);
  }

*/

  if(m_has_video && m_refresh && tv_state.display.hdmi.group && tv_state.display.hdmi.mode)
  {
    m_BcmHost.vc_tv_hdmi_power_on_explicit_new(HDMI_MODE_HDMI, (HDMI_RES_GROUP_T)tv_state.display.hdmi.group, tv_state.display.hdmi.mode);
  }

  m_av_clock->OMXStop();
  m_av_clock->OMXStateIdle();

  m_player_subtitles.Close();
  m_player_video.Close();
  m_player_audio.Close();

  if(m_omx_pkt)
  {
    m_omx_reader.FreePacket(m_omx_pkt);
    m_omx_pkt = NULL;
  }

  m_omx_reader.Close();

  m_av_clock->OMXDeinitialize();
  if (m_av_clock)
    delete m_av_clock;

  vc_tv_show_info(0);
  g_OMX.Deinitialize();
  g_RBP.Deinitialize();

  printf("have a nice day ;)\n");

}

