#include <gtk/gtk.h>
#include <cairo.h>
#include <gtk/gtkcssprovider.h>
#include "tp4.h"
#include "message_utils.h"
#include "killprocess.h"
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
    // Masquer la fenêtre de console sur Mac
    #include <CoreFoundation/CoreFoundation.h>
    #include <Carbon/Carbon.h>

    void hideConsoleWindow() {
        ProcessSerialNumber psn = {0, kCurrentProcess};
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        SetFrontProcess(&psn);
    }
#endif


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


typedef struct {
    double x;
    double y;
    T_Sommet *sommet;
} NoeudGraphique;

T_Arbre copierArbre(T_Arbre abr) {
    if (abr == NULL) {
        return NULL;
    }

    // Créez un nouveau sommet pour l'arbre copié
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

void supprimerArbre(T_Arbre abr) {
    if (abr == NULL) {
        return;
    }

    supprimerArbre(abr->filsGauche);
    supprimerArbre(abr->filsDroit);
    free(abr);
}

// Fonction pour sauvegarder l'état actuel de l'arbre
void sauvegarder_etat_precedent() {
    // Créer un tableau dynamique s'il n'existe pas encore
    if (previous_abrs == NULL) {
        previous_abrs = g_array_new(FALSE, FALSE, sizeof(T_Arbre));
    }
    // Faire une copie de l'arbre actuel et ajoutez-la au tableau
    T_Arbre copie = copierArbre(abr);
    g_array_append_val(previous_abrs, copie);
}

void sauvegarder_etat_suivant() {
    // Créer un tableau dynamique s'il n'existe pas encore
    if (next_abrs == NULL) {
        next_abrs = g_array_new(FALSE, FALSE, sizeof(T_Arbre));
    }
    // Faire une copie de l'arbre actuel et ajoutez-la au tableau
    T_Arbre copie = copierArbre(abr);
    g_array_append_val(next_abrs, copie);
}

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

    // Dessiner un cercle avec un remplissage et une bordure
    cairo_arc(cr, x, y, 20 * zoom_level, 0, 2 * G_PI);

    cairo_set_source_rgba(cr, 0.45, 0.45, 0.45, 1); // Couleur de la bordure du cercle
    cairo_set_line_width(cr, 3);
    cairo_stroke(cr);

    // Préparer le texte à afficher
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
    if (abr->filsGauche != NULL) {
        cairo_set_source_rgba(cr, 0.45, 0.45, 0.45, 1); // Couleur de la ligne
        cairo_set_line_width(cr, 3);
        cairo_move_to(cr, x, y + 20 * zoom_level);
        cairo_line_to(cr, x - x_offset * zoom_level, y + y_offset * zoom_level - 20 * zoom_level);
        cairo_stroke(cr);
        dessiner_arbre(cr, abr->filsGauche, x - x_offset * zoom_level, y + y_offset * zoom_level, x_offset / 1.5, y_offset);
    }

    if (abr->filsDroit != NULL) {
        cairo_set_source_rgba(cr, 0.45, 0.45, 0.45, 1); // Couleur de la ligne
        cairo_set_line_width(cr, 3);
        cairo_move_to(cr, x, y + 20 * zoom_level);
        cairo_line_to(cr, x + x_offset * zoom_level, y + y_offset * zoom_level - 20 * zoom_level);
        cairo_stroke(cr);
        dessiner_arbre(cr, abr->filsDroit, x + x_offset * zoom_level, y + y_offset * zoom_level, x_offset / 1.5, y_offset);
    }
}

static void on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer data) {
    noeudsGraphiques = g_array_new(FALSE, FALSE, sizeof(NoeudGraphique));

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    int width = allocation.width;
    int height = allocation.height;

    double start_x = width / 2.0 + offset_x;
    double start_y = height / 4.0 + offset_y;

    dessiner_arbre(cr, abr, start_x, start_y, (width / 2.0) / 2.0, (height / 4.0) / 2.0);

    g_array_free(noeudsGraphiques, TRUE);
}

static void on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (abr == NULL) {
        return;
    }
    if (event->button == 1) {
        last_x = event->x;
        last_y = event->y;
        dragging = TRUE;
    }
}

static gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1) {
        dragging = FALSE;
    }
    if (abr == NULL) {
        return FALSE;
    }
    return TRUE;
}

