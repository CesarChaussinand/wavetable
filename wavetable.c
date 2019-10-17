#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <SDL/SDL.h>
#include "midi.c"

jack_port_t* input_port;
jack_port_t* output_port_left;
jack_port_t* output_port_right;

float env[4] = {0,0,0,0};

float deltaAtt = 0;
float deltaRel = 0;

float phase[4] = {0,0,0,0};

float frequence[4] = {0,0,0,0};

float wavFactor = 0;
unsigned char note = 0;
unsigned char note_on[4] = {0,0,0,0};

int voiceAct = 0;

float wave0l[128];
float wave1l[128];
float wave0r[128];
float wave1r[128];

void init(){
	FILE* fichier = NULL;
	char lecture[20];
	fichier = fopen("wave", "r");

	int i;
	for (i=0;i<128;i++){
		fgets(lecture,20,fichier);
		wave0l[i] = strtof(lecture,NULL);
	}

	for (i=0;i<128;i++){
		fgets(lecture,20,fichier);
		wave0r[i] = strtof(lecture,NULL);
	}

	for (i=0;i<128;i++){
		fgets(lecture,20,fichier);
		wave1l[i] = strtof(lecture,NULL);
	}

	for (i=0;i<128;i++){
		fgets(lecture,20,fichier);
		wave1r[i] = strtof(lecture,NULL);
	}

	fclose(fichier);
	
}

float table(float *phaseP, float *wave){
	//lit dans la table d'onde
	float phase = *phaseP;
	float oscOut=0;
	int smallPhase = (int)(phase*128);
	int bigPhase = (smallPhase + 1)%128;
	float ratio = fmod(phase*128,1);
	oscOut = wave[smallPhase]*(1.0-ratio) + wave[bigPhase]*(ratio);
	return oscOut;
}

int process(jack_nframes_t nframes, void*arg)
{    
	void* port_buf = jack_port_get_buffer(input_port, nframes);
	float* out1 = jack_port_get_buffer(output_port_left, nframes);
	float* out2 = jack_port_get_buffer(output_port_right, nframes);

	int i;
	
	jack_default_audio_sample_t *outl = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_left, nframes);
	jack_default_audio_sample_t *outr = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_right, nframes);
	
	jack_midi_event_t in_event;
	jack_nframes_t event_index = 0;
	jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

	
		for(i=0; i<nframes; i++)
		{
			int k;
				for (k=0;k<event_count+1;k++){
					jack_midi_event_get(&in_event, port_buf, k);

					if((in_event.time == i) && (event_index < event_count))
					{
						if( ((*(in_event.buffer) & 0xf0)) == 0x90 )
						{
					/* note on */
						note = *(in_event.buffer + 1);
							if ((note!=note_on[0])&&(note!=note_on[1])&&(note!=note_on[2])&&(note!=note_on[3])){
								if(note_on[voiceAct]!=0){voiceAct = (voiceAct+1)%4;}
								if(note_on[voiceAct]!=0){voiceAct = (voiceAct+1)%4;}
								if(note_on[voiceAct]!=0){voiceAct = (voiceAct+1)%4;}
						note_on[voiceAct] = note;
						voiceAct = (voiceAct+1)%4;
						int m;
						for(m=0;m<4;m++){ 
						printf("voix %i = %d\n",m,note_on[m]);}
						printf("________________\n");
							}
						}
						
						else if( ((*(in_event.buffer)) & 0xf0) == 0x80 )
						{
					/* note off */
						int j;
							for(j=0;j<4;j++){
							if(*(in_event.buffer + 1)==note_on[j]){
								note = *(in_event.buffer + 1);
								note_on[j] = 0.0;
								}
							}
						}
					}
				}
			    /* Boucle de traitement sur les échantillons. */
			out1[i] =	(
						table(&phase[0], &wave0l[0])*wavFactor*env[0] + table(&phase[0], &wave1l[0])*(1-wavFactor)*env[0] +
						table(&phase[0]+1, &wave0l[0])*wavFactor*env[1] + table(&phase[0]+1, &wave1l[0])*(1-wavFactor)*env[1] +
						table(&phase[0]+2, &wave0l[0])*wavFactor*env[2] + table(&phase[0]+2, &wave1l[0])*(1-wavFactor)*env[2] +
						table(&phase[0]+3, &wave0l[0])*wavFactor*env[3] + table(&phase[0]+3, &wave1l[0])*(1-wavFactor)*env[3]
						)*0.2;
						
			out2[i] =	(
						table(&phase[0], &wave0r[0])*wavFactor*env[0] + table(&phase[0], &wave1r[0])*(1-wavFactor)*env[0] +
						table(&phase[0]+1, &wave0r[0])*wavFactor*env[1] + table(&phase[0]+1, &wave1r[0])*(1-wavFactor)*env[1] +
						table(&phase[0]+2, &wave0r[0])*wavFactor*env[2] + table(&phase[0]+2, &wave1r[0])*(1-wavFactor)*env[2] +
						table(&phase[0]+3, &wave0r[0])*wavFactor*env[3] + table(&phase[0]+3, &wave1r[0])*(1-wavFactor)*env[3]
						)*0.2;
			
			int j;
			for (j=0;j<4;j++){
				if (note_on[j]!=0.0){frequence[j] = midi2freq(note_on[j]);}
				phase[j] = fmod(phase[j] + (frequence[j]/44100.0),1.0);
				if ((note_on[j]!=0.0) && (env[j] < 1)){env[j] = env[j] + deltaAtt;}
				if ((note_on[j]==0.0) && (env[j] > 0)){env[j] = env[j] - deltaRel;}
				if (env[j] < 0){env[j] = 0;}
				if (env[j] > 1){env[j] = 1;}
			}
	}

    return 0;      
}


