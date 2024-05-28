#include "tp4.h"
#include "message_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

// Définition des fonctions utilitaires

// Fonction pour trouver le maximum entre deux valeurs
#define max(a,b) ({ typeof (a) _a = (a); typeof (b) _b = (b); _a > _b ? _a : _b; })

// Fonction pour trouver le minimum entre deux valeurs
#define min(a,b) ({ typeof (a) _a = (a); typeof (b) _b = (b); _a < _b ? _a : _b; })

// Fonction pour créer un sommet avec un élément donné
T_Sommet *creerSommet(int element) {
    T_Sommet *sommet = (T_Sommet *)malloc(sizeof(T_Sommet));
    sommet->borneInf = element;
    sommet->borneSup = element;
    sommet->filsGauche = NULL;
    sommet->filsDroit = NULL;
    return sommet;
}

// Fonction pour insérer un élément dans un arbre
T_Arbre insererElement(T_Arbre abr, int element) {
    if (abr == NULL) {
        return creerSommet(element);
    }
    // Si l'élément est dans l'intervalle [borneInf - 1, borneSup + 1]
    if (element >= abr->borneInf - 1 && element <= abr->borneSup + 1) {
        // Mettre à jour les bornes du sommet actuel
        abr->borneInf = min(abr->borneInf, element);
        abr->borneSup = max(abr->borneSup, element);
    } else if (element < abr->borneInf - 1) {
        // Insérer l'élément dans le sous-arbre gauche
        abr->filsGauche = insererElement(abr->filsGauche, element);
    } else if (element > abr->borneSup + 1) {
        // Insérer l'élément dans le sous-arbre droit
        abr->filsDroit = insererElement(abr->filsDroit, element);
    }
    fusionnerSommets(abr);
    return (abr);
}

// Fonction pour rechercher un élément dans un arbre
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

    // Vérifie si l'élément se trouve dans les bornes des sous-arbres gauche ou droit
    if ((abr->filsGauche && (element >= abr->filsGauche->borneInf && element <= abr->filsGauche->borneSup)) /* verifie si le noeud a un fils gauche et si l'element est dans l'intervalle de valeur represente par le fils gauche*/||
        (abr->filsDroit && (element >= abr->filsDroit->borneInf && element <= abr->filsDroit->borneSup))) {
        return abr;
    }

    // Si l'élément est inférieur à la borneInf du sommet actuel, recherche dans le sous-arbre gauche
    if (element < abr->borneInf) {
        return rechercherPere(abr->filsGauche, element);
    }
    // Si l'élément est supérieur à la borneSup du sommet actuel, recherche dans le sous-arbre droit
    else if (element > abr->borneSup) {
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
        return -1;
    }

    int hauteurGauche = rechercherHauteur(abr->filsGauche);
    int hauteurDroit = rechercherHauteur(abr->filsDroit);

    return 1 + max(hauteurGauche, hauteurDroit); // Ajoutez 1 pour inclure le nœud actuel
}


// Fonction pour afficher les sommets d'un arbre
void afficherSommets(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }
    afficherSommets(abr->filsGauche);
    append_to_message_view(g_strdup_printf("[%d; %d]\n", abr->borneInf, abr->borneSup));// creer une nouvelle chaie de caractere pour afficher les bornes de l'intervalle
    afficherSommets(abr->filsDroit);
}

// Fonction pour afficher les éléments d'un arbre
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