static gboolean on_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    if (dragging) {
        offset_x += event->x - last_x;
        offset_y += event->y - last_y;
        last_x = event->x;
        last_y = event->y;
        gtk_widget_queue_draw(widget);
    }
    if (abr == NULL) {
        return FALSE;
    }
    return TRUE;
}

static void update_zoom_level_label() {
    if (zoom_level_label != NULL) {
        gchar *zoom_text = g_strdup_printf("Niveau de zoom : %.2f", zoom_level);
        gtk_label_set_text(GTK_LABEL(zoom_level_label), zoom_text);
        g_free(zoom_text);
    }
}

static void reset_zoom_level(GtkWidget *widget, gpointer data) {
    zoom_level = 1.0;
    update_zoom_level_label();
    gtk_widget_queue_draw(darea);
}

static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data) {
    if (event->direction == GDK_SCROLL_UP) {
        // Zoom in
        zoom_level *= 1.1;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        // Zoom out
        zoom_level /= 1.1;
    }

    // Redessiner l'arbre avec le nouveau niveau de zoom et offset
    gtk_widget_queue_draw(widget);
    if (abr == NULL) {
        return FALSE;
    }
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
        // Sauvegarder l'état actuel dans le tableau des états suivants
        sauvegarder_etat_suivant();

        // Libérer l'arbre actuel
        supprimerArbre(abr);
        // Restaurer l'arbre à partir du dernier état sauvegardé
        abr = g_array_index(previous_abrs, T_Arbre, previous_abrs->len - 1);
        // Supprimer l'état restauré du tableau
        g_array_remove_index(previous_abrs, previous_abrs->len - 1);
        // Redessiner l'arbre avec l'état précédent
        if (previous_abrs->len == 0) {
            recentrer_arbre(darea, abr);
            reset_zoom_level(darea, abr);
        }
        gtk_widget_queue_draw(darea);
    }
}

static void avancer_etat_suivant(GtkWidget *widget, gpointer data) {
    if (next_abrs != NULL && next_abrs->len > 0) {
        // Sauvegarder l'état actuel dans le tableau des états précédents
        sauvegarder_etat_precedent();

        // Libérer l'arbre actuel
        supprimerArbre(abr);
        // Restaurer l'arbre à partir de l'état suivant sauvegardé
        abr = g_array_index(next_abrs, T_Arbre, next_abrs->len - 1);
        // Supprimer l'état suivant du tableau
        g_array_remove_index(next_abrs, next_abrs->len - 1);
        // Redessiner l'arbre avec l'état suivant
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
    }
    return FALSE;
}

