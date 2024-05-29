#include <gtk/gtk.h>

// Externe d�claration de l'�l�ment text_view pour �tre utilis� dans d'autres fichiers
extern GtkWidget *text_view;

void append_to_message_view(const gchar *message) {
    GtkTextBuffer *buffer; // D�claration d'un pointeur vers GtkTextBuffer
    GtkTextIter iter;      // D�claration d'un it�rateur GtkTextIter

    // Get du buffer associ� au widget text_view
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    // It�rateur pointant au d�but du buffer
    gtk_text_buffer_get_start_iter(buffer, &iter);

    // Insertion du message � l'emplacement de l'it�rateur en calculant la longueur de la cha�ne automatiquement
    gtk_text_buffer_insert(buffer, &iter, message, -1);
}


