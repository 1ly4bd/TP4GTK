#include <gtk/gtk.h>
#include <cairo.h>
#include <gtk/gtkcssprovider.h>
#include "tp4.h"
#include "message_utils.h"
#include "killprocess.h"
#ifdef _WIN32
#include <windows.h>
#endif


// Déclaration des variables globales pour l'arbre, les états précédents et suivants, la zone de texte, les nœuds graphiques et la zone de dessin
T_Arbre abr = NULL;
GArray *previous_abrs = NULL;
GArray *next_abrs = NULL;
GtkWidget *text_view = NULL;
GArray *noeudsGraphiques = NULL;
static GtkWidget *darea = NULL;

// Déclaration des variables globales pour le zoom et le panoramique
static double zoom_level = 1.0;
static double offset_x = 0;
static double offset_y = 0;
static double last_x = 0;
static double last_y = 0;
static gboolean dragging = FALSE;
static GtkWidget *scrolled_window = NULL;
static GtkWidget *zoom_level_label = NULL;
static GtkWidget *reset_zoom_button = NULL;

// Déclaration des widgets de la fenêtre principale
GtkWidget *window;
GtkWidget *vbox;
GtkWidget *hbox;
GtkWidget *button;
GtkWidget *entry;
GtkCssProvider *provider;
GdkDisplay *display;
GdkScreen *screen;

// Structure pour représenter un nœud graphique
typedef struct {
    double x;
    double y;
    T_Sommet *sommet;
} NoeudGraphique;

// Fonction pour copier un arbre
T_Arbre copierArbre(T_Arbre abr) {
    if (abr == NULL) {
        return NULL;
    }

    // Créer un nouveau sommet pour l'arbre copié
    T_Arbre copie = (T_Arbre)malloc(sizeof(struct T_Sommet));
    if (copie == NULL) {
        append_to_message_view(g_strdup_printf("Erreur lors de l'allocation de mémoire pour la copie de l'arbre.\n"));
        exit(EXIT_FAILURE);
    }

    // Copier les données du sommet
    copie->borneInf = abr->borneInf;
    copie->borneSup = abr->borneSup;

    // Copier les sous-arbres
    copie->filsGauche = copierArbre(abr->filsGauche);
    copie->filsDroit = copierArbre(abr->filsDroit);

    return copie;
}

// Fonction pour supprimer un arbre
void supprimerArbre(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }

    supprimerArbre(abr->filsGauche);
    supprimerArbre(abr->filsDroit);
    free(abr);
}

// Fonction pour sauvegarder l'état précédent de l'arbre
void sauvegarder_etat_precedent() {
    // Créer un tableau dynamique s'il n'existe pas encore
    if (previous_abrs == NULL) {
        previous_abrs = g_array_new(FALSE, FALSE, sizeof(T_Arbre));
    }
    // Faire une copie de l'arbre actuel et l'ajouter au tableau
    T_Arbre copie = copierArbre(abr);
    g_array_append_val(previous_abrs, copie);
}

// Fonction pour sauvegarder l'état suivant de l'arbre
void sauvegarder_etat_suivant() {
    // Créer un tableau dynamique s'il n'existe pas encore
    if (next_abrs == NULL) {
        next_abrs = g_array_new(FALSE, FALSE, sizeof(T_Arbre));
    }
    // Faire une copie de l'arbre actuel et l'ajouter au tableau
    T_Arbre copie = copierArbre(abr);
    g_array_append_val(next_abrs, copie);
}

