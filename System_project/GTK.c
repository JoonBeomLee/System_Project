#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <gtk/gtk.h>

#define BUF_SIZE 256

int main(int argc, char *argv[]) {
  FILE *ftp = NULL;

	char cmd[256];

	sprintf(cmd, "./ftp_dir.sh %s 2>&1", argv[1]);
	system(cmd);

  ftp = fopen("./ftp.txt", "r");
	int count = 0;
	int index = 0;
	char buf[BUF_SIZE];

  GtkWidget *window;
  GtkWidget *view;
  GtkWidget *vbox;
  
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  GtkTextIter iter;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
  gtk_window_set_title(GTK_WINDOW(window), "GtkTextView");

  vbox = gtk_vbox_new(FALSE, 0);
  view = gtk_text_view_new();
  gtk_box_pack_start(GTK_BOX(vbox), view, TRUE, TRUE, 0);

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

  gtk_text_buffer_create_tag(buffer, "gap",
        "pixels_above_lines", 30, NULL);

  gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

  //gtk_text_buffer_insert(buffer, &iter, "Plain text\n", -1);
	
	while(!feof(ftp)){
		count++;
		fgets(buf, BUF_SIZE, ftp);

		if(count < 9) continue;	
		if(strstr(buf, "226") != NULL) break;
		//printf("%s" ,buf);
		gtk_text_buffer_insert(buffer, &iter, buf, -1);
	}	
	fclose(ftp);
	
  gtk_container_add(GTK_CONTAINER(window), vbox);

  g_signal_connect(G_OBJECT(window), "destroy",
        G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}

