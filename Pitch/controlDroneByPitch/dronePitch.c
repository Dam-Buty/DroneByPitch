/*===============================================================================================
 Pitch detection example.
 Copyright (c), Firelight Technologies Pty, Ltd 2004-2010.

 This example combines recording with spectrum analysis to determine the pitch of the sound
 being recorded.
===============================================================================================*/
#include "fmod.h"
//#include "kiss_fft.h"
//#include "kiss_fftr.h"
#include "fmod_errors.h"
#include <stdio.h>
// #include <conio.h>
#include <math.h>

#define NB_RECORD_MAX		200000
#define OUTPUTRATE          44100
#define USER_DETECT_SPEED_MS 200

char	*noteTable [12] = {"DO","DO#","RE","RE#","MI","FA","FA#","SOL","SOL#","LA","LA#","SI"};
float 	frequencyTable[11][12] =
								{
									{16.351, 17.323, 18.354, 19.445, 20.601, 21.826, 23.124, 24.499, 25.956, 27.500, 29.135, 30.867},
									{32.703, 34.647, 36.708, 38.890, 41.203, 43.653, 46.249, 48.999, 51.913, 55.000, 58.270, 61.735},
									{65.406, 69.295, 73.416, 77.781, 82.406, 87.307, 92.498, 97.998, 103.826, 110.000, 116.540, 123.47},
									{130.812, 138.591, 146.832, 155.563, 164.813, 174.614, 184.997, 195.997, 207.652, 220.000, 233.081, 246.941},
									{261.625, 277.182, 293.664, 311.126, 329.627, 349.228, 369.994, 391.995, 415.304, 440.000, 466.163, 493.883},
									{523.251, 554.365, 587.329, 622.253, 659.255, 698.456, 739.988, 783.991, 830.609, 880.000, 932.327, 987.766},
									{1046.502, 1108.730, 1174.059, 1244.507, 1318.510, 1396.912, 1479.976, 1567.982, 1661.218, 1760.000, 1864.654, 1975.532},
									{2093.004, 2217.460, 2344.318, 2489.014, 2637.020, 2793.824, 2959.952, 3135.964, 3322.436, 3520.000, 3729.308, 3951.064},
									{4186.008, 4434.920, 4698.636, 4978.028, 5274.040, 5587.648, 5919.904, 6270.928, 6644.872, 7040.000, 7458.616, 7902.128},
									{8372.016, 8869.840, 9397.272, 9956.056, 10548.080, 11175.296, 11839.808, 12541.856, 13289.744, 14080.000, 14917.232, 15804.256},
									{16744.032, 17739.680, 18794.544, 19912.112, 221096.160, 22350.592, 23679.616, 25083.712, 26579.488, 28160.000, 29834.464, 31608.512}
								};

// Pour un rate � 44100 �chantillons par seconde
float sampleMappingTable[2][7] =	{
										{23.21, 46.43, 92.87, 185.75, 371.51, 743.03, 1486.077}, // valeur en ms
										{1024, 2048, 4096, 8192, 16384, 32768, 65536} // Nb valeurs retenu � 44100 hz
									};


void ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}

