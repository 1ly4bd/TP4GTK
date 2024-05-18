#include <gtk/gtk.h>
#include "tp4.h"

T_Arbre abr = NULL;

static void inserer_un_element(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int element = atoi(text);
    abr = insererElement(abr, element);
    g_print("Element %d insere.\n", element);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

static void rechercher_un_element(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int element = atoi(text);
    T_Sommet *resultatRecherche = rechercherElement(abr, element);
    if (resultatRecherche != NULL) {
        g_print("Element trouve dans l'intervalle [%d; %d].\n", resultatRecherche->borneInf, resultatRecherche->borneSup);
    } else {
        g_print("Element non trouve.\n");
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

static void supprimer_un_element(GtkWidget *widget, gpointer entry) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int element = atoi(text);
    abr = supprimerElement(abr, element);
    g_print("Element %d supprime.\n", element);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

static void afficher_sommets(GtkWidget *widget, gpointer data) {
    g_print("Affichage des sommets de l'arbre:\n");
    afficherSommets(abr);
}

static void afficher_elements(GtkWidget *widget, gpointer data) {
    g_print("Affichage des elements de l'arbre:\n");
    afficherElements(abr);
}

static void afficher_taille_memoire(GtkWidget *widget, gpointer data) {
    g_print("Tailles memoire de l'arbre:\n");
    tailleMemoire(abr);
}

static void afficher_racine(GtkWidget *widget, gpointer data) {
    if (abr != NULL) {
        g_print("Racine de l'arbre: [%d; %d]\n", abr->borneInf, abr->borneSup);
    } else {
        g_print("L'arbre est vide, pas de racine a afficher.\n");
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *entry;
    GtkWidget *label;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gestion d'Arbre Binaire");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    label = gtk_label_new("Element:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);

    button = gtk_button_new_with_label("Inserer Element");
    g_signal_connect(button, "clicked", G_CALLBACK(inserer_un_element), entry);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 2, 1);

    button = gtk_button_new_with_label("Rechercher Element");
    g_signal_connect(button, "clicked", G_CALLBACK(rechercher_un_element), entry);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 2, 2, 1);

    button = gtk_button_new_with_label("Supprimer Element");
    g_signal_connect(button, "clicked", G_CALLBACK(supprimer_un_element), entry);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 3, 2, 1);

    button = gtk_button_new_with_label("Afficher Sommets");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_sommets), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 4, 2, 1);

    button = gtk_button_new_with_label("Afficher Elements");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_elements), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 5, 2, 1);

    button = gtk_button_new_with_label("Afficher Taille Memoire");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_taille_memoire), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 6, 2, 1);

    button = gtk_button_new_with_label("Afficher Racine");
    g_signal_connect(button, "clicked", G_CALLBACK(afficher_racine), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 7, 2, 1);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
