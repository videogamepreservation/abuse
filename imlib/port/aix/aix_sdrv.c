/*

AIX UMS sound module.

Biggest weirdo thing in here besides the include "gen_drv.c" (legacy)
is that there is a bug in the AIX UMS sound support.  UMS won't play
sound unless there are at least 8k (?) samples submitted.  Because this
is almost a second when playing 8-bit 11kHz mono sound, the latency
would kill the usefulness.  So the sound driver plays at 44kHz in
16-bit stereo simply so that it can submit 8k chunks without hurting
the sound latency.

Seeing as how we were 16-bit, gen_drv.c was modified to support 13-bit
mixing instead of 8-bit.  The clip was deferred until the output step
to avoid an extra pass over the 13-bit buffer.

*/

#include <strings.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <UMSAudioDevice.h>

void output_samples(short *buf);
void sound_uninit();
int sound_init();

#define BUF_SIZE 512

static UMSAudioDevice audio_device;
static UMSAudioTypes_Buffer output_buffer;
static Environment *audio_env;
static int output_buffer_samples;
static float usecs_per_samplepair;
static int samplepair_latency;
static short *output_buffer_data;
static int last_sample;
static char str[1024];

#include "gen_drv.c"

void output_samples(short *buf)
{

	long samplepairs_written;
	long samplepairs_en_route;
	int i;
	int delta;
	int d_prev;
	int d_next;

	output_buffer._length = output_buffer._maximum;

// translate 13-bit mono 11kHz (1k) output buffer to linearly interpolated
// 16-bit stereo 44kHz output buffer (8k).  for smooth transition between
// buffers, first sample must be remembered from the last buffer.

	d_prev = last_sample;

	d_next = buf[0] << 3;
	if (d_next < -32768) d_next = -32768;
	else if (d_next > 32767) d_next = 32767;

	delta = (d_next - d_prev) >> 2;
	output_buffer_data[0] = d_prev;
	output_buffer_data[2] = output_buffer_data[0] + delta;
	output_buffer_data[4] = output_buffer_data[2] + delta;
	output_buffer_data[6] = output_buffer_data[4] + delta;
	output_buffer_data[1] = output_buffer_data[0];
	output_buffer_data[3] = output_buffer_data[2];
	output_buffer_data[5] = output_buffer_data[4];
	output_buffer_data[7] = output_buffer_data[6];

	d_prev = d_next;

	for (i=1 ; i<BUF_SIZE ; i++)
	{

		d_next = buf[i] << 3;
		if (d_next < -32768) d_next = -32768;
		else if (d_next > 32767) d_next = 32767;

		delta = (d_next - d_prev)>>2;
		output_buffer_data[i*8] = d_prev;
		output_buffer_data[i*8+2] = output_buffer_data[i*8] + delta;
		output_buffer_data[i*8+4] = output_buffer_data[i*8+2] + delta;
		output_buffer_data[i*8+6] = output_buffer_data[i*8+4] + delta;
		output_buffer_data[i*8+1] = output_buffer_data[i*8];
		output_buffer_data[i*8+3] = output_buffer_data[i*8+2];
		output_buffer_data[i*8+5] = output_buffer_data[i*8+4];
		output_buffer_data[i*8+7] = output_buffer_data[i*8+6];

		d_prev = d_next;
	}

	last_sample = d_prev;

// an essentially non-blocking write()
	UMSAudioDevice_write(audio_device, audio_env, &output_buffer,
		output_buffer_samples, &samplepairs_written);

// simulate a blocking write() with usleep()
	UMSAudioDevice_write_buff_used(audio_device, audio_env,
		&samplepairs_en_route);
	if (samplepairs_en_route > samplepair_latency)
		usleep((int) ((samplepairs_en_route - samplepair_latency)
		* usecs_per_samplepair));

}

void sound_uninit()
{
	UMSAudioDevice_play_remaining_data(audio_device, audio_env, TRUE);
	UMSAudioDevice_stop(audio_device, audio_env);
	UMSAudioDevice_close(audio_device, audio_env);
}

char *newpaths[] = 
{
	"bin",
	"dt/bin",
	"speech/speech_reco/bin",
	"Demos/speech_reco/bin",
	"Demos/tts/bin"
};

char *newlibpaths[] =
{
	"lib",
	"speech/speech_reco/lib",
	"Demos/tts/lib"
};

