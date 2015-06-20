#include "jmalloc.hpp"
#include "sound.hpp"

void free_up_memory()
{
}

extern int jmalloc_max_size;

main()
{
	int argc = 0;
	char *argv[] = { "" };
	
	jmalloc_max_size = 400000;
	jmalloc_init(300000);
	
	sound_init(argc, argv);
	
	sound_effect snd("sfx_switch01.wav");
	
	snd.play();
	snd.play();
	snd.play();
	snd.play();

	snd.play();
	snd.play();
	snd.play();
	snd.play();

	while (!Button()) ;
	
	snd.play();
	snd.play();
	snd.play();
	snd.play();

	snd.play();
	snd.play();
	snd.play();
	snd.play();

	sound_uninit();
}