// Fonction pour dessiner l'arbre
static void dessiner_arbre(cairo_t *cr, T_Arbre abr, double x, double y, double x_offset, double y_offset) {
    // Vérifier si l'arbre est vide
    if (abr == NULL) {
        // Définir le texte et sa taille
        char *message = "Arbre vide";
        double font_size = 16;

        // Centrer le texte
        cairo_set_font_size(cr, font_size);
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_text_extents_t te;
        cairo_text_extents(cr, message, &te);
        double text_center_x = x - te.width / 2.0;
        double text_center_y = y + 125;

        // Afficher le message
        cairo_set_source_rgba(cr, 1, 1, 1, 0.25); // Couleur du texte
        cairo_move_to(cr, text_center_x, text_center_y);
        cairo_show_text(cr, message);
        return;
    }

    // Enregistrer les coordonnées et le sommet associé
    NoeudGraphique noeud = { x, y, abr };
    g_array_append_val(noeudsGraphiques, noeud);

    // Dessiner un cercle
    cairo_arc(cr, x, y, 20 * zoom_level, 0, 2 * G_PI);

    // Définir la couleur de remplissage
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1);
    cairo_fill_preserve(cr); // Remplir le cercle tout en préservant le chemin pour la bordure

    // Couleur de la bordure du cercle
    cairo_set_source_rgba(cr, 0.45, 0.45, 0.45, 1);
    cairo_set_line_width(cr, 3);
    cairo_stroke(cr);

    // Préparer le texte à afficher dans les cercles
    char intervalle[32];
    sprintf(intervalle, "%d - %d", abr->borneInf, abr->borneSup);

    // Sélectionner la police
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    // Définir la taille de la police
    cairo_set_font_size(cr, 10.5 * zoom_level);
    cairo_text_extents_t te;
    cairo_text_extents(cr, intervalle, &te);
    double text_width = te.width;
    double text_height = te.height;
    double text_center_x = x - text_width / 2.0;
    double text_center_y = y + text_height / 2.0;

    // Afficher le texte
    cairo_move_to(cr, text_center_x, text_center_y);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.85); // Couleur du texte
    cairo_show_text(cr, intervalle);

    // Dessiner les lignes vers les fils
    // Dessiner la ligne vers le fils gauche
    if (abr->filsGauche != NULL) {
        cairo_set_source_rgba(cr, 0.45, 0.45, 0.45, 1); // Couleur de la ligne
        cairo_set_line_width(cr, 3);
        cairo_move_to(cr, x, y + 20 * zoom_level);
        cairo_line_to(cr, x - x_offset * zoom_level, y + y_offset * zoom_level - 20 * zoom_level);
        cairo_stroke(cr);
        // Appeler récursivement la fonction pour dessiner l'arbre du fils gauche
        dessiner_arbre(cr, abr->filsGauche, x - x_offset * zoom_level, y + y_offset * zoom_level, x_offset / 1.5, y_offset);
    }

    // Dessiner la ligne vers le fils droit
    if (abr->filsDroit != NULL) {
        cairo_set_source_rgba(cr, 0.45, 0.45, 0.45, 1); // Couleur de la ligne
        cairo_set_line_width(cr, 3);
        cairo_move_to(cr, x, y + 20 * zoom_level);
        cairo_line_to(cr, x + x_offset * zoom_level, y + y_offset * zoom_level - 20 * zoom_level);
        cairo_stroke(cr);
        // Appeler récursivement la fonction pour dessiner l'arbre du fils droit
        dessiner_arbre(cr, abr->filsDroit, x + x_offset * zoom_level, y + y_offset * zoom_level, x_offset / 1.5, y_offset);
    }
}

// Fonction de gestion de l'événement de dessin
// Cette fonction est appelée lorsqu'il est nécessaire de dessiner le contenu de la zone de dessin
static void on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer data) {
    // Créer un tableau pour stocker les nœuds graphiques
    noeudsGraphiques = g_array_new(FALSE, FALSE, sizeof(NoeudGraphique));

    // Obtenir les dimensions de la zone de dessin
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    int width = allocation.width;
    int height = allocation.height;

    // Calculer les coordonnées de départ pour dessiner l'arbre
    double start_x = width / 2.0 + offset_x;
    double start_y = height / 4.0 + offset_y;

    // Dessiner l'arbre en utilisant la fonction dessiner_arbre
    dessiner_arbre(cr, abr, start_x, start_y, (width / 2.0) / 2.0, (height / 4.0) / 2.0);

    // Libérer le tableau des nœuds graphiques
    g_array_free(noeudsGraphiques, TRUE);
}

