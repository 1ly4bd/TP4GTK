#include "tp4.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#define max(a,b) ({ typeof (a) _a = (a); typeof (b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({ typeof (a) _a = (a); typeof (b) _b = (b); _a < _b ? _a : _b; })

T_Sommet *creerSommet(int element) {
    T_Sommet *sommet = (T_Sommet *)malloc(sizeof(T_Sommet));
    sommet->borneInf = element;
    sommet->borneSup = element;
    sommet->filsGauche = NULL;
    sommet->filsDroit = NULL;
    return sommet;
}

T_Arbre insererElement(T_Arbre abr, int element) {
    if (abr == NULL) {
        return creerSommet(element);
    }
    if (rechercherElement(abr, element) != NULL) {
        printf("L'element %d est deja present dans l'arbre.\n", element);
        return abr;
    }
    if (element >= abr->borneInf - 1 && element <= abr->borneSup + 1) {
        abr->borneInf = min(abr->borneInf, element);
        abr->borneSup = max(abr->borneSup, element);
    } else if (element < abr->borneInf - 1) {
        abr->filsGauche = insererElement(abr->filsGauche, element);
    } else if (element > abr->borneSup + 1) {
        abr->filsDroit = insererElement(abr->filsDroit, element);
    }
    return abr;
}

T_Sommet *rechercherElement(T_Arbre abr, int element) {
    if (abr == NULL) {
        return NULL;
    }
    if (element >= abr->borneInf && element <= abr->borneSup) {
        return abr;
    } else if (element > abr->borneSup) {
        return rechercherElement(abr->filsDroit, element);
    } else {
        return rechercherElement(abr->filsGauche, element);
    }
}

void afficherSommets(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }
    afficherSommets(abr->filsGauche);
    printf("[%d; %d]\n", abr->borneInf, abr->borneSup);
    afficherSommets(abr->filsDroit);
}

void afficherElements(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }
    afficherElements(abr->filsGauche);
    for (int i = abr->borneInf; i <= abr->borneSup; i++) {
        printf("%d ", i);
    }
    afficherElements(abr->filsDroit);
}

T_Arbre supprimerElement(T_Arbre abr, int element) {
    if (abr == NULL) {
        return NULL;
    }
    T_Sommet *elementASupprimer = rechercherElement(abr, element);
    if (elementASupprimer == NULL) {
        printf("L'element %d n'existe pas dans l'arbre. Suppression impossible.\n", element);
        return abr;
    }
    if (element < abr->borneInf) {
        abr->filsGauche = supprimerElement(abr->filsGauche, element);
    } else if (element > abr->borneSup) {
        abr->filsDroit = supprimerElement(abr->filsDroit, element);
    } else {
        if (abr->borneInf == abr->borneSup) {
            if (abr->borneInf == element) {
                T_Sommet *nouvelleRacine = NULL;
                if (abr->filsGauche == NULL) {
                    nouvelleRacine = abr->filsDroit;
                } else if (abr->filsDroit == NULL) {
                    nouvelleRacine = abr->filsGauche;
                } else {
                    T_Sommet *noeudMax = abr->filsGauche;
                    while (noeudMax->filsDroit != NULL) {
                        noeudMax = noeudMax->filsDroit;
                    }
                    int temp = abr->borneInf;
                    abr->borneInf = noeudMax->borneInf;
                    noeudMax->borneInf = temp;
                    abr->filsGauche = supprimerElement(abr->filsGauche, noeudMax->borneInf);
                    return abr;
                }
                free(abr);
                return nouvelleRacine;
            }
        } else {
            if (abr->borneInf == element) {
                abr->borneInf++;
            } else if (abr->borneSup == element) {
                abr->borneSup--;
            }
        }
    }
    return abr;
}

unsigned int tailleMemoireIntervalles(T_Arbre abr) {
    if (abr == NULL) {
        return 0;
    }
    unsigned int tailleArbre = sizeof(T_Sommet);
    return tailleArbre + tailleMemoireIntervalles(abr->filsGauche) + tailleMemoireIntervalles(abr->filsDroit);
}

unsigned int tailleMemoireClassique(T_Arbre abr) {
    if (abr == NULL) {
        return 0;
    }
    unsigned int tailleArbre = sizeof(T_Sommet2);
    unsigned int nbElements = abr->borneSup - abr->borneInf + 1;
    return (tailleArbre * nbElements) + tailleMemoireClassique(abr->filsGauche) + tailleMemoireClassique(abr->filsDroit);
}

void tailleMemoire(T_Arbre abr) {
    unsigned int tailleIntervalles = tailleMemoireIntervalles(abr);
    unsigned int tailleClassique = tailleMemoireClassique(abr);
    printf("Taille memoire de l'arbre (version intervalles): %u octets\n", tailleIntervalles);
    printf("Taille memoire de l'arbre (version classique): %u octets\n", tailleClassique);
    printf("Nombre d'octets gagnes par la representation par intervalles: %d\n", tailleClassique - tailleIntervalles);
    double rapport = (1-((double)tailleIntervalles / tailleClassique)) * 100;
    printf("La representation par intervalles occupe %.2f%% de moins que la representation classique.\n", rapport);
}

T_Arbre obtenirRacine(T_Arbre abr) {
    return abr;
}

void insererPlusieursElements(T_Arbre *abr) {
    char buffer[256];
    printf("Entrez les elements a inserer (separes par des espaces): ");
    scanf(" %[^\n]", buffer);

    char *token = strtok(buffer, " ");
    while (token != NULL) {
        int element = atoi(token);
        *abr = insererElement(*abr, element);
        token = strtok(token + strlen(token) + 1, " ");
    }
    printf("Elements inseres.\n");
}

void supprimerPlusieursElements(T_Arbre *abr) {
    char buffer[256];
    printf("Entrez les elements a supprimer (separes par des espaces): ");
    scanf(" %[^\n]", buffer);

    char *token = strtok(buffer, " ");
    while (token != NULL) {
        int element = atoi(token);
        *abr = supprimerElement(*abr, element);
        token = strtok(token + strlen(token) + 1, " ");
    }
    printf("Elements supprimes.\n");
}