// Fonction pour supprimer un élément de l'arbre
T_Arbre supprimerElement(T_Arbre abr, int element) {
    if (abr == NULL) {
        return NULL;
    }
    // Trouver l'élément à supprimer
    T_Sommet *elementASupprimer = rechercherElement(abr, element);
    if (elementASupprimer == NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: L'élément %d n'existe pas dans l'arbre. Suppression impossible.\n", element));
        return abr;
    }
    // Si l'élément est inférieur au sommet actuel, aller dans le sous-arbre gauche
    if (element < abr->borneInf) {
        abr->filsGauche = supprimerElement(abr->filsGauche, element);
    }
    // Si l'élément est supérieur au sommet actuel, aller dans le sous-arbre droit
    else if (element > abr->borneSup) {
        abr->filsDroit = supprimerElement(abr->filsDroit, element);
    }
    // Si l'élément est trouvé
    else {
        // Si le sommet actuel est une feuille
        if (abr->borneInf == abr->borneSup) {
            if (abr->borneInf == element) {
                T_Sommet *nouvelleRacine = NULL;
                // Si le sous-arbre gauche est vide, définir le sous-arbre droit comme nouvelle racine
                if (abr->filsGauche == NULL) {
                    nouvelleRacine = abr->filsDroit;
                }
                // Si le sous-arbre droit est vide, définir le sous-arbre gauche comme nouvelle racine
                else if (abr->filsDroit == NULL) {
                    nouvelleRacine = abr->filsGauche;
                }
                // Si les deux sous-arbres ne sont pas vides, trouver l'élément minimum dans le sous-arbre droit
                else {
                    T_Sommet *filsCourant = abr->filsDroit;
                    while (filsCourant->filsGauche != NULL) {
                        filsCourant = filsCourant->filsGauche;
                    }
                    // Définir le sous-arbre gauche de l'élément minimum comme sous-arbre gauche du sommet actuel
                    filsCourant->filsGauche = abr->filsGauche;
                    nouvelleRacine = abr->filsDroit;
                }
                free(abr);
                append_to_message_view(g_strdup_printf("\n"));
                append_to_message_view(g_strdup_printf("Suppression de %d.\n", element)); // Afficher un message indiquant la suppression réussie
                fusionnerSommets(nouvelleRacine);
                return (nouvelleRacine);
            }
        }
        // Si le sommet actuel n'est pas une feuille
        else {
            if (element == abr->borneInf) {
                abr->borneInf++;
            }
            else if (element == abr->borneSup) {
                abr->borneSup--;
            }
            else {
                // Créer un nouveau sommet pour [borneInf, element - 1]
                T_Sommet *nouveauGauche = creerSommet(abr->borneInf);
                nouveauGauche->borneSup = element - 1;
                nouveauGauche->filsGauche = abr->filsGauche;

                // Créer un nouveau sommet pour [element + 1, borneSup]
                T_Sommet *nouveauDroit = creerSommet(element + 1);
                nouveauDroit->borneSup = abr->borneSup;
                nouveauDroit->filsDroit = abr->filsDroit;

                int x;
                // Insérer les éléments du sous-arbre gauche dans le nouveau sommet gauche
                for (x = nouveauDroit->borneInf; x <= nouveauDroit->borneSup; x++) {
                    nouveauGauche = insererElement(nouveauGauche, x);
                }
                // Connecter le sous-arbre gauche du sommet actuel au nouveau sommet gauche
                if (abr->filsGauche != NULL) {
                    nouveauGauche->filsGauche = abr->filsGauche;
                }
                // Connecter le sous-arbre droit du sommet actuel au nouveau sommet droit
                if (abr->filsGauche != NULL) {
                    nouveauDroit->filsDroit = abr->filsDroit;
                }
                // Insérer les éléments du sous-arbre gauche du sommet actuel dans le nouveau sommet gauche
                if (abr->filsGauche != NULL) {
                    for (x = abr->filsGauche->borneInf; x <= abr->filsGauche->borneSup; x++) {
                        nouveauGauche = insererElement(nouveauGauche, x);
                    }
                }
                // Insérer les éléments du sous-arbre droit du sommet actuel dans le nouveau sommet gauche
                if (abr->filsDroit != NULL) {
                    for (x = abr->filsDroit->borneInf; x <= abr->filsDroit->borneSup; x++) {
                        nouveauGauche = insererElement(nouveauGauche, x);
                    }
                }
                 // Vérifier si abr n'est pas nul avant de le libérer
                if (abr != NULL) {
                    free(abr);
                }
                // Retourner le nouveau sous-arbre avec les deux nouveaux sommets
                fusionnerSommets(nouveauGauche);
                return (nouveauGauche);
            }
        }
    }
    fusionnerSommets(abr);
    return (abr);
}


// Fonction pour calculer la taille mémoire de l'arbre par intervalles
unsigned int tailleMemoireIntervalles(T_Arbre abr) {
    if (abr == NULL) {
        return 0;
    }
    unsigned int tailleArbre = sizeof(T_Sommet);
    return tailleArbre + tailleMemoireIntervalles(abr->filsGauche) + tailleMemoireIntervalles(abr->filsDroit);
}

// Fonction pour calculer la taille mémoire de l'arbre de manière classique
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

void fusionnerSommets(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }

    // Traiter le sommet actuel comme une racine
    ItererFusion(abr, abr);

    // Récursion sur les sous-arbres gauche et droit
    fusionnerSommets(abr->filsGauche);
    fusionnerSommets(abr->filsDroit);
}

void ItererFusion(T_Arbre racine, T_Arbre abr) {
    if (abr == NULL) {
        return;
    }

    // Fusion si le sommet en cours peut fusionner avec la racine
    if (abr != racine && ((abr->borneInf <= racine->borneSup + 1 && abr->borneSup >= racine->borneInf - 1) ||
        (racine->borneInf <= abr->borneSup + 1 && racine->borneSup >= abr->borneInf - 1))) {

            int abrborneinf = abr->borneInf;
            int abrbornesup = abr->borneSup;

            // Supprimer tous les éléments du sommet à fusionner
            for (int i = abr->borneInf; i <= abr->borneSup; i++) {
                racine = supprimerElement(racine, i);
            }

            int min_borne = min(racine->borneInf, abrborneinf);
            int max_borne = max(racine->borneSup, abrbornesup);

            racine->borneInf = min_borne;
            racine->borneSup = max_borne;
        }

    // Récursion sur les sous-arbres gauche et droit
    ItererFusion(racine, abr->filsGauche);
    ItererFusion(racine, abr->filsDroit);
}

