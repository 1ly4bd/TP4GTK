#include <gtk/gtk.h>

// Externe déclaration de l'élément text_view pour être utilisé dans d'autres fichiers
extern GtkWidget *text_view;

void append_to_message_view(const gchar *message) {
    GtkTextBuffer *buffer; // Déclaration d'un pointeur vers GtkTextBuffer
    GtkTextIter iter;      // Déclaration d'un itérateur GtkTextIter

    // Get du buffer associé au widget text_view
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    // Itérateur pointant au début du buffer
    gtk_text_buffer_get_start_iter(buffer, &iter);

    // Insertion du message à l'emplacement de l'itérateur en calculant la longueur de la chaîne automatiquement
    gtk_text_buffer_insert(buffer, &iter, message, -1);
}


