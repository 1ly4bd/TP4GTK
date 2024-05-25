#include <gtk/gtk.h>

extern GtkWidget *text_view;

// Fonction pour la mise à jour de la zone de messages
void append_to_message_view(const gchar *message) {
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_start_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, message, -1);
}

