#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <math.h>

#include <list>
#include <set>

#include "jack.h"

float boxv[][3] = {
	{ -0.5, -0.5, -0.5 },
	{  0.5, -0.5, -0.5 },
	{  0.5,  0.5, -0.5 },
	{ -0.5,  0.5, -0.5 },
	{ -0.5, -0.5,  0.5 },
	{  0.5, -0.5,  0.5 },
	{  0.5,  0.5,  0.5 },
	{ -0.5,  0.5,  0.5 }
};
#define ALPHA 0.5

static float ang = 30.;
Jack g_jack;


static gboolean
expose (GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

	// g_print (" :: expose\n");

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)){
		g_assert_not_reached ();
	}

	/* draw in here */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	

	glShadeModel(GL_FLAT);
	glLineWidth(4.0); 
	glEnable(GL_LINE_SMOOTH); 
	glBegin (GL_LINES);
	glColor4f (1., 1., 1., 0.5);
	// loop over the notes, rendering each. 
	std::list<Note>::iterator it;
	for(it=g_jack.noteVector.begin(); it != g_jack.noteVector.end(); ++it){
		float y = (*it).m_tone; 
		double xs = (double)(*it).m_start / 44100.0; 
		double xe = (double)(*it).m_end / 44100.0; 
		glVertex3f ((float)xs, y, 0.);
		glVertex3f ((float)xe, y, 0.);
	}
	glColor4f (1., 1., 1., 0.7);
	for(it=g_jack.openNotes.begin(); it != g_jack.openNotes.end(); ++it){
		float y = (*it).m_tone; 
		double xs = (double)(*it).m_start / 44100.0; 
		double xe = (double)(g_jack.getPosition()) / 44100.0; 
		glVertex3f ((float)xs, y, 0.);
		glVertex3f ((float)xe, y, 0.);
	}
	glEnd(); 
	
	//draw the cursor.
	glDisable(GL_LINE_SMOOTH); 
	glLineWidth(1.0);
	glColor4f (0.4, 1., 0.6, 0.5);
	glBegin (GL_LINES);
	double xe = (double)(g_jack.getPosition()) / 44100.0;
	glVertex3f ((float)xe, 0, 0.);
	glVertex3f ((float)xe, 128.0, 0.);
	glEnd(); 

	glPopMatrix ();

	if (gdk_gl_drawable_is_double_buffered (gldrawable))
		gdk_gl_drawable_swap_buffers (gldrawable);

	else
		glFlush ();

	gdk_gl_drawable_gl_end (gldrawable);

	return TRUE;
}

static gboolean
configure (GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
	{
		g_assert_not_reached ();
	}

	glLoadIdentity();
	glViewport (0, 0, da->allocation.width, da->allocation.height);
	glOrtho (0,30,0,128,-20050,10000);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glScalef (10., 10., 10.);
	
	gdk_gl_drawable_gl_end (gldrawable);

	return TRUE;
}

static gboolean
rotate (gpointer user_data)
{
	GtkWidget *da = GTK_WIDGET (user_data);

	ang++;

	gdk_window_invalidate_rect (da->window, &da->allocation, FALSE);
	gdk_window_process_updates (da->window, FALSE);

	return TRUE;
}

static void mk_button(const char *label, GtkWidget *container,
                      GtkCallback cb, gpointer data)
{
	GtkWidget *button = gtk_button_new_with_label(label);
	g_signal_connect(button, "clicked", G_CALLBACK(cb), data);
	gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 1);
}

static void cb_play(GtkWidget *, gpointer){
	// refresh the eventSet from the gui data.
	g_jack.eventSet.clear(); 
	unsigned char data[3]; 
	std::list<Note>::iterator it;
	for(it=g_jack.noteVector.begin(); it != g_jack.noteVector.end(); ++it){
		data[0] = 0x90 + (*it).m_chan; 
		data[1] = (*it).m_tone; 
		data[2] = (*it).m_vel_on; 
		MidiEvent m = MidiEvent((*it).m_start, data); 
		g_jack.eventSet.insert(m); 
		data[0] = 0x80 + (*it).m_chan; 
		data[1] = (*it).m_tone; 
		data[2] = (*it).m_vel_off; 
		MidiEvent n = MidiEvent((*it).m_end, data); 
		g_jack.eventSet.insert(n); 
	}
	g_jack.play(); 
}
static void cb_stop(GtkWidget *, gpointer){
	g_jack.stop(); 
}
static void cb_rewind(GtkWidget *, gpointer){
	g_jack.scrub(0); 
}

int main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *da;
	GdkGLConfig *glconfig;

	gtk_init (&argc, &argv);
	gtk_gl_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
	
	GtkWidget *paned;
	paned = gtk_vpaned_new();
	gtk_container_add (GTK_CONTAINER (window), paned);
	
	GtkWidget* hbox; 
	hbox = gtk_hbox_new(TRUE, 1); 
	mk_button("Play", hbox, cb_play, NULL); 
	mk_button("Stop", hbox, cb_stop, NULL); 
	mk_button("RW", hbox, cb_rewind, NULL); 
	
	da = gtk_drawing_area_new (); 
	g_signal_connect_swapped (window, "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_set_events (da, GDK_EXPOSURE_MASK);

	gtk_paned_add1(GTK_PANED(paned), hbox);
	gtk_paned_add2(GTK_PANED(paned), da);
	gtk_widget_show (paned);
	
	gtk_widget_show (window);

	/* prepare GL */
	glconfig = gdk_gl_config_new_by_mode (
		(GdkGLConfigMode)(
			GDK_GL_MODE_RGB |
			GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE));

	if (!glconfig){
		g_assert_not_reached ();
	}

	if (!gtk_widget_set_gl_capability (da, glconfig, NULL, TRUE,
				GDK_GL_RGBA_TYPE)){
		g_assert_not_reached ();
	}

	g_signal_connect (da, "configure-event",
			G_CALLBACK (configure), NULL);
	g_signal_connect (da, "expose-event",
			G_CALLBACK (expose), NULL);
	
	gtk_widget_set_events (da, GDK_EXPOSURE_MASK
	                       | GDK_LEAVE_NOTIFY_MASK
	                       | GDK_BUTTON_PRESS_MASK
	                       | GDK_POINTER_MOTION_MASK
	                       | GDK_POINTER_MOTION_HINT_MASK);
	
	//in order to receive keypresses, must be focusable!
	// http://forums.fedoraforum.org/archive/index.php/t-242963.html
	GTK_WIDGET_SET_FLAGS(da, GTK_CAN_FOCUS );

	gtk_widget_show_all (window);

	g_jack.activate();
  
	g_timeout_add (1000 / 30, rotate, da);

	gtk_main ();
}