static void rechercher_un_element(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int element = atoi(text);
    T_Sommet *resultatRecherche = rechercherElement(abr, element);
    if (resultatRecherche != NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Element trouve dans l'intervalle [%d; %d].\n", resultatRecherche->borneInf, resultatRecherche->borneSup));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Element non trouve.\n"));
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

static void afficher_sommets(GtkWidget *widget, gpointer data) {
    append_to_message_view(g_strdup_printf("\n"));
    afficherSommets(abr);
    append_to_message_view(g_strdup_printf("Affichage des sommets de l'arbre:\n"));
}

static void afficher_elements(GtkWidget *widget, gpointer data) {
    append_to_message_view(g_strdup_printf("\n"));
    append_to_message_view(g_strdup_printf("\n"));
    afficherElements(abr);
    append_to_message_view(g_strdup_printf("Affichage des elements de l'arbre:\n"));
}

static void afficher_taille_memoire(GtkWidget *widget, gpointer data) {
    append_to_message_view(g_strdup_printf("\n"));
    tailleMemoire(abr);
}

static void afficher_racine(GtkWidget *widget, gpointer data) {
    if (abr != NULL) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Racine de l'arbre: [%d; %d]\n", abr->borneInf, abr->borneSup));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: L'arbre est vide, pas de racine a afficher.\n"));
    }
}

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
        append_to_message_view(g_strdup_printf("Pere de l'element %ld : [%d; %d].\n", element, pere->borneInf, pere->borneSup));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Element non trouve ou racine de l'arbre.\n"));
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

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
        append_to_message_view(g_strdup_printf("Niveau de l'element %ld : %d.\n", element, niveau));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Element non trouve.\n"));
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

static void afficher_hauteur(GtkWidget *widget, gpointer entry) {
    int hauteur = rechercherHauteur(abr);
    if (hauteur > 0) {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Hauteur de l'arbre : %d.\n", hauteur));
    } else {
        append_to_message_view(g_strdup_printf("\n"));
        append_to_message_view(g_strdup_printf("Erreur: Arbre vide.\n"));
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

static void clear_message_view() {
    GtkTextBuffer *buffer;
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1);
}

static void reinitialiser_arbre(GtkWidget *widget, gpointer data) {
    // Effacer le contenu de la zone de texte
    clear_message_view();

    // Réinitialiser l'arbre
    supprimerArbre(abr);
    abr = NULL;
    zoom_level = 1.0;
    recentrer_arbre(darea, abr);
    gtk_widget_queue_draw(darea);
    append_to_message_view(g_strdup_printf("Arbre reinitialise.\n"));
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *entry;

    // Initialisation de la fenêtre principale
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gestion d'Arbre Binaire");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);

    // Création de la barre de titre personnalisée
    GtkWidget *titlebar = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(titlebar), "Gestion d'Arbre Binaire");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(titlebar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), titlebar);
    #ifdef _WIN32
    g_signal_connect(window, "destroy", G_CALLBACK(killProcess), NULL);
    #endif

    // Création d'une boîte verticale pour contenir les éléments
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Création de la boîte horizontale pour les boutons
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    // Création du champ de saisie
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 5);

    // Création et configuration des boutons
    button = gtk_button_new_with_label("Inserer Element");
    g_signal_connect(button, "clicked", G_CALLBACK(inserer_un_element), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Supprimer Element");
    g_signal_connect(button, "clicked", G_CALLBACK(supprimer_un_element), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Rechercher Element");
    g_signal_connect(button, "clicked", G_CALLBACK(rechercher_un_element), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

    // Création d'une nouvelle boîte horizontale pour les boutons suivants
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Afficher Elements");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_elements), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Sommets");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_sommets), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Taille Memoire");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_taille_memoire), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Racine");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_racine), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Pere");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_pere), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Niveau");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_niveau), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Afficher Hauteur");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_hauteur), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);


    button = gtk_button_new_with_label("Reinitialiser Arbre");
    g_signal_connect(button, "clicked", G_CALLBACK(reinitialiser_arbre), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);

    // Création de la zone de dessin
    darea = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), darea, TRUE, TRUE, 5);
    g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), NULL);
    g_signal_connect(G_OBJECT(darea), "scroll-event", G_CALLBACK(on_scroll_event), NULL);
    g_signal_connect(G_OBJECT(darea), "button-press-event", G_CALLBACK(on_button_press_event), NULL);
    g_signal_connect(G_OBJECT(darea), "button-release-event", G_CALLBACK(on_button_release_event), NULL);
    g_signal_connect(G_OBJECT(darea), "motion-notify-event", G_CALLBACK(on_motion_notify_event), NULL);
    // Ajouter le gestionnaire d'événements pour la molette de la souris
    // Ajouter le gestionnaire d'événements pour le double clic
    gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);

    // Créer une nouvelle boîte horizontale pour inclure le bouton de recentrage
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    gtk_widget_set_visible(hbox, FALSE); // Rendre la boîte horizontale invisible (marche pas)

    // Création du bouton de recentrage
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


    zoom_level_label = gtk_label_new("Niveau de zoom : 1.00");
    gtk_box_pack_end(GTK_BOX(hbox), zoom_level_label, FALSE, FALSE, 5);

    // Création du cadre pour encadrer la zone de texte et la zone de dessin
    GtkWidget *frame = gtk_frame_new("Messages");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5); // Centre le titre

    // Création de la zone de texte pour les messages
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, -1, 100); // Taille minimale pour être visible
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

    // Charger le fichier CSS
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);

    gtk_css_provider_load_from_path(provider, "gtkgui.css", NULL);

    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


    // Affichage de la fenêtre principale
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_widget_show_all(window);

    // Masquer la fenêtre de console sur Windows
    #ifdef _WIN32
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow != NULL) {
        ShowWindow(consoleWindow, SW_MINIMIZE); // Minimiser la fenêtre de la console
    }
    #endif

    // Masquer la fenêtre de console sur Mac
    #ifdef __APPLE__
    hideConsoleWindow();
    #endif
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);


    return status;
}
