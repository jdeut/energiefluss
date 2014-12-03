#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <math.h>

#include "app.h"

#define MAX_NWIDGETS   4
#define WINWIDTH   220
#define WINHEIGHT  400

static ClutterActor *widgets[MAX_NWIDGETS];
static ClutterActor *stage;

static gboolean
enter_event_cb (ClutterActor * actor,
                ClutterEvent * event, gpointer user_data)
{
    gdouble scale = 1.05;

    clutter_actor_set_easing_mode (actor,
                                   CLUTTER_EASE_OUT_BOUNCE);
    clutter_actor_set_easing_duration (actor, 100);
    clutter_actor_set_scale (actor, scale, scale);

    return TRUE;
}

static gboolean
leave_event_cb (ClutterActor * actor,
                ClutterEvent * event, gpointer user_data)
{
    gdouble scale = 1;

    clutter_actor_set_easing_mode (actor,
                                   CLUTTER_EASE_IN_BOUNCE);
    clutter_actor_set_easing_duration (actor, 100);
    clutter_actor_set_scale (actor, scale, scale);

    return TRUE;
}

gboolean
draw_system_cb (GtkWidget * widget, cairo_t * cr,
                gpointer data)
{
    guint width, height;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    cairo_rectangle (cr, 0, 0, width, height);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_set_line_width (cr, 7);
    cairo_stroke_preserve (cr);

    cairo_set_source_rgba (cr, 0.4, 0.4, 0.4, 0.5);
    cairo_fill (cr);

    return TRUE;
}

static ClutterActor *
create_gtk_actor_system (int i)
{
    GtkWidget *bin, *drawing_area, *overlay, *label;
    ClutterActor *gtk_actor;
    gchar *str;

    gtk_actor = gtk_clutter_actor_new ();
    bin = gtk_clutter_actor_get_widget (GTK_CLUTTER_ACTOR
                                        (gtk_actor));

    overlay = gtk_overlay_new ();
    gtk_container_add (GTK_CONTAINER (bin), overlay);

    drawing_area = gtk_drawing_area_new ();
    gtk_container_add (GTK_CONTAINER (overlay), drawing_area);

    str = g_strdup_printf ("System %i", i + 1);
    label = gtk_label_new (str);

    gtk_overlay_add_overlay (GTK_OVERLAY (overlay), label);

    gtk_widget_set_size_request (drawing_area, 200, 150);
    g_signal_connect (G_OBJECT (drawing_area), "draw",
                      G_CALLBACK (draw_system_cb), NULL);

    gtk_widget_show_all (bin);

    return gtk_actor;
}

static void
add_clutter_actor_system (ClutterActor * actor,
                          ClutterActor * container, int i)
{
    float x, y;

    clutter_actor_add_child (container, actor);

    x = WINWIDTH / 2 + (WINWIDTH + 50) * i;
    y = 100;

    clutter_actor_set_position (actor, x, y);
    clutter_actor_set_pivot_point (actor, 0.5, 0.5);

    clutter_actor_add_action (actor,
                              clutter_drag_action_new ());

    clutter_actor_set_reactive (actor, TRUE);
}

gboolean
connection_line_draw_cb (ClutterCanvas * canvas, cairo_t * cr,
                         gint width, gint height,
                         gpointer data)
{
    GArray *arr = data;
    ClutterActor *source, *target;

    gfloat x_source, y_source, height_source, width_source;
    gfloat x_end, y_end, height_end, width_end;
    double x0, y0, x1, y1, x2, y2, x3, y3, y_tip, x_tip;
    double arrow_tip_width, arrow_tip_length, alpha;

    PangoLayout *layout;
    PangoFontDescription *desc;

    source = g_array_index (arr, gpointer, 0);
    target = g_array_index (arr, gpointer, 1);

    clutter_actor_get_transformed_position (source,
                                            &x_source,
                                            &y_source);
    clutter_actor_get_transformed_size (source,
                                        &width_source,
                                        &height_source);
    clutter_actor_get_transformed_position (target,
                                            &x_end, &y_end);
    clutter_actor_get_transformed_size (target,
                                        &width_end,
                                        &height_end);

    x0 = x_source + width_source;
    y0 = y_source + height_source / 2;

    x3 = x_end;
    y3 = y_end + height_end / 2;

    arrow_tip_width = 10;
    arrow_tip_length = 50;
    alpha = atan2f (y3 - y0, x3 - x0);

    cairo_set_source_rgb (cr, 0, 1, 0);
    cairo_translate (cr, x3, y3);
    cairo_rotate (cr, alpha);
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, -arrow_tip_length, -arrow_tip_width);
    cairo_line_to (cr, -arrow_tip_length, arrow_tip_width);
    cairo_fill (cr);

    x3 = -arrow_tip_length / 2;
    y3 = 0;

    cairo_user_to_device (cr, &x3, &y3);

    x_tip = x3;
    y_tip = (y3 - y0) / (x3 - x0) * (x_tip - x0) + y0;

    cairo_identity_matrix (cr);

    cairo_move_to (cr, x0, y0);
    cairo_curve_to (cr, x0 + 40, y0, x_tip, y_tip, x_tip,
                    y_tip);
    cairo_set_line_width (cr, 5);
    cairo_stroke (cr);

    cairo_set_source_rgb (cr, 0, 0.5, 0);

    layout = pango_cairo_create_layout (cr);

    pango_layout_set_markup (layout, "<span gravity=\"south\" weight=\"550\" size=\"x-large\">W<sub>el</sub></span>", -1);
    pango_cairo_update_layout (cr, layout);

    cairo_move_to (cr, x0, y0);
    pango_cairo_show_layout (cr, layout);

    return TRUE;
}