void run_ums(void)
{
	char *path, *u;
	int i;

	if (!getenv("SOMBASE"))
		putenv("SOMBASE=/usr/lpp/som");
	fprintf(stderr, "SOMBASE=%s\n", getenv("SOMBASE"));
	if (!getenv("UMSDIR"))
		putenv("UMSDIR=/usr/lpp/UMS");
	fprintf(stderr, "UMSDIR=%s\n", getenv("UMSDIR"));
	sprintf(str, "PATH=");
	path = getenv("PATH");
	if (!path || *path==0) path=".";
	strcat(str, path);
	for (i=0 ; i<5 ; i++)
	{
		sprintf(str+512, "%s/%s", getenv("UMSDIR"), newpaths[i]);
		fprintf(stderr, "str+512=[%s]\n", str+512);
		if (!strstr(path, str+512))
		{
			strcat(str, ":");
			strcat(str, str+512);
			fprintf(stderr, "str=[%s]\n", str);
		}
	}
	putenv(str);
	fprintf(stderr, "PATH=%s\n", getenv("PATH"));

	sprintf(str, "LIBPATH=");
	path = getenv("LIBPATH");
	if (!path || *path==0) path=".";
	fprintf(stderr, "[[%s]]\n", path);
	strcat(str, path);
	if (!strstr(path, "/usr/lib"))
		strcat(str, ":/usr/lib");
	sprintf(str+512, "%s/lib", getenv("SOMBASE"));
	if (!strstr(path, str+512))
	{
		strcat(str, ":");
		strcat(str, str+512);
	}
	for (i=0 ; i<3 ; i++)
	{
		sprintf(str+512, "%s/%s", getenv("UMSDIR"), newlibpaths[i]);
		if (!strstr(path, str+512))
		{
			strcat(str, ":");
			strcat(str, str+512);
		}
	}
	putenv(str);
	fprintf(stderr, "LIBPATH=%s\n", getenv("LIBPATH"));

	sprintf(str, "NLSPATH=");
	path = getenv("NLSPATH");
	if (!path || *path == 0) path=".";
	strcat(str, path);
	sprintf(str+512, "%s/msg/%%N", getenv("SOMBASE"));
	if (!strstr(path, str+512))
	{
		strcat(str, ":");
		strcat(str, str+512);
	}
	putenv(str);
	fprintf(stderr, "NLSPATH=%s\n", getenv("NLSPATH"));

	sprintf(str, "SOMIR=");
	path = getenv("SOMIR");
	if (!path || *path == 0) path=".";
	strcat(str, path);
	sprintf(str+512, "%s/etc/UMS.ir", getenv("UMSDIR"));
	if (!strstr(path, str+512))
	{
		strcat(str, ":");
		strcat(str, str+512);
	}
	sprintf(str+512, "%s/etc/som.ir", getenv("UMSDIR"));
	if (!strstr(path, str+512))
	{
		strcat(str, ":");
		strcat(str, str+512);
	}
	putenv(str);
	fprintf(stderr, "SOMIR=%s\n", getenv("SOMIR"));
	putenv("MALLOCTYPE=");
}

int sound_init()
{

	UMSAudioDeviceMClass audio_device_class;
	char *alias;
	long flags;
	UMSAudioDeviceMClass_ErrorCode audio_device_class_error;
	char *error_string;
	char *audio_formats_alias;
	char *audio_inputs_alias;
	char *audio_outputs_alias;
	int bits_per_sample;
	long out_rate;
	int channels;
	long left_gain;
	long right_gain;
	char *sl;

//	run_ums();

	audio_env = somGetGlobalEnvironment();

	audio_device_class = UMSAudioDeviceNewClass(UMSAudioDevice_MajorVersion,
		UMSAudioDevice_MinorVersion);
    if (audio_device_class == NULL)
	{
        fprintf(stderr, "sound driver: Can't create AudioDeviceMClass metaclass\n");
        return 0;
	}

    alias = "Audio";
    flags = UMSAudioDevice_BlockingIO;

	audio_device = UMSAudioDeviceMClass_make_by_alias(audio_device_class,
		audio_env, alias, "PLAY", flags, &audio_device_class_error,
		&error_string, &audio_formats_alias, &audio_inputs_alias,
		&audio_outputs_alias);

    if (audio_device == NULL)
	{
        fprintf(stderr, "sound driver : Can't create audio device object\n");
        return 0;
	}

	channels = 2;
	bits_per_sample = 16;

	UMSAudioDevice_set_sample_rate(audio_device, audio_env, 44100, &out_rate);
	UMSAudioDevice_set_bits_per_sample(audio_device, audio_env,
		bits_per_sample);
	UMSAudioDevice_set_number_of_channels(audio_device, audio_env, channels);

	if (out_rate != 44100)
	{
		fprintf(stderr, "sound driver : Doesn't support 44.1kHz\n");
		return 0;
	}

    UMSAudioDevice_set_audio_format_type(audio_device, audio_env, "PCM");
    UMSAudioDevice_set_byte_order(audio_device, audio_env, "MSB");
    UMSAudioDevice_set_number_format(audio_device, audio_env,
		"TWOS_COMPLEMENT");
	UMSAudioDevice_set_volume(audio_device, audio_env, 100);
	UMSAudioDevice_set_balance(audio_device, audio_env, 0);

	output_buffer_samples = BUF_SIZE * 4;
	UMSAudioDevice_set_time_format(audio_device, audio_env,
		UMSAudioTypes_Samples);

	left_gain = right_gain = 100;
	UMSAudioDevice_enable_output(audio_device, audio_env, "INTERNAL_SPEAKER",
		&left_gain, &right_gain);
	left_gain = right_gain = 100;
	UMSAudioDevice_enable_output(audio_device, audio_env, "LINE_OUT",
		&left_gain, &right_gain);

	UMSAudioDevice_initialize(audio_device, audio_env);
	UMSAudioDevice_start(audio_device, audio_env);

	output_buffer._maximum = BUF_SIZE*8*2;
	output_buffer_data = (short *) malloc(output_buffer._maximum);
	output_buffer._buffer = (octet *) output_buffer_data;

	usecs_per_samplepair = 1000000.0 / out_rate;

	samplepair_latency = 1500;
	sl = getenv("SAMPLEPAIR_LATENCY");
	if (sl) samplepair_latency = atoi(sl);

	last_sample = 0;

	return 1;

}


