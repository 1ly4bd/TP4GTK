#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "killprocess.h"
#define BUFFER_SIZE 256

void killProcess() {
    FILE* cmd_output;
    char buffer[BUFFER_SIZE];
    int pid = 0;

    // Exécuter la commande tasklist et lire la sortie
    cmd_output = popen("tasklist /fi \"imagename eq cb_console_runner.exe\"", "r");
    if (cmd_output == NULL) {
        printf("Erreur lors de l'exécution de la commande.\n");
        return;
    }

    // Lire chaque ligne de la sortie de la commande
    while (fgets(buffer, BUFFER_SIZE, cmd_output) != NULL) {
        // Rechercher le PID dans la ligne lue
        char* pid_ptr = strstr(buffer, "cb_console_runner.exe");
        if (pid_ptr != NULL) {
            // Extraire le PID de la ligne
            sscanf(pid_ptr + strlen("cb_console_runner.exe"), "%d", &pid);
            break; // Sortir de la boucle une fois que le PID est trouvé
        }
    }

    // Fermer le flux de la commande
    pclose(cmd_output);

    // Exécuter la commande taskkill avec le PID
    if (pid != 0) {
        sprintf(buffer, "taskkill /PID %d", pid);
        system(buffer);
    } else {
        printf("Le processus cb_console_runner.exe n'est pas en cours d'exécution.\n");
    }
}

