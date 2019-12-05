#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <SDL/SDL.h>
#include "wave.h"
#include "midi.c"

/*	
 * régler le bug dans le haut de la wavetable, produire plein d'autre wavetables, implémenter une vrai envellope, 
 * implémenter un effet stéréo (reverb ?)
 * envellope en pourcentages de la valeur choisi (ex. wav choisit 24, enveloppe entre 0 et 24 ou 24 et 64)
 * 
 * pour compiler : gcc wavetable.c -g -ljack -lm -lSDL
 */

 char *filename;
 
jack_port_t* input_port;
jack_port_t* output_port_left;
jack_port_t* output_port_right;

float env[4] = {0,0,0,0};

float deltaAtt = 0;
float deltaRel = 0;

float phase[4] = {0,0,0,0};

float frequence[4] = {0,0,0,0};

float wavFact[4] = {0,0,0,0};
unsigned char note = 0;
unsigned char note_on[4] = {0,0,0,0};

int lfoAssign = 0;
float lfoAmount = 0;
float lfoRate = 0;
float lfoValueLeft = 0;
float lfoValueRight = 0;
float lfoPhase = 0;
float stereoSpread = 1; //influe sur la phase du lfo et l'effet de delay

int voiceAct = 0;

long size;

float wave[128][64][2];

typedef struct HEADER Header;

void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

void init(){
	FILE* fichier = NULL;
	char lecture[20];
	fichier = fopen(filename, "r");

	int x;
	int y;
	int z;
	
	for (z=0;z<2;z++){
		for (y=0;y<64;y++){
			for (x=0;x<128;x++){
		fgets(lecture,20,fichier);
		wave[x][y][z] = strtof(lecture,NULL);
			}
		}
	}
	fclose(fichier);
	
}

float table(float phase, float wavFact, int channel){
	//lit dans la table d'onde
	float oscOut=0;
	int smallPhase = (int)(phase*128);
	int bigPhase = (smallPhase + 1)%128;
	float phaseRatio = fmod(phase*128,1);
	
	int smallWavFact = (int)(wavFact*63);
	int bigWavFact = (smallWavFact + 1)%63;
	float wavRatio = fmod(wavFact*63,1);

	oscOut =	(wave[smallPhase][smallWavFact][channel]*(1.0-phaseRatio) + wave[bigPhase][smallWavFact][channel]*(phaseRatio))*(1.0-wavRatio) + 
				(wave[smallPhase][bigWavFact][channel]*(1.0-phaseRatio) + wave[bigPhase][bigWavFact][channel]*(phaseRatio))*(wavRatio) ;
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
						printf("\n__________________________\n");
						printf("voix | 1  | 2  | 3  | 4  |\n");
						printf("-----|----|----|----|----|\n");
						printf("midi | ");
							for(int i = 0;i<4;i++){
								if (note_on[i]!=0){
									printf("%i | ",note_on[i]);}
								else{	printf("   | ");}
							}
						printf("\n");
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
						table(phase[0]+lfoValueLeft*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[0]+(lfoValueLeft*lfoAmount*0.4*(lfoAssign==3)),0),1)	,0)*env[0] + 
						table(phase[1]+lfoValueLeft*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[1]+(lfoValueLeft*lfoAmount*0.4*(lfoAssign==3)),0),1)	,0)*env[1] + 
						table(phase[2]+lfoValueLeft*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[2]+(lfoValueLeft*lfoAmount*0.4*(lfoAssign==3)),0),1)	,0)*env[2] + 
						table(phase[3]+lfoValueLeft*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[3]+(lfoValueLeft*lfoAmount*0.4*(lfoAssign==3)),0),1)	,0)*env[3]
						)*0.2*(1+(lfoValueLeft*lfoAmount*(lfoAssign==2)));
						
			out2[i] =	(
						table(phase[0]+lfoValueRight*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[0]+(lfoValueRight*lfoAmount*0.4*(lfoAssign==3)),0),1)	,1)*env[0] +
						table(phase[1]+lfoValueRight*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[1]+(lfoValueRight*lfoAmount*0.4*(lfoAssign==3)),0),1)	,1)*env[1] +
						table(phase[2]+lfoValueRight*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[2]+(lfoValueRight*lfoAmount*0.4*(lfoAssign==3)),0),1)	,1)*env[2] +
						table(phase[3]+lfoValueRight*lfoAmount*0.05*(lfoAssign==1),	fmin(fmax(wavFact[3]+(lfoValueRight*lfoAmount*0.4*(lfoAssign==3)),0),1)	,1)*env[3]
						)*0.2*(1+(lfoValueRight*lfoAmount*(lfoAssign==2)));
			
			int j;
			for (j=0;j<4;j++){
				if (note_on[j]!=0.0){frequence[j] = midi2freq(note_on[j]);}
				phase[j] = fmod(phase[j] + (frequence[j]/44100.0) ,1.0);
				//phase[j] = fmod(phase[j] + (frequence[j]*(1+lfoValue*lfoAmount*0.05*(lfoAssign==1))/44100.0) ,1.0);
				if ((note_on[j]!=0.0) && (env[j] < 1)){env[j] = env[j] + deltaAtt;}
				if ((note_on[j]==0.0) && (env[j] > 0)){env[j] = env[j] - deltaRel;}
				if (env[j] < 0){env[j] = 0;}
				if (env[j] > 1){env[j] = 1;}
			}
			
			/* avancement du lfo */
			lfoPhase = fmod(lfoPhase + (lfoRate/44100.0),1.0);
			lfoValueLeft = sin((lfoPhase+(stereoSpread*0.25))*2.0*3.141593);
			lfoValueRight = sin((lfoPhase-(stereoSpread*0.25))*2.0*3.141593);
	}

    return 0;      
}


