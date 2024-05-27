#include "tp4.h"
#include "message_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

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
    if (element >= abr->borneInf - 1 && element <= abr->borneSup + 1) {
        abr->borneInf = min(abr->borneInf, element);
        abr->borneSup = max(abr->borneSup, element);
    } else if (element < abr->borneInf - 1) {
        abr->filsGauche = insererElement(abr->filsGauche, element);
    } else if (element > abr->borneSup + 1) {
        abr->filsDroit = insererElement(abr->filsDroit, element);
    }
    return fusionnerSommets(abr);
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

// Fonction pour rechercher le p�re d'un �l�ment
T_Sommet* rechercherPere(T_Arbre abr, int element) {
    if (abr == NULL || (abr->filsGauche == NULL && abr->filsDroit == NULL)) {
        return NULL;
    }

    if ((abr->filsGauche && (element >= abr->filsGauche->borneInf && element <= abr->filsGauche->borneSup)) ||
        (abr->filsDroit && (element >= abr->filsDroit->borneInf && element <= abr->filsDroit->borneSup))) {
        return abr;
    }

    if (element < abr->borneInf) {
        return rechercherPere(abr->filsGauche, element);
    } else if (element > abr->borneSup) {
        return rechercherPere(abr->filsDroit, element);
    }
    return NULL;
}

int niveauDuSommet(T_Arbre racine, T_Sommet *sommet) {
    // Si le sommet est NULL, retourne -1 (niveau non trouv�)
    if (sommet == NULL) {
        return -1;
    }

    int niveau = 0;
    T_Sommet *pere = rechercherPere(racine, sommet->borneInf);

    // Tant qu'il y a un p�re, on continue de remonter vers la racine
    while (pere != NULL) {
        niveau++; // Incr�mente le niveau � chaque �tape
        pere = rechercherPere(racine, pere->borneInf); // Recherche le p�re du p�re
    }

    return niveau;
}

int rechercherHauteur(T_Arbre abr) {
    if (abr == NULL) {
        return 0;
    }

    int hauteurGauche = rechercherHauteur(abr->filsGauche);
    int hauteurDroit = rechercherHauteur(abr->filsDroit);

    return 1 + max(hauteurGauche, hauteurDroit); // Ajoutez 1 pour inclure le n�ud actuel
}


void afficherSommets(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }
    afficherSommets(abr->filsGauche);
    append_to_message_view(g_strdup_printf("[%d; %d]\n", abr->borneInf, abr->borneSup));
    afficherSommets(abr->filsDroit);
}

void afficherElements(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }
    afficherElements(abr->filsGauche);
    for (int i = abr->borneInf; i <= abr->borneSup; i++) {
        append_to_message_view(g_strdup_printf("%d ", i));
    }
    afficherElements(abr->filsDroit);
}

T_Arbre supprimerElement(T_Arbre abr, int element) {
    if (abr == NULL) {
        return NULL;
    }
    T_Sommet *elementASupprimer = rechercherElement(abr, element);
    if (elementASupprimer == NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: L'element %d n'existe pas dans l'arbre. Suppression impossible.\n", element));
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
                    T_Sommet *filsCourant = abr->filsDroit;
                    while (filsCourant->filsGauche != NULL) {
                            filsCourant = filsCourant->filsGauche;
                    }
                    filsCourant->filsGauche = abr->filsGauche;
                    nouvelleRacine = abr->filsDroit;
                }
                free(abr);
                append_to_message_view(g_strdup_printf("\n"));
                append_to_message_view(g_strdup_printf("Suppression de %d.\n", element)); // Affichage d'un message indiquant la suppression r�ussie
                return fusionnerSommets(nouvelleRacine);
            }
        } else {
            if (element == abr->borneInf) {
                abr->borneInf++;
            } else if (element == abr->borneSup) {
                abr->borneSup--;
            } else {
                // Cr�er un nouveau sommet pour [borneInf, element - 1]
                T_Sommet *nouveauGauche = creerSommet(abr->borneInf);
                nouveauGauche->borneSup = element - 1;
                nouveauGauche->filsGauche = abr->filsGauche;

                // Cr�er un nouveau sommet pour [element + 1, borneSup]
                T_Sommet *nouveauDroit = creerSommet(element + 1);
                nouveauDroit->borneSup = abr->borneSup;
                nouveauDroit->filsDroit = abr->filsDroit;

                int x;
                for (x = nouveauDroit->borneInf; x <= nouveauDroit->borneSup; x++) {
                    nouveauGauche = insererElement(nouveauGauche, x);
                }
                if (abr->filsGauche != NULL) {
                        nouveauGauche->filsGauche = abr->filsGauche;

                }
                if (abr->filsGauche != NULL) {
                        nouveauDroit->filsDroit = abr->filsDroit;

                }
                if (abr->filsGauche != NULL) {
                    for (x = abr->filsGauche->borneInf; x <= abr->filsGauche->borneSup; x++) {
                    nouveauGauche = insererElement(nouveauGauche, x);
                }}

                if (abr->filsDroit != NULL) {
                    for (x = abr->filsDroit->borneInf; x <= abr->filsDroit->borneSup; x++) {
                    nouveauGauche = insererElement(nouveauGauche, x);
                }}

                // Lib�rer le n�ud actuel
                free(abr);

                // Retourner le nouveau sous-arbre avec les deux nouveaux sommets
                return fusionnerSommets(nouveauGauche);
            }
        }
    }
    return fusionnerSommets(abr);
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
    unsigned int tailleSommetClassique = sizeof(T_Sommet2);
    unsigned int tailleSommetIntervalles = sizeof(T_Sommet);
    double rapport = (1-((double)tailleIntervalles / tailleClassique)) * 100;
    append_to_message_view(g_strdup_printf("\n"));
    append_to_message_view(g_strdup_printf("Economie de %.2f%%.\n", rapport));
    append_to_message_view(g_strdup_printf("Gain de %d octets. ", tailleClassique - tailleIntervalles));
    append_to_message_view(g_strdup_printf("Memoire de l'arbre classique: %u octets.\n", tailleClassique));
    append_to_message_view(g_strdup_printf("Memoire de l'arbre par intervalles: %u octets. ", tailleIntervalles));
    append_to_message_view(g_strdup_printf("Memoire d'un sommet intervalle: %u octets.\n", tailleSommetIntervalles));
    append_to_message_view(g_strdup_printf("Memoire d'un sommet normal: %u octets. ", tailleSommetClassique));
}