void
notify_property_change_cb (GObject * object,
                           GParamSpec * spec, gpointer data)
{
    clutter_content_invalidate (CLUTTER_CONTENT (data));
}

void
add_connection_line (ClutterActor * source,
                     ClutterActor * target)
{
    ClutterActor *actor;
    ClutterContent *canvas;
    ClutterConstraint *width_constraint, *height_constraint;
    cairo_t *cr;
    gfloat width, height;

    actor = clutter_actor_new ();
    canvas = clutter_canvas_new ();

    clutter_actor_set_content (actor, canvas);
    clutter_actor_set_position (actor, 0, 0);

    width_constraint =
        clutter_bind_constraint_new (stage,
                                     CLUTTER_BIND_WIDTH, 0);
    height_constraint =
        clutter_bind_constraint_new (stage,
                                     CLUTTER_BIND_HEIGHT, 0);

    clutter_actor_add_constraint (actor, width_constraint);
    clutter_actor_add_constraint (actor, height_constraint);

    GArray *arr = g_array_new (TRUE, TRUE, sizeof (gpointer));

    g_array_append_val (arr, source);
    g_array_append_val (arr, target);

    g_signal_connect (canvas, "draw",
                      G_CALLBACK (connection_line_draw_cb),
                      arr);

    /* sync canvas size with stage size !!!!! */
    g_object_bind_property (stage, "width", canvas, "width",
                            G_BINDING_SYNC_CREATE);
    g_object_bind_property (stage, "height", canvas, "height",
                            G_BINDING_SYNC_CREATE);

    clutter_actor_add_child (stage, actor);

    g_signal_connect (source, "notify::x",
                      G_CALLBACK (notify_property_change_cb),
                      canvas);
    g_signal_connect (source, "notify::scale-x",
                      G_CALLBACK (notify_property_change_cb),
                      canvas);
    g_signal_connect (source, "notify::y",
                      G_CALLBACK (notify_property_change_cb),
                      canvas);
    g_signal_connect (target, "notify::x",
                      G_CALLBACK (notify_property_change_cb),
                      canvas);
    g_signal_connect (target, "notify::y",
                      G_CALLBACK (notify_property_change_cb),
                      canvas);
}

int
main (int argc, char *argv[])
{
    ClutterColor stage_color = { 0xff, 0xff, 0xff, 0xff };
    GtkWidget *window, *clutter;
    gint i;

    App *app;

    app = (App *) g_new (App, 1);

    gtk_init (&argc, &argv);

    app_init (app);

    GET_UI_ELEMENT (GtkWidget, window1);
    GET_UI_ELEMENT (GtkWidget, viewport1);
    GET_UI_ELEMENT (GtkWidget, scrolledwindow1);

    if (gtk_clutter_init_with_args
        (&argc, &argv, NULL, NULL, NULL,
         NULL) != CLUTTER_INIT_SUCCESS)
        g_error ("Unable to initialize GtkClutter");

    clutter = gtk_clutter_embed_new ();

    gtk_clutter_embed_set_use_layout_size(GTK_CLUTTER_EMBED(clutter), TRUE);

    g_object_set (G_OBJECT (clutter), "hexpand", TRUE,
                  "halign", GTK_ALIGN_FILL, NULL);

    gtk_container_add (GTK_CONTAINER (viewport1), clutter);

    stage =
        gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED
                                     (clutter));

    clutter_actor_set_background_color (stage, &stage_color);

    for (i = 0; i < MAX_NWIDGETS; i++) {
        widgets[i] = create_gtk_actor_system (i);
        add_clutter_actor_system (widgets[i], stage, i);
    }

    add_connection_line (widgets[0], widgets[1]);
    add_connection_line (widgets[1], widgets[2]);
    add_connection_line (widgets[2], widgets[3]);

    g_signal_connect (widgets[0],
                      "enter-event",
                      G_CALLBACK (enter_event_cb), NULL);
    g_signal_connect (widgets[0],
                      "leave-event",
                      G_CALLBACK (leave_event_cb), NULL);

    /*gtk_widget_show_all (window); */
    gtk_widget_show_all (window1);

    gtk_main ();

    return 0;
}