/*
Une application audio avec deux ports audio d'entrée et deux ports audio de sortie.
*/

int main()
{

init();

if(SDL_Init(SDL_INIT_VIDEO)==-1)
	{
	fprintf(stderr,"erreur a l'initialisation de sdl : %s\n",SDL_GetError());
	exit(EXIT_FAILURE);
	}

SDL_Surface *ecran = NULL;
SDL_WM_SetCaption("wavetable", NULL);
SDL_WM_SetIcon(SDL_LoadBMP("icone.bmp"),NULL);
ecran = SDL_SetVideoMode(110,180,32,SDL_HWSURFACE);
SDL_FillRect(ecran,NULL,SDL_MapRGB(ecran->format,50,50,60));

SDL_Surface *fond = NULL;
fond = SDL_LoadBMP("fond.bmp");
SDL_Rect fondPos;
fondPos.x = 0;
fondPos.y = 0;

SDL_Surface *slider1 = NULL;
slider1 = SDL_LoadBMP("slider0.bmp");
SDL_Rect slider1Pos;
slider1Pos.x = 20 - slider1->w / 2;
slider1Pos.y = ecran->h/2 - slider1->h/2;

SDL_Surface *slider2 = NULL;
slider2 = SDL_LoadBMP("slider1.bmp");
SDL_Rect slider2Pos;
slider2Pos.x = 55 - slider2->w / 2;
slider2Pos.y = ecran->h/2 - slider2->h/2;

SDL_Surface *slider3 = NULL;
slider3 = SDL_LoadBMP("slider2.bmp");
SDL_Rect slider3Pos;
slider3Pos.x = 90 - slider3->w / 2;
slider3Pos.y = ecran->h/2 - slider3->h/2;


int continuer = 1;
SDL_Event event;

int sliderActif = 0;
float maxY = 159.0f;
int minY = 35;

    jack_options_t options = JackNullOption;
    jack_status_t status;

    /* Ouvrir le client JACK */
    jack_client_t* client = jack_client_open("wavetable", options, &status);

    /* Ouvrir les ports en entrée et en sortie */
    output_port_left = jack_port_register(client, "output_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    output_port_right = jack_port_register(client, "output_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	input_port = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	
    /* Enregister le traitement qui sera fait à chaque cycle */
    jack_set_process_callback(client, process, NULL);

    jack_activate(client);

    /* Fonctionne jusqu'à ce que Ctrl+C soit utilisé */
    printf("Utiliser 'echap' pour quitter l'application...\n");


while (continuer)
{
	SDL_WaitEvent(&event);
	switch(event.type)
	{
		case SDL_QUIT:
		continuer = 0;
		break;

		case SDL_KEYDOWN:
		switch(event.key.keysym.sym)
		{
			case SDLK_ESCAPE:
			continuer = 0;
			break;
		}
		break;

		case SDL_MOUSEBUTTONDOWN:
		if(event.button.x<38){sliderActif = 1;}
		else if(event.button.x<73){sliderActif = 2;}
		else {sliderActif = 3;}
		break;

		case SDL_MOUSEBUTTONUP:
		sliderActif = 0;
		break;

		case SDL_MOUSEMOTION:
		switch(sliderActif)
		{
			case 1:
			slider1Pos.y = event.motion.y - slider1->h/2;
			break;

			case 2:
			slider2Pos.y = event.motion.y - slider2->h/2;
			break;

			case 3:
			slider3Pos.y = event.motion.y - slider3->h/2;
			break;
		}
		break;
	}
	if(slider1Pos.y > maxY){slider1Pos.y = maxY;}
	if(slider1Pos.y < minY){slider1Pos.y = minY;}
	if(slider2Pos.y > maxY){slider2Pos.y = maxY;}
	if(slider2Pos.y < minY){slider2Pos.y = minY;}
	if(slider3Pos.y > maxY){slider3Pos.y = maxY;}
	if(slider3Pos.y < minY){slider3Pos.y = minY;}

	SDL_FillRect(ecran,NULL,SDL_MapRGB(ecran->format,0,0,0));
	SDL_BlitSurface(fond,NULL,ecran,&fondPos);
	SDL_BlitSurface(slider1,NULL,ecran,&slider1Pos);
	SDL_BlitSurface(slider2,NULL,ecran,&slider2Pos);
	SDL_BlitSurface(slider3,NULL,ecran,&slider3Pos);
	
	wavFactor = (slider1Pos.y-minY) / (maxY-minY);

	deltaAtt = 0.001*(slider2Pos.y-minY) / (maxY-minY);
	deltaRel = 0.001*(slider3Pos.y-minY) / (maxY-minY);
	
	SDL_Flip(ecran);
}

SDL_FreeSurface(fond);
SDL_FreeSurface(slider1);
SDL_FreeSurface(slider2);
SDL_FreeSurface(slider3);
SDL_Quit();

    jack_deactivate(client);
    jack_client_close(client);
    
    return 0;
}
