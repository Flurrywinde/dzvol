//
// TODO description
//
// Public domain.
//

#define _DEFAULT_SOURCE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <alsa/asoundlib.h>
#include <X11/Xlib.h>

#define _BSD_SOURCE

int WIDTH  = 256;
int HEIGHT = 32;

// -1 means center of the screen
int X = -1;
int Y = -1;

int SECONDS_DELAY = 2;

char _FONT[256];
char _BG[9];
char _FG[9];

// char ICON_TEXT[32] = "♪";
int ANIMICON = 0;  // -a options sets this to 1 for 📢 🔊 🔉 🔈🔇 animated icon

// The delay between refreshing mixer values
unsigned long REFRESH_SPEED = 50000;

// Get volume from Alsa or Pulseaudio. 'a' = alsa (default), 'p' = pulseaudio
int ALSAPULSE = 'a';

// Max volume (default: 100)
int _MAX = 100;

const char *ATTACH = "default";
const snd_mixer_selem_channel_id_t CHANNEL = SND_MIXER_SCHN_FRONT_LEFT;
const char *SELEM_NAME = "Master";

char *LOCK_FILE = "/tmp/dzvol";

void get_volume_alsa(float *vol, int *switch_value);
void get_volume_pulseaudio(float *vol, int *switch_value);
void seticon(float vol, float prev_vol, int switch_value, int prev_switch_value, char *ICON_TEXT);

