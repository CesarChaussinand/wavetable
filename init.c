

int size_measurement(Header wave0,Header wave1){
	long size;
	FILE *fichier0;
	FILE *fichier1;
	
	unsigned char buffer4[4];
	unsigned char buffer2[2];

	fichier0 = fopen("wave0.wav","rb");
	fichier1 = fopen("wave1.wav","rb");
	
		if ((fichier0 == NULL)||(fichier1 == NULL)) {
		printf("Error opening file\n");
		exit(1);
		}
	
	int read = 0;
	read = fread(wave0->riff, sizeof(wave0->riff), 1, fichier0);
	read = fread(buffer4, sizeof(buffer4), 1, fichier0);
	wave0->overall_size  = buffer4[0] | 
						(buffer4[1]<<8) | 
						(buffer4[2]<<16) | 
						(buffer4[3]<<24);
	read = fread(wave0->wave, sizeof(wave0->wave), 1, fichier0);
	read = fread(wave0->fmt_chunk_marker, sizeof(wave0->fmt_chunk_marker), 1, fichier0);
	read = fread(buffer4, sizeof(buffer4), 1, fichier0);
	wave0->length_of_fmt = buffer4[0] |
							(buffer4[1] << 8) |
							(buffer4[2] << 16) |
							(buffer4[3] << 24);
	read = fread(buffer2, sizeof(buffer2), 1, fichier0);
	read = fread(buffer2, sizeof(buffer2), 1, fichier0);
	wave0->channels = buffer2[0] | (buffer2[1] << 8);
	read = fread(buffer4, sizeof(buffer4), 1, fichier0);
	wave0->sample_rate = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
	read = fread(buffer4, sizeof(buffer4), 1, fichier0);
	wave0->byterate  = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
	read = fread(buffer2, sizeof(buffer2), 1, fichier0);
	wave0->block_align = buffer2[0] |
					(buffer2[1] << 8);
	read = fread(buffer2, sizeof(buffer2), 1, fichier0);
	wave0->bits_per_sample = buffer2[0] |
					(buffer2[1] << 8);
	read = fread(wave0->data_chunk_header, sizeof(wave0->data_chunk_header), 1, fichier0);
	read = fread(buffer4, sizeof(buffer4), 1, fichier0);
	wave0->data_size = buffer4[0] |
				(buffer4[1] << 8) |
				(buffer4[2] << 16) | 
				(buffer4[3] << 24 );
	
long num_samples_0 = (8 * wave0->data_size) / (wave0->channels * wave0->bits_per_sample);
	
	read = 0;
	read = fread(wave1->riff, sizeof(wave1->riff), 1, fichier1);
	read = fread(buffer4, sizeof(buffer4), 1, fichier1);
	wave1->overall_size  = buffer4[0] | 
						(buffer4[1]<<8) | 
						(buffer4[2]<<16) | 
						(buffer4[3]<<24);
	read = fread(wave1->wave, sizeof(wave1->wave), 1, fichier1);
	read = fread(wave1->fmt_chunk_marker, sizeof(wave1->fmt_chunk_marker), 1, fichier1);
	read = fread(buffer4, sizeof(buffer4), 1, fichier1);
	wave1->length_of_fmt = buffer4[0] |
							(buffer4[1] << 8) |
							(buffer4[2] << 16) |
							(buffer4[3] << 24);
	read = fread(buffer2, sizeof(buffer2), 1, fichier1);
	read = fread(buffer2, sizeof(buffer2), 1, fichier1);
	wave1->channels = buffer2[0] | (buffer2[1] << 8);
	read = fread(buffer4, sizeof(buffer4), 1, fichier1);
	wave1->sample_rate = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
	read = fread(buffer4, sizeof(buffer4), 1, fichier1);
	wave1->byterate  = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
	read = fread(buffer2, sizeof(buffer2), 1, fichier1);
	wave1->block_align = buffer2[0] |
					(buffer2[1] << 8);
	read = fread(buffer2, sizeof(buffer2), 1, fichier1);
	wave1->bits_per_sample = buffer2[0] |
					(buffer2[1] << 8);
	read = fread(wave1->data_chunk_header, sizeof(wave1->data_chunk_header), 1, fichier1);
	read = fread(buffer4, sizeof(buffer4), 1, fichier1);
	wave1->data_size = buffer4[0] |
				(buffer4[1] << 8) |
				(buffer4[2] << 16) | 
				(buffer4[3] << 24 );
	
long num_samples_1 = (8 * wave1->data_size) / (wave1->channels * wave1->bits_per_sample);
	
	if (num_samples_0 < num_samples_1){
		size = num_samples_0;
	}else{ size = num_samples_1;}
	
	
	return size;
}
