#include <stdio.h>
#include "includes/cpu8080.h"
#include "includes/memory.h"

int main()
{
    CPU cpu;

    init_cpu(&cpu);
    printf("Le CPU a bien été initialisé\n");
    load_rom(&cpu, "rom/invaders.rom\n");
    printf("La ROM a bien été chargé\n");

    

    return 0;
}