// _POSIX_C_SOURCE is defined in Makefile CFLAGS
// System headers <unistd.h>, <stdio.h>, <stdlib.h> are expected to be included
// via ../utils/global.h (which is included by ../include/animation.h).

#include "../include/animation.h" // Project header


// credits :- https://github.com/sahilgajjar/ascii_art
void animated_duck_close(int space, int n)
{
    for(int i = 0; i <= n; i++)
        printf("\n");

    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("   _\n");

    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("__(O)>\n"); // Corrected escape sequence

    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("\\___)\n");
}

void animated_duck_normal(int space)
{
    printf("\n");
    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("   _\n");

    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("__(O)=\n"); // Corrected escape sequence

    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("\\___)\n");
}

void animated_duck_open(int space, int n)
{
    for(int i = 0; i <= n; i++)
        printf("\n");
    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("   _\n");

    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("__(O)<\n"); // Corrected escape sequence

    for(int i = 0; i <= space; i++)
    {
        printf(" ");
    }
    printf("\\___)\n");
}

void animation_open_close(int space)
{
    int frames = 0;
    int start_frame = 0; // Renamed start to start_frame for clarity
    int cycle_count = 0; // Renamed k to cycle_count
    int frame_interval = 12; // Renamed interval to frame_interval

    while(cycle_count < 4)
    {
        for( int i = start_frame; i < frames + frame_interval; i++)
        {
            animated_duck_close(i,space);
            usleep(80000); // usleep is in <unistd.h>, should be available via global.h
            (void)system("clear"); // Cast to void to indicate return value is intentionally ignored
        }

        frames = frames + frame_interval;

        for(int i = frames; i < frames + frame_interval; i++)
        {
            animated_duck_open(i,space);
            usleep(80000);
            (void)system("clear"); // Cast to void
        }

        frames = frames + frame_interval;
        start_frame = frames;

        cycle_count++; // Corrected variable name
    }

}

void animation()
{ 
    int i = 0;
    while(i < 100)
    {
        animated_duck_open(i,1);
        usleep(100000);
        // system("clear");
        i++;
    }
}