// Fonction de gestion de l'événement de pression sur le bouton de la souris
// Cette fonction est appelée lorsqu'un bouton de la souris est pressé
static void on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    // Vérifier si l'arbre est vide
    if (abr == NULL) {
        return;
    }
    // Vérifier si le bouton pressé est le bouton gauche de la souris
    if (event->button == 1) {
        // Enregistrer les coordonnées du dernier clic
        last_x = event->x;
        last_y = event->y;
        dragging = TRUE; // Décalage activé
    }
}

// Fonction de gestion de l'événement de relâchement du bouton de la souris
// Cette fonction est appelée lorsqu'un bouton de la souris est relâché
static gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    // Vérifier si le bouton relâché est le bouton gauche de la souris
    if (event->button == 1) {
        dragging = FALSE; // Décalage désactivé
    }
    // Vérifier si l'arbre est vide
    if (abr == NULL) {
        return FALSE;
    }
    return TRUE;
}

// Fonction de gestion de l'événement de mouvement de la souris
// Cette fonction est appelée lorsqu'il y a un mouvement de la souris
static gboolean on_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    // Vérifier si le glissement est en cours
    if (dragging) {
        // Mettre à jour les coordonnées de décalage en fonction du mouvement de la souris
        offset_x += event->x - last_x;
        offset_y += event->y - last_y;
        last_x = event->x;
        last_y = event->y;
        // Mettre à jour l'affichage de la zone de dessin
        gtk_widget_queue_draw(widget);
    }
    // Vérifier si l'arbre est vide
    if (abr == NULL) {
        return FALSE;
    }
    return TRUE;
}

// Fonction pour mettre à jour le libellé du niveau de zoom
static void update_zoom_level_label() {
    if (zoom_level_label != NULL) {
        // Calculer le pourcentage de zoom
        int zoom_percentage = (int)(zoom_level * 100);
        // Créer le texte du libellé
        gchar *zoom_text = g_strdup_printf("Zoom : %d%%", zoom_percentage);
        // Mettre à jour le libellé du niveau de zoom
        gtk_label_set_text(GTK_LABEL(zoom_level_label), zoom_text);
        g_free(zoom_text);
    }
}

// Fonction pour réinitialiser le niveau de zoom
static void reset_zoom_level(GtkWidget *widget, gpointer data) {
    zoom_level = 1.0;
    // Mettre à jour le libellé du niveau de zoom
    update_zoom_level_label();
    // Mettre à jour l'affichage de la zone de dessin
    gtk_widget_queue_draw(darea);
}

// Fonction de gestion de l'événement de défilement de la souris
// Cette fonction est appelée lorsqu'il y a un défilement de la souris
static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data) {
    // Vérifier la direction du défilement
    if (event->direction == GDK_SCROLL_UP) {
        // Zoom avant
        zoom_level *= 1.1;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        // Zoom arrière
        zoom_level /= 1.1;
    }
    // Mettre à jour l'affichage de la zone de dessin
    gtk_widget_queue_draw(widget);
    // Vérifier si l'arbre est vide
    if (abr == NULL) {
        return FALSE;
    }
    // Mettre à jour le libellé du niveau de zoom
    update_zoom_level_label();
    return TRUE;
}

// Fonction pour insérer un élément dans l'arbre
static void inserer_un_element(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (strlen(text) == 0) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier.\n"));
        gtk_entry_set_text(GTK_ENTRY(entry), "");
        return;
    }
    char *endptr;
    long element = strtol(text, &endptr, 10);
    if (*endptr != '\0') {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier valide.\n"));
        gtk_entry_set_text(GTK_ENTRY(entry), "");
        return;
    }
    if (rechercherElement(abr, element) != NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: L'element %ld est deja present dans l'arbre.\n", element));
        gtk_entry_set_text(GTK_ENTRY(entry), "");
        return;
    }
    // Sauvegarder l'état actuel de l'arbre
    sauvegarder_etat_precedent();

    abr = insererElement(abr, (int)element);
    append_to_message_view(g_strdup_printf("\n"));
    append_to_message_view(g_strdup_printf("Element %ld insere.\n", element));
    gtk_entry_set_text(GTK_ENTRY(entry), "");
    gtk_widget_queue_draw(darea);
}

