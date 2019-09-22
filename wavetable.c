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

float env = 0;

float deltaAtt = 0;
float deltaRel = 0;

float phase1 = 0;
float *phase1p = &phase1;

float phase2 = 0;
float *phase2p = &phase2;

float frequence = 0;
float frequenceMod = 0;
float fMod = 1;

float detFactor = 0;
unsigned char note = 0;
unsigned char note_on = 0;

float wave[128];
float wave0[128];
float wave1[128];

float saw(float freq, float *phaseP){
	//incrémenter la phase à chaque appel de la fonction
	float phase = *phaseP;
	float oscOut=0;
	int smallPhase = (int)(phase*128);
	int bigPhase = (smallPhase + 1)%128;
	float ratio = fmod(phase*128,1);
	oscOut = wave[smallPhase]*(1.0-ratio) + wave[bigPhase]*(ratio);
	*phaseP = fmod((phase + freq/44100.0),1.0);
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
						note_on = note;
						}
						
						else if( ((*(in_event.buffer)) & 0xf0) == 0x80 )
						{
					/* note off */
						if(*(in_event.buffer + 1)==note_on){
							note = *(in_event.buffer + 1);
							note_on = 0.0;
							}
						}
					}
				}
			    /* Boucle de traitement sur les échantillons. */
			if (note_on!=0.0){frequence = midi2freq(note_on);}
			frequenceMod = frequence * fMod;
			out1[i] = saw(frequenceMod, &phase1)*env;
			out2[i] = saw(frequence, &phase2)*env;
			if ((note_on!=0.0) && (env < 1)){env = env + deltaAtt;}
			if ((note_on==0.0) && (env > 0)){env = env - deltaRel;}
			if (env < 0){env = 0;}
			if (env > 1){env = 1;}
	}

    return 0;      
}


/*
Une application audio avec deux ports audio d'entrée et deux ports audio de sortie.
*/

int main()
{
//____________________________________
FILE* fichier = NULL;
char lecture[20];
fichier = fopen("wave0", "r");


int i;
for (i=0;i<128;i++){
	fgets(lecture,20,fichier);
	wave0[i] = strtof(lecture,NULL);
}

fichier = fopen("wave1", "r");

for (i=0;i<128;i++){
	fgets(lecture,20,fichier);
	wave1[i] = strtof(lecture,NULL);
}

//_____________________________________

fclose(fichier);
	
if(SDL_Init(SDL_INIT_VIDEO)==-1)
	{
	fprintf(stderr,"erreur a l'initialisation de sdl : %s\n",SDL_GetError());
	exit(EXIT_FAILURE);
	}

SDL_Surface *ecran = NULL;
SDL_WM_SetCaption("wavetable", NULL);
SDL_WM_SetIcon(SDL_LoadBMP("icone.bmp"),NULL);
ecran = SDL_SetVideoMode(147,180,32,SDL_HWSURFACE);
SDL_FillRect(ecran,NULL,SDL_MapRGB(ecran->format,50,50,60));

SDL_Surface *fond = NULL;
fond = SDL_LoadBMP("fond.bmp");
SDL_Rect fondPos;
fondPos.x = 0;
fondPos.y = 0;

SDL_Surface *det = NULL;
det = SDL_LoadBMP("slider0.bmp");
SDL_Rect detPos;
detPos.x = 20 - det->w / 2;
detPos.y = ecran->h/2 - det->h/2;

SDL_Surface *att = NULL;
att = SDL_LoadBMP("slider1.bmp");
SDL_Rect attPos;
attPos.x = 55 - att->w / 2;
attPos.y = ecran->h/2 - att->h/2;

SDL_Surface *rel = NULL;
rel = SDL_LoadBMP("slider2.bmp");
SDL_Rect relPos;
relPos.x = 90 - rel->w / 2;
relPos.y = ecran->h/2 - rel->h/2;

SDL_Surface *lop = NULL;
lop = SDL_LoadBMP("slider3.bmp");
SDL_Rect lopPos;
lopPos.x = 127 - lop->w / 2;
lopPos.y = ecran->h/2 - lop->h/2;

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
		else if(event.button.x<109){sliderActif = 3;}
		else {sliderActif = 4;}
		break;

		case SDL_MOUSEBUTTONUP:
		sliderActif = 0;
		break;

		case SDL_MOUSEMOTION:
		switch(sliderActif)
		{
			case 1:
			detPos.y = event.motion.y - det->h/2;
			break;

			case 2:
			attPos.y = event.motion.y - att->h/2;
			break;

			case 3:
			relPos.y = event.motion.y - rel->h/2;
			break;

			case 4:
			lopPos.y = event.motion.y - lop->h/2;
			break;
		}
		break;
	}
	if(detPos.y > maxY){detPos.y = maxY;}
	if(detPos.y < minY){detPos.y = minY;}
	if(attPos.y > maxY){attPos.y = maxY;}
	if(attPos.y < minY){attPos.y = minY;}
	if(relPos.y > maxY){relPos.y = maxY;}
	if(relPos.y < minY){relPos.y = minY;}
	if(lopPos.y > maxY){lopPos.y = maxY;}
	if(lopPos.y < minY){lopPos.y = minY;}

	SDL_FillRect(ecran,NULL,SDL_MapRGB(ecran->format,0,0,0));
	SDL_BlitSurface(fond,NULL,ecran,&fondPos);
	SDL_BlitSurface(det,NULL,ecran,&detPos);
	SDL_BlitSurface(att,NULL,ecran,&attPos);
	SDL_BlitSurface(rel,NULL,ecran,&relPos);
	SDL_BlitSurface(lop,NULL,ecran,&lopPos);
	
	detFactor = ((detPos.y-minY) / (maxY-minY));
	for(i=0;i<128;i++){
	wave[i] = wave0[i]*detFactor + wave1[i]*(1-detFactor);	
	}

	fMod = ((lopPos.y-minY) / (maxY-minY))+1;
	
	deltaAtt = 0.001*(attPos.y-minY) / (maxY-minY);
	deltaRel = 0.001*(relPos.y-minY) / (maxY-minY);
	
	SDL_Flip(ecran);
}

SDL_FreeSurface(fond);
SDL_FreeSurface(det);
SDL_FreeSurface(att);
SDL_FreeSurface(rel);
SDL_FreeSurface(lop);
SDL_Quit();

    jack_deactivate(client);
    jack_client_close(client);
    
    return 0;
}