T_Arbre fusionnerSommets(T_Arbre abr) {
    if (abr == NULL) {
        return NULL;
    }

    // Fusionner les sous-arbres gauche et droit
    abr->filsGauche = fusionnerSommets(abr->filsGauche);
    abr->filsDroit = fusionnerSommets(abr->filsDroit);

    // Fusionner les sommets cons�cutifs � gauche
    if (abr->filsGauche != NULL && abr->filsGauche->borneSup + 1 >= abr->borneInf) {
        // Mettre � jour la borneInf du sommet actuel
        abr->borneInf = abr->filsGauche->borneInf;

        // Supprimer le sommet fusionn�
        T_Sommet *temp = abr->filsGauche;
        abr->filsGauche = abr->filsGauche->filsGauche;
        free(temp);
    }

    // Fusionner les sommets cons�cutifs � droite
    if (abr->filsDroit != NULL && abr->filsDroit->borneInf <= abr->borneSup + 1) {
        // Mettre � jour la borneSup du sommet actuel
        abr->borneSup = abr->filsDroit->borneSup;

        // Supprimer le sommet fusionn�
        T_Sommet *temp = abr->filsDroit;
        abr->filsDroit = abr->filsDroit->filsDroit;
        free(temp);
    }

    return abr;
}

// Rotation gauche
T_Arbre rotationGauche(T_Arbre abr) {
    if (abr == NULL || abr->filsDroit == NULL) {
        return abr;
    }

    T_Arbre nouvelleRacine = abr->filsDroit;
    abr->filsDroit = nouvelleRacine->filsGauche;
    nouvelleRacine->filsGauche = abr;
    return nouvelleRacine;
}

// Rotation droite
T_Arbre rotationDroite(T_Arbre abr) {
    if (abr == NULL || abr->filsGauche == NULL) {
        return abr;
    }

    T_Arbre nouvelleRacine = abr->filsGauche;
    abr->filsGauche = nouvelleRacine->filsDroit;
    nouvelleRacine->filsDroit = abr;
    return nouvelleRacine;
}

// �quilibrage de l'arbre
T_Arbre equilibrerArbre(T_Arbre abr) {
    if (abr == NULL) {
        return NULL;
    }

    // �quilibrer les sous-arbres gauche et droit
    abr->filsGauche = equilibrerArbre(abr->filsGauche);
    abr->filsDroit = equilibrerArbre(abr->filsDroit);

    // Calculer la hauteur des sous-arbres gauche et droit
    int hauteurGauche = rechercherHauteur(abr->filsGauche);
    int hauteurDroit = rechercherHauteur(abr->filsDroit);

    // V�rifier l'�quilibre de l'arbre et effectuer les rotations si n�cessaire
    if (hauteurGauche - hauteurDroit > 1) {
        // D�s�quilibre � gauche
        if (rechercherHauteur(abr->filsGauche->filsGauche) < rechercherHauteur(abr->filsGauche->filsDroit)) {
            // Double rotation gauche-droite
            abr->filsGauche = rotationGauche(abr->filsGauche);
        }
        // Rotation droite simple
        abr = rotationDroite(abr);
    } else if (hauteurDroit - hauteurGauche > 1) {
        // D�s�quilibre � droite
        if (rechercherHauteur(abr->filsDroit->filsDroit) < rechercherHauteur(abr->filsDroit->filsGauche)) {
            // Double rotation droite-gauche
            abr->filsDroit = rotationDroite(abr->filsDroit);
        }
        // Rotation gauche simple
        abr = rotationGauche(abr);
    }

    return abr;
}