// Fonction pour supprimer un élément de l'arbre
static void supprimer_un_element(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (strlen(text) == 0) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier.\n"));
        gtk_entry_set_text(GTK_ENTRY(entry), "");
        return;
    }
    char *endptr;
    long element = strtol(text, &endptr, 10);
    if (*endptr != '\0') {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier valide.\n"));
        gtk_entry_set_text(GTK_ENTRY(entry), "");
        return;
    }
    // Sauvegarder l'état actuel de l'arbre
    sauvegarder_etat_precedent();

    abr = supprimerElement(abr, (int)element);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
    gtk_widget_queue_draw(darea);
}

static void recentrer_arbre(GtkWidget *widget, gpointer data) {
    // Réinitialiser les valeurs de décalage pour recentrer l'arbre
    offset_x = 0;
    offset_y = 0;
    // Redessiner l'arbre avec le nouveau décalage et le niveau de zoom
    gtk_widget_queue_draw(darea);
}

// Fonction pour restaurer l'état précédent de l'arbre
static void retourner_etat_precedent(GtkWidget *widget, gpointer data) {
    if (previous_abrs != NULL && previous_abrs->len > 0) {
        // Sauvegarder l'�tat actuel dans le tableau des �tats suivants
        sauvegarder_etat_suivant();

        // Lib�rer l'arbre actuel
        supprimerArbre(abr);
        // Restaurer l'arbre � partir du dernier �tat sauvegard�
        abr = g_array_index(previous_abrs, T_Arbre, previous_abrs->len - 1);
        // Supprimer l'�tat restaur� du tableau
        g_array_remove_index(previous_abrs, previous_abrs->len - 1);
        // Redessiner l'arbre avec l'�tat pr�c�dent
        if (previous_abrs->len == 0) {
            recentrer_arbre(darea, abr);
            reset_zoom_level(darea, abr);
        }
        gtk_widget_queue_draw(darea);
    }
}

static void avancer_etat_suivant(GtkWidget *widget, gpointer data) {
    if (next_abrs != NULL && next_abrs->len > 0) {
        // Sauvegarder l'�tat actuel dans le tableau des �tats pr�c�dents
        sauvegarder_etat_precedent();

        // Lib�rer l'arbre actuel
        supprimerArbre(abr);
        // Restaurer l'arbre � partir de l'�tat suivant sauvegard�
        abr = g_array_index(next_abrs, T_Arbre, next_abrs->len - 1);
        // Supprimer l'�tat suivant du tableau
        g_array_remove_index(next_abrs, next_abrs->len - 1);
        // Redessiner l'arbre avec l'�tat suivant
        gtk_widget_queue_draw(darea);
    }
}

static gboolean on_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_z)) {
        // Ctrl + Z
        retourner_etat_precedent(widget, user_data);
        return TRUE;
    } else if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_y)) {
        // Ctrl + Y
        avancer_etat_suivant(widget, user_data);
        return TRUE;
    } else if ((event->state & GDK_MOD1_MASK) && (event->keyval == GDK_KEY_F4)) {
        // Alt + F4
        killProcess();
    }
    return FALSE;
}

