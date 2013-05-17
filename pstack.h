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

#ifndef __P_STACK_H__
#define __P_STACK_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define P_TYPE_STACK (p_stack_get_type ())
#define P_STACK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), P_TYPE_STACK, PStack))
#define P_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), P_TYPE_STACK, PStackClass))
#define P_IS_STACK(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), P_TYPE_STACK))
#define P_IS_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), P_TYPE_STACK))
#define P_STACK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), P_TYPE_STACK, PStackClass))

typedef struct _PStack PStack;
typedef struct _PStackClass PStackClass;
typedef struct _PStackPrivate PStackPrivate;

typedef enum {
  P_STACK_TRANSITION_TYPE_NONE,
  P_STACK_TRANSITION_TYPE_CROSSFADE,
  P_STACK_TRANSITION_TYPE_SLIDE_RIGHT,
  P_STACK_TRANSITION_TYPE_SLIDE_LEFT,
  P_STACK_TRANSITION_TYPE_SLIDE_UP,
  P_STACK_TRANSITION_TYPE_SLIDE_DOWN
} PStackTransitionType;

struct _PStack {
  GtkContainer parent_instance;
  PStackPrivate *priv;
};

struct _PStackClass {
  GtkContainerClass parent_class;
};

GType                  p_stack_get_type                (void) G_GNUC_CONST;

GtkWidget *            p_stack_new                     (void);
void                   p_stack_add_named               (PStack               *stack,
                                                        GtkWidget            *child,
                                                        const gchar          *name);
void                   p_stack_add_titled              (PStack               *stack,
                                                        GtkWidget            *child,
                                                        const gchar          *name,
                                                        const gchar          *title);
void                   p_stack_set_visible_child       (PStack               *stack,
                                                        GtkWidget            *child);
GtkWidget *            p_stack_get_visible_child       (PStack               *stack);
void                   p_stack_set_visible_child_name  (PStack               *stack,
                                                        const gchar          *name);
const gchar *          p_stack_get_visible_child_name  (PStack               *stack);
void                   p_stack_set_visible_child_full  (PStack               *stack,
                                                        const gchar          *name,
                                                        PStackTransitionType  transition);
void                   p_stack_set_homogeneous         (PStack               *stack,
                                                        gboolean              homogeneous);
gboolean               p_stack_get_homogeneous         (PStack               *stack);
void                   p_stack_set_transition_duration (PStack               *stack,
                                                        guint                 duration);
guint                  p_stack_get_transition_duration (PStack               *stack);
void                   p_stack_set_transition_type     (PStack               *stack,
                                                        PStackTransitionType  transition);
PStackTransitionType   p_stack_get_transition_type     (PStack               *stack);

G_END_DECLS

#endif