/*
Une application audio avec deux ports audio d'entrée et deux ports audio de sortie.
*/

int main(int argc, char **argv){
	
 filename = (char*) malloc(sizeof(char) * 1024);
 if (filename == NULL) {
   printf("Error in malloc\n");
   exit(1);
 }

 // get file path
 char cwd[1024];
 if (getcwd(cwd, sizeof(cwd)) != NULL) {
   
	strcpy(filename, cwd);

	// get filename from command line
	if (argc < 2) {
	  printf("No wavetable file specified\n");
	  return(1);
	}
	
	strcat(filename, "/");
	strcat(filename, argv[1]);
	printf("%s\n", filename);
 }

init();


if(SDL_Init(SDL_INIT_VIDEO)==-1)
	{
	fprintf(stderr,"erreur a l'initialisation de sdl : %s\n",SDL_GetError());
	exit(EXIT_FAILURE);
	}

SDL_Surface *ecran = NULL;
SDL_WM_SetCaption("wavetable", NULL);
SDL_WM_SetIcon(SDL_LoadBMP("icone.bmp"),NULL);
ecran = SDL_SetVideoMode(380,360,32,SDL_HWSURFACE);
SDL_FillRect(ecran,NULL,SDL_MapRGB(ecran->format,50,50,60));

SDL_Surface *fond = NULL;
fond = SDL_LoadBMP("fond.bmp");
SDL_Rect fondPos;
fondPos.x = 0;
fondPos.y = 0;

SDL_Surface *slider1 = NULL;
slider1 = SDL_LoadBMP("slider0.bmp");
SDL_Rect slider1Pos;
slider1Pos.x = 150 - slider1->w / 2;
slider1Pos.y = 159;

SDL_Surface *slider2 = NULL;
slider2 = SDL_LoadBMP("slider1.bmp");
SDL_Rect slider2Pos;
slider2Pos.x = 185 - slider2->w / 2;
slider2Pos.y = 159;

SDL_Surface *slider3 = NULL;
slider3 = SDL_LoadBMP("slider2.bmp");
SDL_Rect slider3Pos;
slider3Pos.x = 220 - slider3->w / 2;
slider3Pos.y = 97;

SDL_Surface *boutonFreq = NULL;
boutonFreq = SDL_LoadBMP("slider0.bmp");
SDL_Rect boutonFreqPos;
boutonFreqPos.x = 275 - slider1->w / 2;
boutonFreqPos.y = 62 - slider1->h/2;

SDL_Surface *boutonAmp = NULL;
boutonAmp = SDL_LoadBMP("slider1.bmp");
SDL_Rect boutonAmpPos;
boutonAmpPos.x = 275 - slider1->w / 2;
boutonAmpPos.y = 107 - slider1->h/2;

SDL_Surface *boutonWav = NULL;
boutonWav = SDL_LoadBMP("slider2.bmp");
SDL_Rect boutonWavPos;
boutonWavPos.x = 275 - slider1->w / 2;
boutonWavPos.y = 151 - slider1->h/2;

SDL_Surface *slider4 = NULL;
slider4 = SDL_LoadBMP("slider0.bmp");
SDL_Rect slider4Pos;
slider4Pos.x = 325 - slider4->w / 2;
slider4Pos.y = 159;

SDL_Surface *slider5 = NULL;
slider5 = SDL_LoadBMP("slider1.bmp");
SDL_Rect slider5Pos;
slider5Pos.x = 360 - slider5->w / 2;
slider5Pos.y = 159;

SDL_Surface *slider6 = NULL;
slider6 = SDL_LoadBMP("slider1.bmp");
SDL_Rect slider6Pos;
slider6Pos.x = 20 - slider6->w / 2;
slider6Pos.y = 339;

SDL_Surface *slider7 = NULL;
slider7 = SDL_LoadBMP("slider1.bmp");
SDL_Rect slider7Pos;
slider7Pos.x = 55 - slider7->w / 2;
slider7Pos.y = 339;

SDL_Surface *slider8 = NULL;
slider8 = SDL_LoadBMP("slider1.bmp");
SDL_Rect slider8Pos;
slider8Pos.x = 90 - slider8->w / 2;
slider8Pos.y = 339;

SDL_Surface *slider9 = NULL;
slider9 = SDL_LoadBMP("slider1.bmp");
SDL_Rect slider9Pos;
slider9Pos.x = 125 - slider9->w / 2;
slider9Pos.y = 339;

SDL_Surface *slider10 = NULL;
slider10 = SDL_LoadBMP("slider2.bmp");
SDL_Rect slider10Pos;
slider10Pos.x = 164 - slider10->w / 2;
slider10Pos.y = 339;

SDL_Surface *slider11 = NULL;
slider11 = SDL_LoadBMP("slider2.bmp");
SDL_Rect slider11Pos;
slider11Pos.x = 199 - slider11->w / 2;
slider11Pos.y = 339;

SDL_Surface *slider12 = NULL;
slider12 = SDL_LoadBMP("slider2.bmp");
SDL_Rect slider12Pos;
slider12Pos.x = 234 - slider12->w / 2;
slider12Pos.y = 339;

SDL_Surface *slider13 = NULL;
slider13 = SDL_LoadBMP("slider2.bmp");
SDL_Rect slider13Pos;
slider13Pos.x = 269 - slider13->w / 2;
slider13Pos.y = 339;


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
		if(event.button.y<180){
			if(event.button.x<168){sliderActif = 1;}
			else if(event.button.x<203){sliderActif = 2;}
			else if(event.button.x<240){sliderActif = 3;}
			else if(event.button.x<310){
				if(event.button.y<100){if(lfoAssign!=1){lfoAssign = 1;}else{lfoAssign = 0;}}
				else if(event.button.y<135){if(lfoAssign!=2){lfoAssign = 2;} else{lfoAssign = 0;}}
				else if(event.button.y<175){if(lfoAssign!=3){lfoAssign = 3;} else{lfoAssign = 0;}}
				else{lfoAssign = 0;}
				}
			else if(event.button.x<345){sliderActif = 4;}
			else {sliderActif = 5;}
			}
		else{
			if(event.button.x<35){sliderActif = 6;}
			}
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
			
			case 4:
			slider4Pos.y = event.motion.y - slider4->h/2;
			break;
			
			case 5:
			slider5Pos.y = event.motion.y - slider5->h/2;
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
	if(slider4Pos.y > maxY){slider4Pos.y = maxY;}
	if(slider4Pos.y < minY){slider4Pos.y = minY;}
	if(slider5Pos.y > maxY){slider5Pos.y = maxY;}
	if(slider5Pos.y < minY){slider5Pos.y = minY;}

	SDL_FillRect(ecran,NULL,SDL_MapRGB(ecran->format,0,0,0));
	SDL_BlitSurface(fond,NULL,ecran,&fondPos);
	SDL_BlitSurface(slider1,NULL,ecran,&slider1Pos);
	SDL_BlitSurface(slider2,NULL,ecran,&slider2Pos);
	SDL_BlitSurface(slider3,NULL,ecran,&slider3Pos);
	SDL_BlitSurface(slider4,NULL,ecran,&slider4Pos);
	SDL_BlitSurface(slider5,NULL,ecran,&slider5Pos);
	SDL_BlitSurface(slider6,NULL,ecran,&slider6Pos);
	SDL_BlitSurface(slider7,NULL,ecran,&slider7Pos);
	SDL_BlitSurface(slider8,NULL,ecran,&slider8Pos);
	SDL_BlitSurface(slider9,NULL,ecran,&slider9Pos);
	SDL_BlitSurface(slider10,NULL,ecran,&slider10Pos);
	SDL_BlitSurface(slider11,NULL,ecran,&slider11Pos);
	SDL_BlitSurface(slider12,NULL,ecran,&slider12Pos);
	SDL_BlitSurface(slider13,NULL,ecran,&slider13Pos);
	
	switch(lfoAssign){
		case 1:
		SDL_BlitSurface(boutonFreq,NULL,ecran,&boutonFreqPos);
		break;
		
		case 2:
		SDL_BlitSurface(boutonAmp,NULL,ecran,&boutonAmpPos);
		break;
		
		case 3:
		SDL_BlitSurface(boutonWav,NULL,ecran,&boutonWavPos);
		break;	
	}
	
	int i;
	for(i=0;i<4;i++){
	wavFact[i] = 1-((slider1Pos.y-minY) / (maxY-minY));
}
	deltaAtt = 0.00001+(0.001*(slider2Pos.y-minY) / (maxY-minY));
	deltaRel = 0.00001+(0.001*(slider3Pos.y-minY) / (maxY-minY));
	lfoAmount = (slider4Pos.y-minY) / (maxY-minY);
	lfoRate = pow((slider5Pos.y-minY)*3.5 / (maxY-minY),2);
	
	SDL_LockSurface(ecran); /* On bloque la surface */
	
	for (i=0;i<128;i++){
		setPixel(ecran, i+1,45 + (int)(44*table(i/128.0,wavFact[0],0)), SDL_MapRGB(ecran->format, 255, 255, 255));
		setPixel(ecran, i+1,135 + (int)(44*table(i/128.0,wavFact[0],1)), SDL_MapRGB(ecran->format, 255, 255, 255));
		}
	SDL_UnlockSurface(ecran); /* On débloque la surface*/
	
	SDL_Flip(ecran);
}

SDL_FreeSurface(fond);
SDL_FreeSurface(slider1);
SDL_FreeSurface(slider2);
SDL_FreeSurface(slider3);
SDL_FreeSurface(slider4);
SDL_FreeSurface(slider5);
SDL_FreeSurface(boutonFreq);
SDL_FreeSurface(boutonAmp);
SDL_FreeSurface(boutonWav);
SDL_FreeSurface(slider6);
SDL_FreeSurface(slider7);
SDL_FreeSurface(slider8);
SDL_FreeSurface(slider9);
SDL_FreeSurface(slider10);
SDL_FreeSurface(slider11);
SDL_FreeSurface(slider12);
SDL_FreeSurface(slider13);

SDL_Quit();

    jack_deactivate(client);
    jack_client_close(client);
    
    return 0;
}


void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel){
	
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;
    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}
