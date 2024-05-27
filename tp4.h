#ifndef TP4_H
#define TP4_H

typedef struct T_Sommet T_Sommet;
typedef struct T_Sommet2 T_Sommet2; // Simule un sommet classique (caractérisé par un entier unique plutôt que des bornes inf et sup)
typedef T_Sommet* T_Arbre;

// Désactivation du remplissage automatique pour T_Sommet
#pragma pack(1)
struct T_Sommet {
    int borneInf;
    int borneSup;
    T_Arbre filsGauche;
    T_Arbre filsDroit;
};

// Désactivation du remplissage automatique pour T_Sommet2
struct T_Sommet2 {
    int element;
    T_Arbre filsGauche;
    T_Arbre filsDroit;
};
#pragma pack()

T_Arbre creerSommet(int element);
T_Arbre insererElement(T_Arbre abr, int element);
T_Sommet *rechercherElement(T_Arbre abr, int element);
T_Sommet* rechercherPere(T_Arbre abr, int element);
int niveauDuSommet(T_Arbre racine, T_Sommet *sommet);
int rechercherHauteur(T_Arbre abr);
void afficherSommets(T_Arbre abr);
void afficherElements(T_Arbre abr);
T_Arbre supprimerElement(T_Arbre abr, int element);
unsigned int tailleMemoireIntervalles(T_Arbre abr);
unsigned int tailleMemoireClassique(T_Arbre abr);
void tailleMemoire(T_Arbre abr);
T_Arbre fusionnerSommets(T_Arbre abr);
T_Arbre rotationGauche(T_Arbre abr);
T_Arbre rotationDroite(T_Arbre abr);
T_Arbre equilibrerArbre(T_Arbre abr);

#endif /* TP4_H */