/* MAPPING FUNCTION BETWEEN NOTE AND FREQUENCY */
float mappingNoteFrequency(float f_in)
{
	float f_out = 0.0;
	int ind = 0;
	int jnd = 0;

	for (jnd=0;jnd<11;jnd++) // 1 ligne par octave (en tout : 11 octaves)
	{
		for (ind=0;ind<11;ind++) // 1 colonne par 1/2 ton (en tout : 12 demi tons)
		{
			if ((f_in>frequencyTable[jnd][ind]) && (f_in<frequencyTable[jnd][ind+1]))
			{
				printf("Detected note : %s%i	Note frequency : %f Hz\n",noteTable[ind],jnd,f_in);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	FMOD_SYSTEM           *system  = 0;
	FMOD_SOUND            *sound   = 0;
	FMOD_CHANNEL          *channel = 0;
	FMOD_RESULT            result;
	FMOD_CREATESOUNDEXINFO exinfo;
	FMOD_SPEAKERMODE *speakermode = 0;
	FMOD_CHANNELGROUP *hMasterChannelGroup;
	FMOD_DSP *hDSP = NULL;
	int i = 0;
	int j =0;
	int isRecording = 0;
	int systemrate = 0;
	int speakermodechannels = 0;
	int numrawspeakers = 0;
	int key, driver, recorddriver, numdrivers, count, bin;
	unsigned int    recordpos = 0;
	unsigned int version;
	unsigned int NB_record=1;
	float f_dominant_freq_prec = -1.0;
	float f_dominant_freq = 0.0;
	//float f_totalFreq = 0.0;

	int WINDOW_SIZE_FFT = 0;
	for (i=0;i<6;i++)
	{
		if ((USER_DETECT_SPEED_MS>=sampleMappingTable[0][i]) && (USER_DETECT_SPEED_MS<=sampleMappingTable[0][i+1]))
		{
			WINDOW_SIZE_FFT = sampleMappingTable[1][i+1];
			// printf("WINDOW_SIZE_FFT : %i\n",WINDOW_SIZE_FFT);
		}
	}
	if(WINDOW_SIZE_FFT==0)
	{
		printf("\nChange la valeur de 24 ms<USER_DETECT_SPEED_MS<1000ms et relance moi !\n");
		return 1;
	}
	i=0;
	//

	//KISS FFT
	// int size = 1024;//160;
	// int isinverse = 1;
	// kiss_fft_scalar zero;
	// memset(&zero,0,sizeof(zero));
	// kiss_fft_cpx fft_in[size];
	// kiss_fft_cpx fft_out[size];
	// kiss_fft_cpx fft_reconstructed[size];

	// kiss_fftr_cfg fft = kiss_fftr_alloc(size*2 ,0 ,0,0);
	// kiss_fftr_cfg ifft = kiss_fftr_alloc(size*2,isinverse,0,0);

	// /* Haming Function */
	// float hanningWindow(short in, size_t i, size_t s)
	// {return in*0.5f*(1.0f-cos(2.0f*M_PI*(float)(i)/(float)(s-1.0f)));}
	// float *input = malloc(size*sizeof(float));
	// float *fft_norm = malloc(size*sizeof(float));
	//

	/**/


/* SKYFX*/

    /*
        Create a System object and initialize.
    */
    result = FMOD_System_Create(&system);
    ERRCHECK(result);

    result = FMOD_System_GetVersion(system, &version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return 0;
    }

    /*
        System initialization
    */
    printf("---------------------------------------------------------\n");
    printf("Select OUTPUT type\n");
    printf("---------------------------------------------------------\n");
    printf("1 :  DirectSound\n");
    printf("2 :  Windows Multimedia WaveOut\n");
    printf("3 :  ASIO\n");
    printf("---------------------------------------------------------\n");
    printf("Press a cor1responding number or ESC to quit\n");

    do
    {
        key = _getch();
    } while (key != 27 && key < '1' && key > '5');

    switch (key)
    {
        case '1' :  result = FMOD_System_SetOutput(system, FMOD_OUTPUTTYPE_DSOUND);
                    break;
        case '2' :  result = FMOD_System_SetOutput(system, FMOD_OUTPUTTYPE_WINMM);
                    break;
        case '3' :  result = FMOD_System_SetOutput(system, FMOD_OUTPUTTYPE_ASIO);
                    break;
        default  :  return 1;
    }
    ERRCHECK(result);

    /*
        Enumerate playback devices
    */

    result = FMOD_System_GetNumDrivers(system, &numdrivers);
    ERRCHECK(result);

	printf("\nnumdrivers : %i\n",numdrivers);
    printf("---------------------------------------------------------\n");
    printf("Choose a PLAYBACK driver\n");
    printf("---------------------------------------------------------\n");
    for (count=0; count < numdrivers; count++)
    {
        char name[256];

        result = FMOD_System_GetDriverInfo(system, count, name, 256, 0, &systemrate, speakermode, &speakermodechannels);
        ERRCHECK(result);

        printf("%d : %s\n", count + 1, name);
    }
    printf("---------------------------------------------------------\n");

    printf("Press a corresponding number or ESC to quit\n");

    do
    {
        key = _getch();
        if (key == 27)
        {
            return 0;
        }
        driver = key - '1';
    } while (driver < 0 || driver >= numdrivers);

    result = FMOD_System_SetDriver(system, driver);
    ERRCHECK(result);

    /*
        Enumerate record devices
    */

    result = FMOD_System_GetRecordNumDrivers(system, &numdrivers);
    ERRCHECK(result);

    printf("---------------------------------------------------------\n");
    printf("Choose a RECORD driver\n");
    printf("---------------------------------------------------------\n");
    for (count=0; count < numdrivers; count++)
    {
        char name[256];

        result = FMOD_System_GetRecordDriverInfo(system, count, name, 256, 0, &systemrate, speakermode, &speakermodechannels);
        ERRCHECK(result);

        printf("%d : %s\n", count + 1, name);
    }
    printf("---------------------------------------------------------\n");
    printf("Press a corresponding number or ESC to quit\n");

    recorddriver = 0;
    do
    {
        key = _getch();
        if (key == 27)
        {
            return 0;
        }
        recorddriver = key - '1';
    } while (recorddriver < 0 || recorddriver >= numdrivers);

    printf("\n");

	result = FMOD_System_SetDriver(system, recorddriver);
	ERRCHECK(result);

    result = FMOD_System_SetSoftwareFormat(system, OUTPUTRATE, FMOD_SOUND_FORMAT_PCM16, 1);
    ERRCHECK(result);

	result = FMOD_System_GetSoftwareFormat(system, &systemrate, speakermode, &numrawspeakers);
	ERRCHECK(result);

	printf("------------------------\n");
	printf("DRIVER_INFO_RATE : %i\nSPEAKER_MODE_CHANNEL : %i\nNUM_RAW_SPEAKERS : %i\nWINDOW_SIZE_FFT : %i\nUSER_DETECT_SPEED_MS : %i\n",systemrate, speakermodechannels, numrawspeakers, WINDOW_SIZE_FFT, USER_DETECT_SPEED_MS);
	printf("------------------------\n\n");


	// INIT SYSTEM
	result = FMOD_System_Init(system, 32, FMOD_INIT_NORMAL, NULL);
    ERRCHECK(result);

    /*
        Create a sound to record to.
    */
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize           = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels      = 1;
    exinfo.format           = FMOD_SOUND_FORMAT_PCM16;
    exinfo.defaultfrequency = OUTPUTRATE;
    exinfo.length           = exinfo.defaultfrequency * sizeof(short) * exinfo.numchannels * 5;

    result = FMOD_System_CreateSound(system, 0, FMOD_OPENUSER, &exinfo, &sound); //FMOD_2D | FMOD_DEFAULT | FMOD_LOOP_NORMAL | FMOD_OPENUSER
    ERRCHECK(result);

	/* GET DSP */
	result = FMOD_System_GetMasterChannelGroup(system, &hMasterChannelGroup);
	ERRCHECK(result);

	result = FMOD_ChannelGroup_GetDSP(hMasterChannelGroup,0,&hDSP);
	ERRCHECK(result);

	// CREATE DSP and SET IT
	result = FMOD_System_CreateDSPByType(system, FMOD_DSP_TYPE_FFT, &hDSP);
	ERRCHECK(result);

	result = FMOD_DSP_SetParameterInt(hDSP, FMOD_DSP_FFT_WINDOWSIZE, WINDOW_SIZE_FFT); //32768
	ERRCHECK(result);

	result = FMOD_DSP_SetParameterInt(hDSP, FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_HAMMING);
	ERRCHECK(result);

	/* ADD DSP */
	result = FMOD_ChannelGroup_AddDSP(hMasterChannelGroup, FMOD_CHANNELCONTROL_DSP_HEAD, hDSP);
	ERRCHECK(result);

	//printf("Init DSP : OK\n");
	/*
        Start the interface
    */
    printf("========================\n");
    printf("Main frequency detection\n");
    printf("========================\n");

	printf("Press 'r' to record \n");
	printf("Press 'p' to play and detect main sound frequency\n");
	printf("Press 'ESC' to quit\n");
	/* SEARCH DOMINANT FREQUENCY WHILE IS PLAYING */
	do {
		if (_kbhit())
		{
			key = _getch();
			if (key==112) // 112 --> 'p' : play sound
			{
				result = FMOD_System_PlaySound(system, sound, hMasterChannelGroup, 0, &channel);
				ERRCHECK(result);

				f_dominant_freq_prec = -1;
				i=0;
			}
			printf("\n");
			if(key==114) // 114 --> 'r' : record
			{
				printf("\n");
				result = FMOD_System_RecordStart(system, recorddriver, sound, TRUE);
				ERRCHECK(result);
				i=0;
				do
				{
					isRecording = 0;
					result = FMOD_System_IsRecording(system, recorddriver, &isRecording);
					ERRCHECK(result);

					NB_record=recordpos;
					if ((isRecording))
					{
						result = FMOD_System_GetRecordPosition(system, recorddriver, &recordpos);
						ERRCHECK(result);

						if (recordpos!=NB_record)
						{
							i++;
							printf("Record position: %5d/100\r", 100*recordpos/NB_RECORD_MAX);
						}
					}
				}
				while((recordpos<NB_RECORD_MAX)&&(result == FMOD_OK)); //97 = 'a' et 27 = "�chap"

				/* STOP RECORDING */
				result = FMOD_System_RecordStop(system, recorddriver);
				ERRCHECK(result);
			}
			//printf("key= % i\r",key);
		}
		/* UPDATE SYSTEM */
		FMOD_System_Update(system);
		ERRCHECK(result);

		result = FMOD_DSP_GetParameterFloat(hDSP, FMOD_DSP_FFT_DOMINANT_FREQ, &f_dominant_freq, 0, 0); //FMOD_DSP_FFT_DOMINANT_FREQ
		ERRCHECK(result);

		if ((int)f_dominant_freq>200)
		{
			f_dominant_freq = mappingNoteFrequency(f_dominant_freq);
			//printf("Dominant_freq : %f Hz\n",f_dominant_freq);
		}
		usleep(USER_DETECT_SPEED_MS * 1000); // -->200 ms

		/* SELECT FREQUENCY MEANING */
		// if(((int)f_dominant_freq>0)&&(((int)f_dominant_freq)!=((int)f_dominant_freq_prec)))//&&(f_dominant_freq>f_dominant_freq_prec))
		// {
			// if(j>=20)
			// {
				// f_dominant_freq = f_totalFreq/j;
				// printf("Dominant_freq : %f Hz\n",f_dominant_freq);
				// j=0;
				// f_totalFreq = 0;
			// }
			// else
			// {
				// j++;
				// f_totalFreq = f_totalFreq + f_dominant_freq;
			// }


			// f_dominant_freq_prec = f_dominant_freq;
		// }
	}while (key != 27);

	/*
        Shut down
    */
	FMOD_ChannelGroup_RemoveDSP(hMasterChannelGroup, hDSP);
	ERRCHECK(result);
	result = FMOD_DSP_Release(hDSP);
	ERRCHECK(result);

    result = FMOD_Sound_Release(sound);
    ERRCHECK(result);
    result = FMOD_System_Close(system);
    ERRCHECK(result);
    result = FMOD_System_Release(system);
    ERRCHECK(result);

    return 0;
}