void print_usage(void)
{
    remove(LOCK_FILE);
    puts("Usage: dzvol [options]");
    puts("Most options are the same as dzen2\n");
    puts("\t-x X_POS\tmove to the X coordinate on your screen, -1 will center (default: -1)");
    puts("\t-y Y_POS\tmove to the Y coordinate on your screen, -1 will center (default: -1)");
    puts("\t-w WIDTH\tset the width (default: 256)");
    puts("\t-h HEIGHT\tset the height (default: 32)");
    puts("\t-d|--delay DELAY  time it takes to exit when volume doesn't change, in seconds (default: 2)");
    puts("\t-bg BG\t\tset the background color");
    puts("\t-fg FG\t\tset the foreground color");
    puts("\t-fn FONT\tset the font face; same format dzen2 would accept");
    puts("\t-i ICON\t\tsets the icon text/character to whatever you want");
    puts("\t-s|--speed SPEED  time to wait between volume checks, in microseconds (default: 50000)");
    puts("\t\t\t  higher amounts are slower, lower amounts (<20000) will begin to cause high CPU usage");
    puts("\t\t\t  see `man 3 usleep`");
    puts("\t-p|--pulseaudio\tget volume from pulseaudio (else defaults to alsa)");
    puts("\t-m|--max MAX\tset volume maximum (default: 100)");
    puts("\t-a|--animated-icon  Make the icon a color emoji that changes depending on the volume.");
    puts("\t                    Can be used in conjunction with -i which sets the initial icon.");
    puts("\t-h|--help\tdisplay this message and exit (0)");
    puts("\t-v|--version\tdisplay version information. (Side-effect: reset the lock file in case of prior crash)\n");
    puts("Note that dzvol does NOT background itself.");
    puts("It DOES, however, allow only ONE instance of itself to be running at a time by creating /tmp/dzvol as a lock.");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	char *ICON_TEXT = malloc(sizeof(char) * 32);
	strcpy(ICON_TEXT, "♪");

    // command line arguments {{{
    for(int i = 1; i < argc; i++)
        if(strcmp(argv[i], "-v") == 0 ||
           strcmp(argv[i], "--version") == 0)
        {
            puts("dzvol: 2.0");
            puts("Nick Allevato, inspired by bruenig's dvol");
            puts("Now with modifications by Flurrywinde");
            remove(LOCK_FILE);
            exit(EXIT_SUCCESS);
        }

        else if(strcmp(argv[i], "-bg") == 0)
            strcpy(_BG, argv[++i]);

        else if(strcmp(argv[i], "-i") == 0)
            strcpy(ICON_TEXT, argv[++i]);

        else if(strcmp(argv[i], "-fg") == 0)
            strcpy(_FG, argv[++i]);

        else if(strcmp(argv[i], "-fn") == 0)
            strcpy(_FONT, argv[++i]);

        else if(strcmp(argv[i], "-x") == 0)
            X = atoi(argv[++i]);

        else if(strcmp(argv[i], "-y") == 0)
            Y = atoi(argv[++i]);

        else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--delay") == 0)
            SECONDS_DELAY = atoi(argv[++i]);

        else if(strcmp(argv[i], "-w") == 0)
            WIDTH = atoi(argv[++i]);

        else if(strcmp(argv[i], "-h") == 0) {
			if (argc - 1 <= i) {
				print_usage();
			} else
				HEIGHT = atoi(argv[++i]);
		}
        else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--speed") == 0)
            REFRESH_SPEED = atoi(argv[++i]);
        else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pulseaudio") == 0)
			ALSAPULSE = 'p';
        else if(strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--max") == 0)
            _MAX = atoi(argv[++i]);
        else if(strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--animated-icon") == 0)
			ANIMICON = 1;

        else if(strcmp(argv[i], "--help") == 0)
            print_usage();
    // }}}

    // Create a file in /tmp/ to see if this was already running
    if(access(LOCK_FILE, F_OK) == -1)
    {
        FILE *f = fopen(LOCK_FILE, "w");
        fprintf(f, "%d", getpid());
        fclose(f);
    }
    else {
        fprintf(stderr, "Lock file found");
        return 0;
	}

    // get X and Y
    Display *xdisp = XOpenDisplay(NULL);
    Screen *screen = DefaultScreenOfDisplay(xdisp);

    // screw division, bit shift to the right
    if(X == -1)
        X = (screen->width >> 1) - (WIDTH >> 1);

    if(Y == -1)
        Y = (screen->height >> 1) - HEIGHT;

    // Calculate the progress bar width and height from a base percentage.
    // I calculated these by coming up with a solid foundation for dimensions, then
    // divided to get a percentage for preservation of proportions.
    long pbar_height = lround(HEIGHT * 0.34);
    long pbar_max_width = lround(WIDTH * 0.586);
    long ltext_x = lround(WIDTH * 0.078);
    long rtext_x = lround(WIDTH * 0.78);

    char *command = malloc(sizeof(char) * 256);

    char BG[24];
    char FG[24];
    char FONT[270];
	float MAX;

    // Do color and font checks
    if(strcmp(_BG, "") != 0)
        sprintf(BG, "-bg '%s'", _BG);
    else
        strcpy(BG, "");

    if(strcmp(_FG, "") != 0)
        sprintf(FG, "-fg '%s'", _FG);
    else
        strcpy(FG, "");

    if(strcmp(_FONT, "") != 0)
        sprintf(FONT, "-fn '%s'", _FONT);
    else {
        strcpy(FONT, "");
	}

	// Calculate MAX
	if(_MAX < 100 || _MAX > 200) {
        fprintf(stderr, "--max must be an integer between 100 and 200\n");
		return 1;
	}
	MAX = (float)_MAX / 100;  // TODO: rename this. It's really what vol needs to be divided by to become 100% max

    sprintf(command, "dzen2 -ta l -x %d -y %d -w %d -h %d %s %s %s", X, Y, WIDTH, HEIGHT,
            BG, FG, FONT);

    FILE *stream = popen(command, "w");

    free(command);

    float prev_vol = -1;
    int prev_switch_value;

    // The time we should stop
    time_t current_time = (unsigned)time(NULL);
    time_t stop_time = current_time + SECONDS_DELAY;

	// Benchmarking alsa vs pulseaudio
	clock_t start, end;
	double cpu_time_used, total_time = 0;
	int i = 0;

    while(current_time <= stop_time)
    {
        current_time = (unsigned)time(NULL);
		i += 1;

        float vol;
        int switch_value;

		start = clock();
		if (ALSAPULSE == 'a')
			get_volume_alsa(&vol, &switch_value);
		else
			get_volume_pulseaudio(&vol, &switch_value);
		end = clock();
		cpu_time_used = ((double) (end - start));
		cpu_time_used = cpu_time_used / CLOCKS_PER_SEC;
		total_time += cpu_time_used;
		//printf("%f\n", vol);
		// Adjust vol to be 100% max
		vol = vol / MAX;
		if(ANIMICON && prev_vol != -1)
			seticon(vol, prev_vol, switch_value, prev_switch_value, ICON_TEXT);

        if(prev_vol != vol || prev_switch_value != switch_value)
        {
            char *string = malloc(sizeof(char) * 512);
			// Kanon added
			//sprintf(string, "^fn(Noto Color Emoji:size=16)^pa(+20X)👻^fn(IosevkaTerm Nerd Font:size=16)  ^r(37x11) ^pa(+200X) 24%^pa()\n");
			sprintf(string, "^pa(+%ldX)%s^pa()^fn(%s)  ^r%s(%ldx%ld) ^pa(+%ldX)%3.0f%%^pa()\n",
					ltext_x, ICON_TEXT, _FONT, (switch_value == 1) ? "" : "o",
					lround(pbar_max_width * vol), pbar_height, rtext_x, vol*100);
			/* Original code:
			sprintf(string, "^pa(+%ldX)%s^pa()  ^r%s(%ldx%ld) ^pa(+%ldX)%3.0f%%^pa()\n",
					ltext_x, ICON_TEXT, (switch_value == 1) ? "" : "o",
					lround(pbar_max_width * vol), pbar_height, rtext_x, vol*100); */
			// End of Kanon mod
            fprintf(stream, string);
            fflush(stream);
			//printf("%s\n", string);
            free(string);

            prev_vol = vol;
            prev_switch_value = switch_value;

            stop_time = current_time + SECONDS_DELAY;
        }

        usleep(REFRESH_SPEED);
    }

    fflush(NULL);
    pclose(stream);
    remove(LOCK_FILE);
	//printf("Benchmark: %f total time spent getting the volume (in seconds) %d times for an average of %f s.\n", total_time, i, total_time/i);
    return 0;
}

void error_close_exit(char *errmsg, int err, snd_mixer_t *h_mixer)
{
    if(err == 0)
        fprintf(stderr, errmsg);
    else
        fprintf(stderr, errmsg, snd_strerror(err));

    if(h_mixer != NULL)
        snd_mixer_close(h_mixer);

    exit(EXIT_FAILURE);
}

char* popen_firstline(char *cmd) {
  FILE *fp;
  char *line = malloc(sizeof(char) * 1035);

  /* Open the command for reading. */
  fp = popen(cmd, "r");
  if (fp == NULL) {
    printf("Failed to run command: %s\n", cmd);
    exit(1);
  }

  fgets(line, sizeof(line), fp);
  pclose(fp);
  return line;
}

void get_volume_pulseaudio(float *vol, int *switch_value) {
	char *volume, *muteness;
	volume = popen_firstline("/usr/bin/pamixer --get-volume");
	*vol = atof(volume) / 100;  // 0-1 float
	free(volume);
	muteness = popen_firstline("/usr/bin/pamixer --get-mute");
	if(strcmp(muteness, "true\n") == 0)
		*switch_value = 0;
	else
		*switch_value = 1;
	free(muteness);
}

void get_volume_alsa(float *vol, int *switch_value)
{
    int err;
    long volume, vol_min, vol_max;

    snd_mixer_t *h_mixer;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem ;

    if((err = snd_mixer_open(&h_mixer, 1)) < 0)
        error_close_exit("Mixer open error: %s\n", err, NULL);

    if((err = snd_mixer_attach(h_mixer, ATTACH)) < 0)
        error_close_exit("Mixer attach error: %s\n", err, h_mixer);

    if((err = snd_mixer_selem_register(h_mixer, NULL, NULL)) < 0)
        error_close_exit("Mixer simple element register error: %s\n", err, h_mixer);

    if((err = snd_mixer_load(h_mixer)) < 0)
        error_close_exit("Mixer load error: %s\n", err, h_mixer);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, SELEM_NAME);

    if((elem = snd_mixer_find_selem(h_mixer, sid)) == NULL)
        error_close_exit("Cannot find simple element\n", 0, h_mixer);

    snd_mixer_selem_get_playback_volume(elem, CHANNEL, &volume);

    snd_mixer_selem_get_playback_volume_range(elem, &vol_min, &vol_max);
    snd_mixer_selem_get_playback_switch(elem, CHANNEL, switch_value);

    *vol = volume / (float)vol_max;

    snd_mixer_close(h_mixer);
}

void seticon(float vol, float prev_vol, int switch_value, int prev_switch_value, char *ICON_TEXT) {
	if (switch_value == 0)
		strcpy(ICON_TEXT, "^fn(Noto Color Emoji:size=16)🔇");
	else if (vol >= 1)
		strcpy(ICON_TEXT, "^fn(Noto Color Emoji:size=16)📢");
	else if (vol >= .66)
		strcpy(ICON_TEXT, "^fn(Noto Color Emoji:size=16)🔊");
	else if (vol >= .33)
		strcpy(ICON_TEXT, "^fn(Noto Color Emoji:size=16)🔉");
	else
		strcpy(ICON_TEXT, "^fn(Noto Color Emoji:size=16)🔈");
}
