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
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>


void ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}

#define OUTPUTRATE          44100
#define SPECTRUMSIZE        8192
#define SPECTRUMRANGE       ((float)OUTPUTRATE / 2.0f)      /* 0 to nyquist */

#define BINSIZE      (SPECTRUMRANGE / (float)SPECTRUMSIZE)

#define CHUNKSIZE 4096

int main(int argc, char *argv[])
{
/* SKYFX */
		FMOD_SPEAKERMODE *speakermode = 0; 
		FMOD_CHANNELGROUP *hMasterChannelGroup;
		FMOD_DSP *hDSP = NULL;
		int i = 0;
		int isRecording = 0;
		int systemrate = 0;
		int speakermodechannels = 0;
		int numrawspeakers = 0;
		unsigned int    recordpos = 0;
		unsigned int length=0;
		unsigned int NB_record=1;
		float f_dominant_freq_prec = 0.0;
		float f_dominant_freq = 0.0;
		
		
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

    FMOD_SYSTEM           *system  = 0;
    FMOD_SOUND            *sound   = 0;
    FMOD_CHANNEL          *channel = 0;
    FMOD_RESULT            result;
    FMOD_CREATESOUNDEXINFO exinfo;
    int                    key, driver, recorddriver, numdrivers, count, bin;
    unsigned int           version;    

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
	printf("\n    --> DRIVER_INFO_RATE : %i\n    --> SPEAKER_MODE_CHANNEL : %i\n    --> NUM_RAW_SPEAKERS : %i\n\n",systemrate, speakermodechannels, numrawspeakers);
	
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

    /*
        Start the interface
    */
    printf("========================\n");
    printf("Main frequency detection\n");
    printf("========================\n");
    printf("\n");

	printf("Press 'a' to record\n");
    printf("\n");

    
	/* PLAY RECORDING */
    key = _getch();
	if (key == 97)
	{
		result = FMOD_System_RecordStart(system, recorddriver, sound, TRUE);    
		ERRCHECK(result);
		i=0;
		do
		{			
			isRecording = 0;
			result = FMOD_System_IsRecording(system, recorddriver, &isRecording);
			ERRCHECK(result);

			NB_record=recordpos;
			if ((isRecording))//&&(recordpos!=NB_record))
			{
				result = FMOD_System_GetRecordPosition(system, recorddriver, &recordpos);
				ERRCHECK(result);
				
				if (recordpos!=NB_record)
				{
					i++;
					printf("Record position: %5d\r", recordpos);
				}
			} 
		}
		while((recordpos<100000)&&(result == FMOD_OK)); //97 = 'a' et 27 = "échap"
		printf("\nNb_Record : %i\n",i);
	}
	/* STOP RECORDING */
	result = FMOD_System_RecordStop(system, recorddriver);
	ERRCHECK(result);

	/* GET LENGTH SOUND */
	result = FMOD_Sound_GetLength(sound, &length, FMOD_TIMEUNIT_MS);
	ERRCHECK(result);
	printf("SOUND LENGTH : %i\n",length);
		
	//
	/* GET DSP */
	result = FMOD_System_GetMasterChannelGroup(system, &hMasterChannelGroup);
	ERRCHECK(result);
	
	result = FMOD_ChannelGroup_GetDSP(hMasterChannelGroup,0,&hDSP);
	ERRCHECK(result);
	
	// CREATE DSP and SET IT
	result = FMOD_System_CreateDSPByType(system, FMOD_DSP_TYPE_FFT, &hDSP);
	ERRCHECK(result);

	result = FMOD_DSP_SetParameterInt(hDSP, FMOD_DSP_FFT_WINDOWSIZE, 1024);
	ERRCHECK(result);

	result = FMOD_DSP_SetParameterInt(hDSP, FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_RECT);
	ERRCHECK(result);

	/* ADD DSP */
	result = FMOD_ChannelGroup_AddDSP(hMasterChannelGroup, FMOD_CHANNELCONTROL_DSP_HEAD, hDSP);
	ERRCHECK(result);
	
	/* PLAY SOUND */
	result = FMOD_System_PlaySound(system, sound, hMasterChannelGroup, 0, &channel);
    ERRCHECK(result);
	//Sleep(5000);  
	
	/* SEARCH DOMINANT FREQUENCY WHILE IS PLAYING */
	i=0;
	f_dominant_freq_prec = -1;
	do {
		if (_kbhit())
		{
			key = _getch();
			if (key==97)
			{
				result = FMOD_System_PlaySound(system, sound, hMasterChannelGroup, 0, &channel);
				ERRCHECK(result);
				
				f_dominant_freq_prec = -1;
			}
			
		}
		/* UPDATE SYSTEM */
		FMOD_System_Update(system);
		ERRCHECK(result);
		
		/* Get dominate frequency in spectrum */
		f_dominant_freq = 0;
		result = FMOD_DSP_GetParameterFloat(hDSP, FMOD_DSP_FFT_DOMINANT_FREQ, &f_dominant_freq, 0, 0); //FMOD_DSP_FFT_DOMINANT_FREQ
		ERRCHECK(result);
		
		//printf("DSP != NULL5\n");
		if((f_dominant_freq>0.0)&&(f_dominant_freq>f_dominant_freq_prec))
		{	
			i++;
			printf("\nDominant_freq %i : %f\n",i, f_dominant_freq);
			f_dominant_freq_prec = f_dominant_freq;
		}	
		//sleep(1);
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
        // dominanthz  = (float)bin * BINSIZE;       /* dominant frequency min */

        // dominantnote = 0;
        // for (count = 0; count < 120; count++)
        // {
             // if (dominanthz >= notefreq[count] && dominanthz < notefreq[count + 1])
             // {
                // /* which is it closer to.  This note or the next note */
                // if (fabs(dominanthz - notefreq[count]) < fabs(dominanthz - notefreq[count+1]))
                // {
                    // dominantnote = count;
                // }
                // else
                // {
                    // dominantnote = count + 1;
                // }
                // break;
             // }
        // }

        // printf("Detected rate : %7.1f -> %7.1f hz.  Detected musical note. %-3s (%7.1f hz)\r", dominanthz, ((float)bin + 0.99f) * BINSIZE, note[dominantnote], notefreq[dominantnote]);

        // FMOD_System_Update(system);

        // Sleep(10);

    // } while (key != 27);

    // printf("\n");

    // /*
        // Shut down
    // */
    // result = FMOD_Sound_Release(sound);
    // ERRCHECK(result);

    // result = FMOD_System_Release(system);
    // ERRCHECK(result);

    // return 0;
// }

