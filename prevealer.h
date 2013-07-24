/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#ifndef __P_REVEALER_H__
#define __P_REVEALER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define P_TYPE_REVEALER_TRANSITION_TYPE (p_revealer_transition_type_get_type ())
GType p_revealer_transition_type_get_type (void);

#define P_TYPE_REVEALER (p_revealer_get_type ())
#define P_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), P_TYPE_REVEALER, PRevealer))
#define P_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), P_TYPE_REVEALER, PRevealerClass))
#define P_IS_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), P_TYPE_REVEALER))
#define P_IS_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), P_TYPE_REVEALER))
#define P_REVEALER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), P_TYPE_REVEALER, PRevealerClass))

typedef struct _PRevealer PRevealer;
typedef struct _PRevealerClass PRevealerClass;
typedef struct _PRevealerPrivate PRevealerPrivate;

typedef enum {
  P_REVEALER_TRANSITION_TYPE_NONE,
  P_REVEALER_TRANSITION_TYPE_CROSSFADE,
  P_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT,
  P_REVEALER_TRANSITION_TYPE_SLIDE_LEFT,
  P_REVEALER_TRANSITION_TYPE_SLIDE_UP,
  P_REVEALER_TRANSITION_TYPE_SLIDE_DOWN
} PRevealerTransitionType;

struct _PRevealer {
  GtkBin parent_instance;
  PRevealerPrivate * priv;
};

struct _PRevealerClass {
  GtkBinClass parent_class;
};

GType                    p_revealer_get_type                (void) G_GNUC_CONST;

GtkWidget*               p_revealer_new                     (void);
gboolean                 p_revealer_get_reveal_child        (PRevealer               *revealer);
void                     p_revealer_set_reveal_child        (PRevealer               *revealer,
                                                             gboolean                 reveal_child);
gboolean                 p_revealer_get_child_revealed      (PRevealer               *revealer);
guint                    p_revealer_get_transition_duration (PRevealer               *revealer);
void                     p_revealer_set_transition_duration (PRevealer               *revealer,
                                                             guint                    duration);
void                     p_revealer_set_transition_type     (PRevealer               *revealer,
                                                             PRevealerTransitionType  transition);
PRevealerTransitionType  p_revealer_get_transition_type     (PRevealer               *revealer);


G_END_DECLS

#endif