// Fonction pour rechercher un élément dans l'arbre
static void rechercher_un_element(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int element = atoi(text);
    T_Sommet *resultatRecherche = rechercherElement(abr, element);
    if (resultatRecherche != NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Element trouvé dans l'intervalle [%d; %d].\n", resultatRecherche->borneInf, resultatRecherche->borneSup));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Élément non trouvé.\n"));
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

// Fonction pour afficher les sommets de l'arbre
static void afficher_sommets(GtkWidget *widget, gpointer data) {
    append_to_message_view(g_strdup_printf("\n"));
    afficherSommets(abr);
    append_to_message_view(g_strdup_printf("Affichage des sommets de l'arbre:\n"));
}

// Fonction pour afficher les éléments de l'arbre
static void afficher_elements(GtkWidget *widget, gpointer data) {
    append_to_message_view(g_strdup_printf("\n"));
    append_to_message_view(g_strdup_printf("\n"));
    afficherElements(abr);
    append_to_message_view(g_strdup_printf("Affichage des éléments de l'arbre:\n"));
}

// Fonction pour afficher la taille mémoire de l'arbre
static void afficher_taille_memoire(GtkWidget *widget, gpointer data) {
    append_to_message_view(g_strdup_printf("\n"));
    tailleMemoire(abr);
}

// Fonction pour afficher la racine de l'arbre
static void afficher_racine(GtkWidget *widget, gpointer data) {
    if (abr != NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Racine de l'arbre: [%d; %d]\n", abr->borneInf, abr->borneSup));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: L'arbre est vide, pas de racine à afficher.\n"));
    }
}

// Fonction pour afficher le père d'un élément
static void afficher_pere(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (strlen(text) == 0) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier.\n"));
        return;
    }
    char *endptr;
    long element = strtol(text, &endptr, 10);
    if (*endptr != '\0') {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier valide.\n"));
        return;
    }
    T_Sommet *pere = rechercherPere(abr, (int)element);
    if (pere != NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Père de l'élément %ld : [%d; %d].\n", element, pere->borneInf, pere->borneSup));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Élément non trouvé ou racine de l'arbre.\n"));
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

// Fonction pour afficher le niveau d'un élément
static void afficher_niveau(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (strlen(text) == 0) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier.\n"));
        return;
    }
    char *endptr;
    long element = strtol(text, &endptr, 10);
    if (*endptr != '\0') {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur : Veuillez entrer un entier valide.\n"));
        return;
    }
    int niveau = niveauDuSommet(abr, rechercherElement(abr, (int)element));
    if (niveau >= 0) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Niveau de l'élément %ld : %d.\n", element, niveau));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Élément non trouvé.\n"));
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

// Fonction pour afficher la hauteur de l'arbre
static void afficher_hauteur(GtkWidget *widget, gpointer entry) {
    int hauteur = rechercherHauteur(abr);
    if (hauteur >= 0) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Hauteur de l'arbre : %d.\n", hauteur));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Arbre vide.\n"));
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

// Fonction pour effacer le contenu de la vue de message
static void clear_message_view() {
    GtkTextBuffer *buffer;
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1);
}

static void reinitialiser_arbre(GtkWidget *widget, gpointer data) {
    // Effacer le contenu de la zone de texte
    clear_message_view();

    // R�initialiser l'arbre
    supprimerArbre(abr);
    abr = NULL;
    zoom_level = 1.0;
    recentrer_arbre(darea, abr);
    reset_zoom_level(darea, abr);
    gtk_widget_queue_draw(darea);
    previous_abrs = NULL;
    next_abrs = NULL;
    append_to_message_view(g_strdup_printf("Arbre reinitialise.\n"));
}

// D�claration de la fen�tre pour l'�cran de d�marrage
GtkWidget *splash_window = NULL;
GtkWidget *spinner;

// Fonction pour masquer ou fermer l'�cran de d�marrage
static void hide_splash_screen() {
    if (splash_window != NULL) {
        gtk_spinner_stop(GTK_SPINNER(spinner));
        gtk_widget_destroy(splash_window);
        splash_window = NULL;
    }
}

// Fonction de rappel pour afficher la fen�tre principale apr�s un d�lai
static gboolean show_main_window(gpointer user_data) {
    // Masquer l'�cran de d�marrage
    hide_splash_screen();

    // Affichage de la fen�tre principale
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_widget_show_all(window);

    return G_SOURCE_REMOVE; // Supprimer la source de l'�v�nement
}

// Fonction pour cr�er et afficher l'�cran de d�marrage
static void show_splash_screen() {
    // Cr�ation de la fen�tre
    splash_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(splash_window), FALSE); // Supprimer la d�coration de la fen�tre
    gtk_window_set_position(GTK_WINDOW(splash_window), GTK_WIN_POS_CENTER); // Centrer la fen�tre
    gtk_window_set_default_size(GTK_WINDOW(splash_window), 1000, 500);
    gtk_widget_set_name(splash_window, "splash_window");

    // Cr�ation de la bo�te verticale pour le contenu de l'�cran de d�marrage
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(splash_window), vbox);

    // Espacement pour centrer la hbox verticalement
    GtkWidget *vbox_spacing_top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(vbox_spacing_top, -1, 150); // Ajustez la hauteur pour centrer verticalement
    gtk_box_pack_start(GTK_BOX(vbox), vbox_spacing_top, FALSE, FALSE, 0);

    // Cr�ation de la bo�te horizontale pour l'ic�ne et le label
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); // Espacement de 10 pixels entre les �l�ments
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    // Chargement de l'image PNG et redimensionnement
    GtkWidget *image = gtk_image_new_from_file("icone.png");
    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
    GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, gdk_pixbuf_get_width(pixbuf) / 2, gdk_pixbuf_get_height(pixbuf) / 2, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), scaled_pixbuf);
    gtk_widget_set_name(image, "splash_image");

    // Cr�ation d'une bo�te de remplissage pour centrer l'image et le label horizontalement
    GtkWidget *hbox_filler_left = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hbox_filler_left, TRUE, TRUE, 0);

    // Ajout de l'image � la bo�te horizontale
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

    // Ajout du label "Chargement en cours" et agrandissement de la taille de la police
    GtkWidget *label = gtk_label_new("Chargement en cours");
    PangoFontDescription *font_desc = pango_font_description_from_string("Segoe UI 24"); // Agrandir la taille de la police
    gtk_widget_override_font(label, font_desc);
    pango_font_description_free(font_desc);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_set_name(label, "splash_label");

    // Création du widget spinner
    spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(hbox), spinner, TRUE, TRUE, 0);

    // Démarrage de l'animation du spinner
    gtk_spinner_start(GTK_SPINNER(spinner));

    // Cr�ation d'une bo�te de remplissage pour centrer l'image et le label horizontalement
    GtkWidget *hbox_filler_right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hbox_filler_right, TRUE, TRUE, 0);

    // Espacement pour centrer la hbox verticalement
    GtkWidget *vbox_spacing_bottom = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(vbox_spacing_bottom, -1, 150); // Ajustez la hauteur pour centrer verticalement
    gtk_box_pack_start(GTK_BOX(vbox), vbox_spacing_bottom, FALSE, FALSE, 0);

    // Affichage de la fen�tre
    gtk_widget_show_all(splash_window);

    // Temporisation pour fermer l'�cran de d�marrage apr�s quelques secondes (par exemple, 2 secondes)
    g_timeout_add(2000, show_main_window, NULL);
}

