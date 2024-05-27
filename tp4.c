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

// Fonction pour rechercher le père d'un élément
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
    // Si le sommet est NULL, retourne -1 (niveau non trouvé)
    if (sommet == NULL) {
        return -1;
    }

    int niveau = 0;
    T_Sommet *pere = rechercherPere(racine, sommet->borneInf);

    // Tant qu'il y a un père, on continue de remonter vers la racine
    while (pere != NULL) {
        niveau++; // Incrémente le niveau à chaque étape
        pere = rechercherPere(racine, pere->borneInf); // Recherche le père du père
    }

    return niveau;
}

int rechercherHauteur(T_Arbre abr) {
    if (abr == NULL) {
        return 0;
    }

    int hauteurGauche = rechercherHauteur(abr->filsGauche);
    int hauteurDroit = rechercherHauteur(abr->filsDroit);

    return 1 + max(hauteurGauche, hauteurDroit); // Ajoutez 1 pour inclure le nœud actuel
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
                append_to_message_view(g_strdup_printf("Suppression de %d.\n", element)); // Affichage d'un message indiquant la suppression réussie
                return fusionnerSommets(nouvelleRacine);
            }
        } else {
            if (element == abr->borneInf) {
                abr->borneInf++;
            } else if (element == abr->borneSup) {
                abr->borneSup--;
            } else {
                // Créer un nouveau sommet pour [borneInf, element - 1]
                T_Sommet *nouveauGauche = creerSommet(abr->borneInf);
                nouveauGauche->borneSup = element - 1;
                nouveauGauche->filsGauche = abr->filsGauche;

                // Créer un nouveau sommet pour [element + 1, borneSup]
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

                // Libérer le nœud actuel
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

    // Fusionner les sommets consécutifs à gauche
    if (abr->filsGauche != NULL && abr->filsGauche->borneSup + 1 >= abr->borneInf) {
        // Mettre à jour la borneInf du sommet actuel
        abr->borneInf = abr->filsGauche->borneInf;

        // Supprimer le sommet fusionné
        T_Sommet *temp = abr->filsGauche;
        abr->filsGauche = abr->filsGauche->filsGauche;
        free(temp);
    }

    // Fusionner les sommets consécutifs à droite
    if (abr->filsDroit != NULL && abr->filsDroit->borneInf <= abr->borneSup + 1) {
        // Mettre à jour la borneSup du sommet actuel
        abr->borneSup = abr->filsDroit->borneSup;

        // Supprimer le sommet fusionné
        T_Sommet *temp = abr->filsDroit;
        abr->filsDroit = abr->filsDroit->filsDroit;
        free(temp);
    }

    return abr;
}

// Effectue une rotation à gauche sur l'arbre pour équilibrer les hauteurs des sous-arbres.
T_Arbre rotationGauche(T_Arbre abr) {
    printf("Rotation gauche.\n");
    // Sauvegarde le sous-arbre droit du nœud actuel
    T_Arbre nouvelArbre = abr->filsDroit;
    // Le sous-arbre gauche du sous-arbre droit devient le nouveau sous-arbre droit du nœud actuel
    abr->filsDroit = nouvelArbre->filsGauche;
    // Le nœud actuel devient le sous-arbre gauche du sous-arbre droit
    nouvelArbre->filsGauche = abr;
    // Retourne le nouvel arbre équilibré
    return nouvelArbre;
}

// Effectue une rotation à droite sur l'arbre pour équilibrer les hauteurs des sous-arbres.
T_Arbre rotationDroite(T_Arbre abr) {
    printf("Rotation droite.\n");
    // Sauvegarde le sous-arbre gauche du nœud actuel
    T_Arbre nouvelArbre = abr->filsGauche;
    // Le sous-arbre droit du sous-arbre gauche devient le nouveau sous-arbre gauche du nœud actuel
    abr->filsGauche = nouvelArbre->filsDroit;
    // Le nœud actuel devient le sous-arbre droit du sous-arbre gauche
    nouvelArbre->filsDroit = abr;
    // Retourne le nouvel arbre équilibré
    return nouvelArbre;
}

// Équilibre l'arbre en effectuant des rotations selon les cas de déséquilibre.
T_Arbre equilibrerArbre(T_Arbre abr) {
    if (abr == NULL) {
        return NULL;
    }

    // Équilibrer les sous-arbres gauche et droit récursivement
    abr->filsGauche = equilibrerArbre(abr->filsGauche);
    abr->filsDroit = equilibrerArbre(abr->filsDroit);

    // Calculer les hauteurs des sous-arbres gauche et droit
    int hauteurGauche = rechercherHauteur(abr->filsGauche);
    int hauteurDroit = rechercherHauteur(abr->filsDroit);

    // Calculer la différence de hauteur
    int diffHauteur = hauteurGauche - hauteurDroit;

    // Cas de déséquilibre à gauche
    if (diffHauteur > 1) {
        // Cas de rotation simple à droite
        if (rechercherHauteur(abr->filsGauche->filsGauche) >= rechercherHauteur(abr->filsGauche->filsDroit)) {
            printf("Cas de rotation simple à droite.\n");
            abr = rotationDroite(abr);
        }
        // Cas de rotation double à gauche-droite
        else {
            printf("Cas de rotation double à gauche-droite.\n");
            abr->filsGauche = rotationGauche(abr->filsGauche);
            abr = rotationDroite(abr);
        }
    }
    // Cas de déséquilibre à droite
    else if (diffHauteur < -1) {
        // Cas de rotation simple à gauche
        if (rechercherHauteur(abr->filsDroit->filsDroit) >= rechercherHauteur(abr->filsDroit->filsGauche)) {
            printf("Cas de rotation simple à gauche.\n");
            abr = rotationGauche(abr);
        }
        // Cas de rotation double à droite-gauche
        else {
            printf("Cas de rotation double à droite-gauche.\n");
            abr->filsDroit = rotationDroite(abr->filsDroit);
            abr = rotationGauche(abr);
        }
    }

    // Retourne l'arbre équilibré
    return abr;
}












