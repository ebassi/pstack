/* GTK - The GIMP Toolkit
 * Copyright © 2013 Carlos Garnacho <carlosg@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __P_BUBBLE_WINDOW_H__
#define __P_BUBBLE_WINDOW_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define P_TYPE_BUBBLE_WINDOW           (_p_bubble_window_get_type ())
#define P_BUBBLE_WINDOW(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), P_TYPE_BUBBLE_WINDOW, PBubbleWindow))
#define P_BUBBLE_WINDOW_CLASS(c)       (G_TYPE_CHECK_CLASS_CAST ((c), P_TYPE_BUBBLE_WINDOW, PBubbleWindowClass))
#define P_IS_BUBBLE_WINDOW(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), P_TYPE_BUBBLE_WINDOW))
#define P_IS_BUBBLE_WINDOW_CLASS(o)    (G_TYPE_CHECK_CLASS_TYPE ((o), P_TYPE_BUBBLE_WINDOW))
#define P_BUBBLE_WINDOW_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), P_TYPE_BUBBLE_WINDOW, PBubbleWindowClass))

typedef struct _PBubbleWindow PBubbleWindow;
typedef struct _PBubbleWindowClass PBubbleWindowClass;

struct _PBubbleWindow
{
  GtkWindow parent_instance;

  /*< private >*/
  gpointer priv;
};

struct _PBubbleWindowClass
{
  GtkWindowClass parent_class;
};

GType       _p_bubble_window_get_type        (void) G_GNUC_CONST;

GtkWidget * _p_bubble_window_new             (void);

void        _p_bubble_window_set_relative_to (PBubbleWindow *window,
                                                GdkWindow       *relative_to);
GdkWindow * _p_bubble_window_get_relative_to (PBubbleWindow *window);

void        _p_bubble_window_set_pointing_to (PBubbleWindow       *window,
                                                cairo_rectangle_int_t *rect);
gboolean    _p_bubble_window_get_pointing_to (PBubbleWindow       *window,
                                                cairo_rectangle_int_t *rect);
void        _p_bubble_window_set_position    (PBubbleWindow       *window,
                                                GtkPositionType        position);

GtkPositionType
            _p_bubble_window_get_position    (PBubbleWindow       *window);

void        _p_bubble_window_popup           (PBubbleWindow       *window,
                                                GdkWindow             *relative_to,
                                                cairo_rectangle_int_t *pointing_to,
                                                GtkPositionType        position);

void        _p_bubble_window_popdown         (PBubbleWindow       *window);

gboolean    _p_bubble_window_grab            (PBubbleWindow       *window,
                                                GdkDevice             *device,
                                                guint32                activate_time);

void        _p_bubble_window_ungrab          (PBubbleWindow       *window);

G_END_DECLS

#endif /* __P_BUBBLE_WINDOW_H__ */