void cleanup() {
    // Lib�rer la m�moire allou�e dynamiquement
    if (previous_abrs != NULL) {
        g_array_free(previous_abrs, TRUE);
    }
    if (next_abrs != NULL) {
        g_array_free(next_abrs, TRUE);
    }
    if (abr != NULL) {
        supprimerArbre(abr);
    }
     if (provider != NULL) {
        g_object_unref(provider);
        provider = NULL;
    }
    if (splash_window != NULL) {
        gtk_widget_destroy(splash_window);
        splash_window = NULL;
    }
    if (text_view != NULL) {
        g_object_unref(text_view);
        text_view = NULL;
    }
    if (noeudsGraphiques != NULL) {
        g_array_free(noeudsGraphiques, TRUE);
        noeudsGraphiques = NULL;
    }
    if (scrolled_window != NULL) {
        g_object_unref(scrolled_window);
        scrolled_window = NULL;
    }
    if (zoom_level_label != NULL) {
        g_object_unref(zoom_level_label);
        zoom_level_label = NULL;
    }
    if (reset_zoom_button != NULL) {
        g_object_unref(reset_zoom_button);
        reset_zoom_button = NULL;
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    // Initialisation de la fen�tre principale
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gestion d'Arbre Binaire");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);

    // Cr�ation de la barre de titre personnalis�e
    GtkWidget *titlebar = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(titlebar), "Gestion d'Arbre Binaire");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(titlebar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), titlebar);

    // Cr�ation d'une bo�te verticale pour contenir les �l�ments
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Cr�ation de la bo�te horizontale pour les boutons
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    // Cr�ation du champ de saisie
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 5);

    // Cr�ation et configuration des boutons
    button = gtk_button_new_with_label("Inserer Element");
    g_signal_connect(button, "clicked", G_CALLBACK(inserer_un_element), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Supprimer Element");
    g_signal_connect(button, "clicked", G_CALLBACK(supprimer_un_element), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Rechercher Element");
    g_signal_connect(button, "clicked", G_CALLBACK(rechercher_un_element), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    // Cr�ation d'une nouvelle bo�te horizontale pour les boutons suivants
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Afficher Elements");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_elements), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Sommets");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_sommets), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Pere");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_pere), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Niveau");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_niveau), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Racine");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_racine), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Hauteur");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_hauteur), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Taille Memoire");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_taille_memoire), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    // Cr�ation de la zone de dessin
    darea = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), darea, TRUE, TRUE, 5);
    g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), NULL);
    g_signal_connect(G_OBJECT(darea), "scroll-event", G_CALLBACK(on_scroll_event), NULL);
    g_signal_connect(G_OBJECT(darea), "button-press-event", G_CALLBACK(on_button_press_event), NULL);
    g_signal_connect(G_OBJECT(darea), "button-release-event", G_CALLBACK(on_button_release_event), NULL);
    g_signal_connect(G_OBJECT(darea), "motion-notify-event", G_CALLBACK(on_motion_notify_event), NULL);
    // Ajouter le gestionnaire d'�v�nements pour la molette de la souris
    // Ajouter le gestionnaire d'�v�nements pour le double clic
    gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);

    // Cr�er une nouvelle bo�te horizontale
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    gtk_widget_set_visible(hbox, FALSE); // Rendre la bo�te horizontale invisible (marche pas)

    // Cr�ation des boutons de recentrage, retour, avancement et zoom
    button = gtk_button_new_with_label("Reinitialiser Arbre");
    g_signal_connect(button, "clicked", G_CALLBACK(reinitialiser_arbre), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Avancer");
    g_signal_connect(button, "clicked", G_CALLBACK(avancer_etat_suivant), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Retour");
    g_signal_connect(button, "clicked", G_CALLBACK(retourner_etat_precedent), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Recentrer");
    g_signal_connect(button, "clicked", G_CALLBACK(recentrer_arbre), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    reset_zoom_button = gtk_button_new_with_label("Reset zoom");
    g_signal_connect(reset_zoom_button, "clicked", G_CALLBACK(reset_zoom_level), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), reset_zoom_button, FALSE, FALSE, 5);


    zoom_level_label = gtk_label_new("Zoom : 100%");
    gtk_widget_set_opacity(GTK_WIDGET(zoom_level_label), 0.5); // R�gler l'opacit� � 50%
    gtk_box_pack_end(GTK_BOX(hbox), zoom_level_label, FALSE, FALSE, 5);

    // Cr�ation du cadre pour encadrer la zone de texte et la zone de dessin
    GtkWidget *frame = gtk_frame_new("Messages");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5); // Centre le titre

    // Cr�ation de la zone de texte pour les messages
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, -1, 100); // Taille minimale pour �tre visible
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // Rendre le fond de la zone de texte transparent
    GdkRGBA transparent;
    transparent.red = 0.0;
    transparent.green = 0.0;
    transparent.blue = 0.0;
    transparent.alpha = 0.0;
    gtk_widget_override_background_color(text_view, GTK_STATE_FLAG_NORMAL, &transparent);

    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press_event), NULL);
    #ifdef _WIN32
    g_signal_connect(window, "destroy", G_CALLBACK(killProcess), NULL);
    #endif

    // Charger le fichier CSS
    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);

    gtk_css_provider_load_from_path(provider, "gtkgui.css", NULL);

    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider); // Libérer le CSS provider après utilisation

    // Afficher l'�cran de d�marrage qui va ensuite afficher la fenetre principale
    show_splash_screen();
}

int main(int argc, char **argv) {
    // Masquer la fen�tre de console sur Windows
    #ifdef _WIN32
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow != NULL) {
        ShowWindow(consoleWindow, SW_SHOWMINIMIZED); // Minimiser la fen�tre de la console
    }
    #endif

    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    // Appel de la fonction de nettoyage avant de quitter le programme
    cleanup();

    return status;
}
