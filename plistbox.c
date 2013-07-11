/*
 * Copyright (C) 2012 Alexander Larsson <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <float.h>
#include <math.h>
#include <cairo.h>
#include <string.h>
#include <gobject/gvaluecollector.h>

#include "plistbox.h"

/* This already exists in gtk as _gtk_marshal_VOID__ENUM_INT, inline it here for now
   to avoid separate marshallers file */
static void
_p_marshal_VOID__ENUM_INT (GClosure * closure,
			     GValue * return_value,
			     guint n_param_values,
			     const GValue * param_values,
			     gpointer invocation_hint,
			     gpointer marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ENUM_INT) (gpointer data1, gint arg_1, gint arg_2, gpointer data2);
  register GMarshalFunc_VOID__ENUM_INT callback;
  register GCClosure * cc;
  register gpointer data1;
  register gpointer data2;
  cc = (GCClosure *) closure;
  g_return_if_fail (n_param_values == 3);
  if (G_CCLOSURE_SWAP_DATA (closure)) {
    data1 = closure->data;
    data2 = param_values->data[0].v_pointer;
  } else {
    data1 = param_values->data[0].v_pointer;
    data2 = closure->data;
  }
  callback = (GMarshalFunc_VOID__ENUM_INT) (marshal_data ? marshal_data : cc->callback);
  callback (data1, g_value_get_enum (param_values + 1), g_value_get_int (param_values + 2), data2);
}

typedef struct _PListBoxChildInfo PListBoxChildInfo;

struct _PListBoxPrivate
{
  GSequence *children;
  GHashTable *child_hash;
  GHashTable *separator_hash;

  PListBoxSortFunc sort_func;
  gpointer sort_func_target;
  GDestroyNotify sort_func_target_destroy_notify;

  PListBoxFilterFunc filter_func;
  gpointer filter_func_target;
  GDestroyNotify filter_func_target_destroy_notify;

  PListBoxUpdateSeparatorFunc update_separator_func;
  gpointer update_separator_func_target;
  GDestroyNotify update_separator_func_target_destroy_notify;

  PListBoxChildInfo *selected_child;
  PListBoxChildInfo *prelight_child;
  PListBoxChildInfo *cursor_child;

  gboolean active_child_active;
  PListBoxChildInfo *active_child;

  GtkSelectionMode selection_mode;

  GtkAdjustment *adjustment;
  gboolean activate_single_click;

  /* DnD */
  GtkWidget *drag_highlighted_widget;
  guint auto_scroll_timeout_id;
};

struct _PListBoxChildInfo
{
  GSequenceIter *iter;
  GtkWidget *widget;
  GtkWidget *separator;
  gint y;
  gint height;
};

enum {
  CHILD_SELECTED,
  CHILD_ACTIVATED,
  ACTIVATE_CURSOR_CHILD,
  TOGGLE_CURSOR_CHILD,
  MOVE_CURSOR,
  REFILTER,
  LAST_SIGNAL
};

enum  {
  PROP_0,
  PROP_SELECTION_MODE,
  PROP_ACTIVATE_ON_SINGLE_CLICK,
  LAST_PROPERTY
};

G_DEFINE_TYPE (PListBox, p_list_box, GTK_TYPE_CONTAINER)

static PListBoxChildInfo *p_list_box_find_child_at_y              (PListBox          *list_box,
								       gint                 y);
static PListBoxChildInfo *p_list_box_lookup_info                  (PListBox          *list_box,
								       GtkWidget           *widget);
static void                 p_list_box_update_selected              (PListBox          *list_box,
								       PListBoxChildInfo *child);
static void                 p_list_box_apply_filter_all             (PListBox          *list_box);
static void                 p_list_box_update_separator             (PListBox          *list_box,
								       GSequenceIter       *iter);
static GSequenceIter *      p_list_box_get_next_visible             (PListBox          *list_box,
								       GSequenceIter       *_iter);
static void                 p_list_box_apply_filter                 (PListBox          *list_box,
								       GtkWidget           *child);
static void                 p_list_box_add_move_binding             (GtkBindingSet       *binding_set,
								       guint                keyval,
								       GdkModifierType      modmask,
								       GtkMovementStep      step,
								       gint                 count);
static void                 p_list_box_update_cursor                (PListBox          *list_box,
								       PListBoxChildInfo *child);
static void                 p_list_box_select_and_activate          (PListBox          *list_box,
								       PListBoxChildInfo *child);
static void                 p_list_box_update_prelight              (PListBox          *list_box,
								       PListBoxChildInfo *child);
static void                 p_list_box_update_active                (PListBox          *list_box,
								       PListBoxChildInfo *child);
static gboolean             p_list_box_real_enter_notify_event      (GtkWidget           *widget,
								       GdkEventCrossing    *event);
static gboolean             p_list_box_real_leave_notify_event      (GtkWidget           *widget,
								       GdkEventCrossing    *event);
static gboolean             p_list_box_real_motion_notify_event     (GtkWidget           *widget,
								       GdkEventMotion      *event);
static gboolean             p_list_box_real_button_press_event      (GtkWidget           *widget,
								       GdkEventButton      *event);
static gboolean             p_list_box_real_button_release_event    (GtkWidget           *widget,
								       GdkEventButton      *event);
static void                 p_list_box_real_show                    (GtkWidget           *widget);
static gboolean             p_list_box_real_focus                   (GtkWidget           *widget,
								       GtkDirectionType     direction);
static GSequenceIter*       p_list_box_get_previous_visible         (PListBox          *list_box,
								       GSequenceIter       *_iter);
static PListBoxChildInfo *p_list_box_get_first_visible            (PListBox          *list_box);
static PListBoxChildInfo *p_list_box_get_last_visible             (PListBox          *list_box);
static gboolean             p_list_box_real_draw                    (GtkWidget           *widget,
								       cairo_t             *cr);
static void                 p_list_box_real_realize                 (GtkWidget           *widget);
static void                 p_list_box_real_add                     (GtkContainer        *container,
								       GtkWidget           *widget);
static void                 p_list_box_real_remove                  (GtkContainer        *container,
								       GtkWidget           *widget);
static void                 p_list_box_real_forall_internal         (GtkContainer        *container,
								       gboolean             include_internals,
								       GtkCallback          callback,
								       void                *callback_target);
static void                 p_list_box_real_compute_expand_internal (GtkWidget           *widget,
								       gboolean            *hexpand,
								       gboolean            *vexpand);
static GType                p_list_box_real_child_type              (GtkContainer        *container);
static GtkSizeRequestMode   p_list_box_real_get_request_mode        (GtkWidget           *widget);
static void                 p_list_box_real_size_allocate           (GtkWidget           *widget,
								       GtkAllocation       *allocation);
static void                 p_list_box_real_drag_leave              (GtkWidget           *widget,
								       GdkDragContext      *context,
								       guint                time_);
static gboolean             p_list_box_real_drag_motion             (GtkWidget           *widget,
								       GdkDragContext      *context,
								       gint                 x,
								       gint                 y,
								       guint                time_);
static void                 p_list_box_real_activate_cursor_child   (PListBox          *list_box);
static void                 p_list_box_real_toggle_cursor_child     (PListBox          *list_box);
static void                 p_list_box_real_move_cursor             (PListBox          *list_box,
								       GtkMovementStep      step,
								       gint                 count);
static void                 p_list_box_real_refilter                (PListBox          *list_box);
static void                 p_list_box_finalize                     (GObject             *obj);


static void                 p_list_box_real_get_preferred_height           (GtkWidget           *widget,
									      gint                *minimum_height,
									      gint                *natural_height);
static void                 p_list_box_real_get_preferred_height_for_width (GtkWidget           *widget,
									      gint                 width,
									      gint                *minimum_height,
									      gint                *natural_height);
static void                 p_list_box_real_get_preferred_width            (GtkWidget           *widget,
									      gint                *minimum_width,
									      gint                *natural_width);
static void                 p_list_box_real_get_preferred_width_for_height (GtkWidget           *widget,
									      gint                 height,
									      gint                *minimum_width,
									      gint                *natural_width);

static GParamSpec *properties[LAST_PROPERTY] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static PListBoxChildInfo*
p_list_box_child_info_new (GtkWidget *widget)
{
  PListBoxChildInfo *info;

  info = g_new0 (PListBoxChildInfo, 1);
  info->widget = g_object_ref (widget);
  return info;
}

static void
p_list_box_child_info_free (PListBoxChildInfo *info)
{
  g_clear_object (&info->widget);
  g_clear_object (&info->separator);
  g_free (info);
}

GtkWidget *
p_list_box_new (void)
{
  return g_object_new (P_TYPE_LIST_BOX, NULL);
}

static void
p_list_box_init (PListBox *list_box)
{
  PListBoxPrivate *priv;

  list_box->priv = priv =
    G_TYPE_INSTANCE_GET_PRIVATE (list_box, P_TYPE_LIST_BOX, PListBoxPrivate);

  gtk_widget_set_can_focus (GTK_WIDGET (list_box), TRUE);
  gtk_widget_set_has_window (GTK_WIDGET (list_box), TRUE);
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (list_box), TRUE);
  priv->selection_mode = GTK_SELECTION_SINGLE;
  priv->activate_single_click = TRUE;

  priv->children = g_sequence_new ((GDestroyNotify)p_list_box_child_info_free);
  priv->child_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
  priv->separator_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
}

static void
p_list_box_get_property (GObject    *obj,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PListBox *list_box = P_LIST_BOX (obj);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, list_box->priv->selection_mode);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, list_box->priv->activate_single_click);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
p_list_box_set_property (GObject      *obj,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PListBox *list_box = P_LIST_BOX (obj);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      p_list_box_set_selection_mode (list_box, g_value_get_enum (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      p_list_box_set_activate_on_single_click (list_box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
p_list_box_finalize (GObject *obj)
{
  PListBox *list_box = P_LIST_BOX (obj);
  PListBoxPrivate *priv = list_box->priv;

  if (priv->auto_scroll_timeout_id != ((guint) 0))
    g_source_remove (priv->auto_scroll_timeout_id);

  if (priv->sort_func_target_destroy_notify != NULL)
    priv->sort_func_target_destroy_notify (priv->sort_func_target);
  if (priv->filter_func_target_destroy_notify != NULL)
    priv->filter_func_target_destroy_notify (priv->filter_func_target);
  if (priv->update_separator_func_target_destroy_notify != NULL)
    priv->update_separator_func_target_destroy_notify (priv->update_separator_func_target);

  g_clear_object (&priv->adjustment);
  g_clear_object (&priv->drag_highlighted_widget);

  g_sequence_free (priv->children);
  g_hash_table_unref (priv->child_hash);
  g_hash_table_unref (priv->separator_hash);

  G_OBJECT_CLASS (p_list_box_parent_class)->finalize (obj);
}

static void
p_list_box_class_init (PListBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
  GtkBindingSet *binding_set;

  p_list_box_parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private (klass, sizeof (PListBoxPrivate));

  object_class->get_property = p_list_box_get_property;
  object_class->set_property = p_list_box_set_property;
  object_class->finalize = p_list_box_finalize;
  widget_class->enter_notify_event = p_list_box_real_enter_notify_event;
  widget_class->leave_notify_event = p_list_box_real_leave_notify_event;
  widget_class->motion_notify_event = p_list_box_real_motion_notify_event;
  widget_class->button_press_event = p_list_box_real_button_press_event;
  widget_class->button_release_event = p_list_box_real_button_release_event;
  widget_class->show = p_list_box_real_show;
  widget_class->focus = p_list_box_real_focus;
  widget_class->draw = p_list_box_real_draw;
  widget_class->realize = p_list_box_real_realize;
  widget_class->compute_expand = p_list_box_real_compute_expand_internal;
  widget_class->get_request_mode = p_list_box_real_get_request_mode;
  widget_class->get_preferred_height = p_list_box_real_get_preferred_height;
  widget_class->get_preferred_height_for_width = p_list_box_real_get_preferred_height_for_width;
  widget_class->get_preferred_width = p_list_box_real_get_preferred_width;
  widget_class->get_preferred_width_for_height = p_list_box_real_get_preferred_width_for_height;
  widget_class->size_allocate = p_list_box_real_size_allocate;
  widget_class->drag_leave = p_list_box_real_drag_leave;
  widget_class->drag_motion = p_list_box_real_drag_motion;
  container_class->add = p_list_box_real_add;
  container_class->remove = p_list_box_real_remove;
  container_class->forall = p_list_box_real_forall_internal;
  container_class->child_type = p_list_box_real_child_type;
  klass->activate_cursor_child = p_list_box_real_activate_cursor_child;
  klass->toggle_cursor_child = p_list_box_real_toggle_cursor_child;
  klass->move_cursor = p_list_box_real_move_cursor;
  klass->refilter = p_list_box_real_refilter;

  properties[PROP_SELECTION_MODE] =
    g_param_spec_enum ("selection-mode",
                       "Selection mode",
                       "The selection mode",
                       GTK_TYPE_SELECTION_MODE,
                       GTK_SELECTION_SINGLE,
                       G_PARAM_READWRITE);

  properties[PROP_ACTIVATE_ON_SINGLE_CLICK] =
    g_param_spec_boolean ("activate-on-single-click",
                          "Activate on Single Click",
                          "Activate row on a single click",
                          TRUE,
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);

  signals[CHILD_SELECTED] =
    g_signal_new ("child-selected",
		  P_TYPE_LIST_BOX,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (PListBoxClass, child_selected),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GTK_TYPE_WIDGET);
  signals[CHILD_ACTIVATED] =
    g_signal_new ("child-activated",
		  P_TYPE_LIST_BOX,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (PListBoxClass, child_activated),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GTK_TYPE_WIDGET);
  signals[ACTIVATE_CURSOR_CHILD] =
    g_signal_new ("activate-cursor-child",
		  P_TYPE_LIST_BOX,
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (PListBoxClass, activate_cursor_child),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  signals[TOGGLE_CURSOR_CHILD] =
    g_signal_new ("toggle-cursor-child",
		  P_TYPE_LIST_BOX,
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (PListBoxClass, toggle_cursor_child),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  signals[MOVE_CURSOR] =
    g_signal_new ("move-cursor",
		  P_TYPE_LIST_BOX,
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (PListBoxClass, move_cursor),
		  NULL, NULL,
		  _p_marshal_VOID__ENUM_INT,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_MOVEMENT_STEP, G_TYPE_INT);
  signals[REFILTER] =
    g_signal_new ("refilter",
		  P_TYPE_LIST_BOX,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (PListBoxClass, refilter),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  widget_class->activate_signal = signals[ACTIVATE_CURSOR_CHILD];

  binding_set = gtk_binding_set_by_class (klass);
  p_list_box_add_move_binding (binding_set, GDK_KEY_Home, 0,
				 GTK_MOVEMENT_BUFFER_ENDS, -1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_KP_Home, 0,
				 GTK_MOVEMENT_BUFFER_ENDS, -1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_End, 0,
				 GTK_MOVEMENT_BUFFER_ENDS, 1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_KP_End, 0,
				 GTK_MOVEMENT_BUFFER_ENDS, 1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_Up, GDK_CONTROL_MASK,
				 GTK_MOVEMENT_DISPLAY_LINES, -1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_KP_Up, GDK_CONTROL_MASK,
				 GTK_MOVEMENT_DISPLAY_LINES, -1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_Down, GDK_CONTROL_MASK,
				 GTK_MOVEMENT_DISPLAY_LINES, 1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_KP_Down, GDK_CONTROL_MASK,
				 GTK_MOVEMENT_DISPLAY_LINES, 1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_Page_Up, 0,
				 GTK_MOVEMENT_PAGES, -1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_KP_Page_Up, 0,
				 GTK_MOVEMENT_PAGES, -1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_Page_Down, 0,
				 GTK_MOVEMENT_PAGES, 1);
  p_list_box_add_move_binding (binding_set, GDK_KEY_KP_Page_Down, 0,
				 GTK_MOVEMENT_PAGES, 1);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_space, GDK_CONTROL_MASK,
				"toggle-cursor-child", 0, NULL);
}

/**
 * p_list_box_get_selected_child:
 * @self: An #PListBox.
 *
 * Gets the selected child.
 *
 * Return value: (transfer none): The selected #GtkWidget.
 **/
GtkWidget *
p_list_box_get_selected_child (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_val_if_fail (list_box != NULL, NULL);

  if (priv->selected_child != NULL)
    return priv->selected_child->widget;

  return NULL;
}

/**
 * p_list_box_get_child_at_y:
 * @self: An #PListBox.
 * @y: position
 *
 * Gets the child at the position.
 *
 * Return value: (transfer none): The child #GtkWidget.
 **/
GtkWidget *
p_list_box_get_child_at_y (PListBox *list_box, gint y)
{
  PListBoxChildInfo *child;

  g_return_val_if_fail (list_box != NULL, NULL);

  child = p_list_box_find_child_at_y (list_box, y);
  if (child == NULL)
    return NULL;

  return child->widget;
}


/**
 * p_list_box_select_child:
 * @self: a #PListBox
 * @child: (allow-none): The child to select or %NULL
 */
void
p_list_box_select_child (PListBox *list_box, GtkWidget *child)
{
  PListBoxChildInfo *info = NULL;

  g_return_if_fail (list_box != NULL);

  if (child != NULL)
    info = p_list_box_lookup_info (list_box, child);

  p_list_box_update_selected (list_box, info);
}

void
p_list_box_set_adjustment (PListBox *list_box,
			     GtkAdjustment *adjustment)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  g_object_ref (adjustment);
  if (priv->adjustment)
    g_object_unref (priv->adjustment);
  priv->adjustment = adjustment;
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list_box),
				       adjustment);
}

void
p_list_box_add_to_scrolled (PListBox *list_box,
			      GtkScrolledWindow *scrolled)
{
  g_return_if_fail (list_box != NULL);
  g_return_if_fail (scrolled != NULL);

  gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (list_box));
  p_list_box_set_adjustment (list_box,
			       gtk_scrolled_window_get_vadjustment (scrolled));
}


void
p_list_box_set_selection_mode (PListBox *list_box, GtkSelectionMode mode)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  if (mode == GTK_SELECTION_MULTIPLE)
    {
      g_warning ("Multiple selections not supported");
      return;
    }

  if (priv->selection_mode == mode)
    return;

  priv->selection_mode = mode;
  if (mode == GTK_SELECTION_NONE)
    p_list_box_update_selected (list_box, NULL);

  g_object_notify_by_pspec (G_OBJECT (list_box), properties[PROP_SELECTION_MODE]);
}


void
p_list_box_set_filter_func (PListBox *list_box,
			      PListBoxFilterFunc f,
			      void *f_target,
			      GDestroyNotify f_target_destroy_notify)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  if (priv->filter_func_target_destroy_notify != NULL)
    priv->filter_func_target_destroy_notify (priv->filter_func_target);

  priv->filter_func = f;
  priv->filter_func_target = f_target;
  priv->filter_func_target_destroy_notify = f_target_destroy_notify;

  p_list_box_refilter (list_box);
}

void
p_list_box_set_separator_funcs (PListBox *list_box,
				  PListBoxUpdateSeparatorFunc update_separator,
				  void *update_separator_target,
				  GDestroyNotify update_separator_target_destroy_notify)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  if (priv->update_separator_func_target_destroy_notify != NULL)
    priv->update_separator_func_target_destroy_notify (priv->update_separator_func_target);

  priv->update_separator_func = update_separator;
  priv->update_separator_func_target = update_separator_target;
  priv->update_separator_func_target_destroy_notify = update_separator_target_destroy_notify;
  p_list_box_reseparate (list_box);
}

static void
p_list_box_real_refilter (PListBox *list_box)
{
  p_list_box_apply_filter_all (list_box);
  p_list_box_reseparate (list_box);
  gtk_widget_queue_resize (GTK_WIDGET (list_box));
}

void
p_list_box_refilter (PListBox *list_box)
{
  g_return_if_fail (list_box != NULL);

  g_signal_emit (list_box, signals[REFILTER], 0);
}

static gint
do_sort (PListBoxChildInfo *a,
	 PListBoxChildInfo *b,
	 PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;

  return priv->sort_func (a->widget, b->widget,
			  priv->sort_func_target);
}

void
p_list_box_resort (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  g_sequence_sort (priv->children,
		   (GCompareDataFunc)do_sort, list_box);
  p_list_box_reseparate (list_box);
  gtk_widget_queue_resize (GTK_WIDGET (list_box));
}

void
p_list_box_reseparate (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;
  GSequenceIter *iter;

  g_return_if_fail (list_box != NULL);

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    p_list_box_update_separator (list_box, iter);

  gtk_widget_queue_resize (GTK_WIDGET (list_box));
}

/**
 * p_list_box_set_sort_func:
 * @f: (closure f_target):
 * @f_target: (allow-none):
 * @f_target_destroy_notify: (allow-none):
 */
void
p_list_box_set_sort_func (PListBox *list_box,
			    PListBoxSortFunc f,
			    void *f_target,
			    GDestroyNotify f_target_destroy_notify)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  if (priv->sort_func_target_destroy_notify != NULL)
    priv->sort_func_target_destroy_notify (priv->sort_func_target);

  priv->sort_func = f;
  priv->sort_func_target = f_target;
  priv->sort_func_target_destroy_notify = f_target_destroy_notify;
  p_list_box_resort (list_box);
}

void
p_list_box_child_changed (PListBox *list_box, GtkWidget *widget)
{
  PListBoxPrivate *priv = list_box->priv;
  PListBoxChildInfo *info;
  GSequenceIter *prev_next, *next;

  g_return_if_fail (list_box != NULL);
  g_return_if_fail (widget != NULL);

  info = p_list_box_lookup_info (list_box, widget);
  if (info == NULL)
    return;

  prev_next = p_list_box_get_next_visible (list_box, info->iter);
  if (priv->sort_func != NULL)
    {
      g_sequence_sort_changed (info->iter,
			       (GCompareDataFunc)do_sort,
			       list_box);
      gtk_widget_queue_resize (GTK_WIDGET (list_box));
    }
  p_list_box_apply_filter (list_box, info->widget);
  if (gtk_widget_get_visible (GTK_WIDGET (list_box)))
    {
      next = p_list_box_get_next_visible (list_box, info->iter);
      p_list_box_update_separator (list_box, info->iter);
      p_list_box_update_separator (list_box, next);
      p_list_box_update_separator (list_box, prev_next);
    }
}

void
p_list_box_set_activate_on_single_click (PListBox *list_box,
					   gboolean single)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  single = single != FALSE;

  if (priv->activate_single_click == single)
    return;

  priv->activate_single_click = single;

  g_object_notify_by_pspec (G_OBJECT (list_box), properties[PROP_ACTIVATE_ON_SINGLE_CLICK]);
}

static void
p_list_box_add_move_binding (GtkBindingSet *binding_set,
			       guint keyval,
			       GdkModifierType modmask,
			       GtkMovementStep step,
			       gint count)
{
  gtk_binding_entry_add_signal (binding_set, keyval, modmask,
				"move-cursor", (guint) 2, GTK_TYPE_MOVEMENT_STEP, step, G_TYPE_INT, count, NULL);

  if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
    return;

  gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK,
				"move-cursor", (guint) 2, GTK_TYPE_MOVEMENT_STEP, step, G_TYPE_INT, count, NULL);
}

static PListBoxChildInfo*
p_list_box_find_child_at_y (PListBox *list_box, gint y)
{
  PListBoxPrivate *priv = list_box->priv;
  PListBoxChildInfo *child_info;
  GSequenceIter *iter;
  PListBoxChildInfo *info;

  child_info = NULL;
  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      info = (PListBoxChildInfo*) g_sequence_get (iter);
      if (y >= info->y && y < (info->y + info->height))
	{
	  child_info = info;
	  break;
	}
    }

  return child_info;
}

static void
p_list_box_update_cursor (PListBox *list_box,
			    PListBoxChildInfo *child)
{
  PListBoxPrivate *priv = list_box->priv;

  priv->cursor_child = child;
  gtk_widget_grab_focus (GTK_WIDGET (list_box));
  gtk_widget_queue_draw (GTK_WIDGET (list_box));
  if (child != NULL && priv->adjustment != NULL)
    {
      GtkAllocation allocation;
      gtk_widget_get_allocation (GTK_WIDGET (list_box), &allocation);
      gtk_adjustment_clamp_page (priv->adjustment,
				 priv->cursor_child->y + allocation.y,
				 priv->cursor_child->y + allocation.y + priv->cursor_child->height);
  }
}

static void
p_list_box_update_selected (PListBox *list_box,
			      PListBoxChildInfo *child)
{
  PListBoxPrivate *priv = list_box->priv;

  if (child != priv->selected_child &&
      (child == NULL || priv->selection_mode != GTK_SELECTION_NONE))
    {
      priv->selected_child = child;
      g_signal_emit (list_box, signals[CHILD_SELECTED], 0,
		     (priv->selected_child != NULL) ? priv->selected_child->widget : NULL);
      gtk_widget_queue_draw (GTK_WIDGET (list_box));
    }
  if (child != NULL)
    p_list_box_update_cursor (list_box, child);
}

static void
p_list_box_select_and_activate (PListBox *list_box, PListBoxChildInfo *child)
{
  GtkWidget *w = NULL;

  if (child != NULL)
    w = child->widget;

  p_list_box_update_selected (list_box, child);

  if (w != NULL)
    g_signal_emit (list_box, signals[CHILD_ACTIVATED], 0, w);
}

static void
p_list_box_update_prelight (PListBox *list_box, PListBoxChildInfo *child)
{
  PListBoxPrivate *priv = list_box->priv;

  if (child != priv->prelight_child)
    {
      priv->prelight_child = child;
      gtk_widget_queue_draw (GTK_WIDGET (list_box));
    }
}

static void
p_list_box_update_active (PListBox *list_box, PListBoxChildInfo *child)
{
  PListBoxPrivate *priv = list_box->priv;
  gboolean val;

  val = priv->active_child == child;
  if (priv->active_child != NULL &&
      val != priv->active_child_active)
    {
      priv->active_child_active = val;
      gtk_widget_queue_draw (GTK_WIDGET (list_box));
    }
}

static gboolean
p_list_box_real_enter_notify_event (GtkWidget *widget,
				      GdkEventCrossing *event)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxChildInfo *child;


  if (event->window != gtk_widget_get_window (GTK_WIDGET (list_box)))
    return FALSE;

  child = p_list_box_find_child_at_y (list_box, event->y);
  p_list_box_update_prelight (list_box, child);
  p_list_box_update_active (list_box, child);

  return FALSE;
}

static gboolean
p_list_box_real_leave_notify_event (GtkWidget *widget,
				      GdkEventCrossing *event)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxChildInfo *child = NULL;

  if (event->window != gtk_widget_get_window (GTK_WIDGET (list_box)))
    return FALSE;

  if (event->detail != GDK_NOTIFY_INFERIOR)
    child = NULL;
  else
    child = p_list_box_find_child_at_y (list_box, event->y);

  p_list_box_update_prelight (list_box, child);
  p_list_box_update_active (list_box, child);

  return FALSE;
}

static gboolean
p_list_box_real_motion_notify_event (GtkWidget *widget,
				       GdkEventMotion *event)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxChildInfo *child;
  GdkWindow *window, *event_window;
  gint relative_y;
  gdouble parent_y;

  window = gtk_widget_get_window (GTK_WIDGET (list_box));
  event_window = event->window;
  relative_y = event->y;

  while ((event_window != NULL) && (event_window != window))
    {
      gdk_window_coords_to_parent (event_window, 0, relative_y, NULL, &parent_y);
      relative_y = parent_y;
      event_window = gdk_window_get_effective_parent (event_window);
    }

  child = p_list_box_find_child_at_y (list_box, relative_y);
  p_list_box_update_prelight (list_box, child);
  p_list_box_update_active (list_box, child);

  return FALSE;
}

static gboolean
p_list_box_real_button_press_event (GtkWidget *widget,
				      GdkEventButton *event)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      PListBoxChildInfo *child;
      child = p_list_box_find_child_at_y (list_box, event->y);
      if (child != NULL)
	{
	  priv->active_child = child;
	  priv->active_child_active = TRUE;
	  gtk_widget_queue_draw (GTK_WIDGET (list_box));
	  if (event->type == GDK_2BUTTON_PRESS &&
	      !priv->activate_single_click)
	    g_signal_emit (list_box, signals[CHILD_ACTIVATED], 0,
			   child->widget);

	}
      /* TODO:
	 Should mark as active while down,
	 and handle grab breaks */
    }

  return FALSE;
}

static gboolean
p_list_box_real_button_release_event (GtkWidget *widget,
					GdkEventButton *event)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      if (priv->active_child != NULL &&
          priv->active_child_active)
        {
          if (priv->activate_single_click)
            p_list_box_select_and_activate (list_box, priv->active_child);
          else
            p_list_box_update_selected (list_box, priv->active_child);
        }
      priv->active_child = NULL;
      priv->active_child_active = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (list_box));
  }

  return FALSE;
}

static void
p_list_box_real_show (GtkWidget *widget)
{
  PListBox * list_box = P_LIST_BOX (widget);

  p_list_box_reseparate (list_box);

  GTK_WIDGET_CLASS (p_list_box_parent_class)->show ((GtkWidget*) G_TYPE_CHECK_INSTANCE_CAST (list_box, GTK_TYPE_CONTAINER, GtkContainer));
}


static gboolean
p_list_box_real_focus (GtkWidget* widget, GtkDirectionType direction)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;
  gboolean had_focus = FALSE;
  gboolean focus_into = FALSE;
  GtkWidget* recurse_into;
  PListBoxChildInfo *current_focus_child;
  PListBoxChildInfo *next_focus_child;
  gboolean modify_selection_pressed;
  GdkModifierType state = 0;

  recurse_into = NULL;
  focus_into = TRUE;

  g_object_get (GTK_WIDGET (list_box), "has-focus", &had_focus, NULL);
  current_focus_child = NULL;
  next_focus_child = NULL;
  if (had_focus)
    {
      /* If on row, going right, enter into possible container */
      if (direction == GTK_DIR_RIGHT || direction == GTK_DIR_TAB_FORWARD)
	{
	  if (priv->cursor_child != NULL)
	    recurse_into = priv->cursor_child->widget;
	}
      current_focus_child = priv->cursor_child;
      /* Unless we're going up/down we're always leaving
      the container */
      if (direction != GTK_DIR_UP && direction != GTK_DIR_DOWN)
	focus_into = FALSE;
    }
  else if (gtk_container_get_focus_child ((GtkContainer*) list_box) != NULL)
    {
      /* There is a focus child, always navigat inside it first */
      recurse_into = gtk_container_get_focus_child ((GtkContainer*) list_box);
      current_focus_child = p_list_box_lookup_info (list_box, recurse_into);

      /* If exiting child container to the right, exit row */
      if (direction == GTK_DIR_RIGHT || direction == GTK_DIR_TAB_FORWARD)
	focus_into = FALSE;

      /* If exiting child container to the left, select row or out */
      if (direction == GTK_DIR_LEFT || direction == GTK_DIR_TAB_BACKWARD)
	next_focus_child = current_focus_child;
    }
  else
    {
      /* If coming from the left, enter into possible container */
      if (direction == GTK_DIR_LEFT || direction == GTK_DIR_TAB_BACKWARD)
	{
	  if (priv->selected_child != NULL)
	    recurse_into = priv->selected_child->widget;
	}
    }

  if (recurse_into != NULL)
    {
      if (gtk_widget_child_focus (recurse_into, direction))
	return TRUE;
    }

  if (!focus_into)
    return FALSE; /* Focus is leaving us */

  /* TODO: This doesn't handle up/down going into a focusable separator */

  if (next_focus_child == NULL)
    {
      if (current_focus_child != NULL)
	{
	  GSequenceIter* i;
	  if (direction == GTK_DIR_UP)
	    {
	      i = p_list_box_get_previous_visible (list_box, current_focus_child->iter);
	      if (i != NULL)
		next_focus_child = g_sequence_get (i);

	    }
	  else
	    {
	      i = p_list_box_get_next_visible (list_box, current_focus_child->iter);
	      if (!g_sequence_iter_is_end (i))
		next_focus_child = g_sequence_get (i);

	    }
	}
      else
	{
	  switch (direction)
	    {
	    case GTK_DIR_UP:
	    case GTK_DIR_TAB_BACKWARD:
	      next_focus_child = priv->selected_child;
	      if (next_focus_child == NULL)
		next_focus_child = p_list_box_get_last_visible (list_box);
	      break;
	    default:
	      next_focus_child = priv->selected_child;
	      if (next_focus_child == NULL)
		next_focus_child =
		  p_list_box_get_first_visible (list_box);
	      break;
	    }
	}
    }

  if (next_focus_child == NULL)
    {
      if (direction == GTK_DIR_UP || direction == GTK_DIR_DOWN)
	{
          if (gtk_widget_keynav_failed (GTK_WIDGET (list_box), direction))
	    return TRUE;
	}

      return FALSE;
    }

  modify_selection_pressed = FALSE;
  if (gtk_get_current_event_state (&state))
    {
      GdkModifierType modify_mod_mask;
      modify_mod_mask =
	gtk_widget_get_modifier_mask (GTK_WIDGET (list_box),
				      GDK_MODIFIER_INTENT_MODIFY_SELECTION);
      if ((state & modify_mod_mask) == modify_mod_mask)
	modify_selection_pressed = TRUE;
    }

  if (modify_selection_pressed)
    p_list_box_update_cursor (list_box, next_focus_child);
  else
    p_list_box_update_selected (list_box, next_focus_child);

  return TRUE;
}

typedef struct {
  PListBoxChildInfo *child;
  GtkStateFlags state;
} ChildFlags;

static ChildFlags*
child_flags_find_or_add (ChildFlags *array,
			 int *array_length,
			 PListBoxChildInfo *to_find)
{
  gint i;

  for (i = 0; i < *array_length; i++)
    {
      if (array[i].child == to_find)
	return &array[i];
    }

  *array_length = *array_length + 1;
  array[*array_length - 1].child = to_find;
  array[*array_length - 1].state = 0;
  return &array[*array_length - 1];
}

static gboolean
p_list_box_real_draw (GtkWidget* widget, cairo_t* cr)
{
  PListBox * list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;
  GtkAllocation allocation = {0};
  GtkStyleContext* context;
  GtkStateFlags state;
  ChildFlags flags[3], *found;
  gint flags_length;
  gint focus_pad;
  int i;

  gtk_widget_get_allocation (GTK_WIDGET (list_box), &allocation);
  context = gtk_widget_get_style_context (GTK_WIDGET (list_box));
  state = gtk_widget_get_state_flags (widget);
  gtk_render_background (context, cr, (gdouble) 0, (gdouble) 0, (gdouble) allocation.width, (gdouble) allocation.height);
  flags_length = 0;

  if (priv->selected_child != NULL)
    {
      found = child_flags_find_or_add (flags, &flags_length, priv->selected_child);
      found->state |= (state | GTK_STATE_FLAG_SELECTED);
    }

  if (priv->prelight_child != NULL)
    {
      found = child_flags_find_or_add (flags, &flags_length, priv->prelight_child);
      found->state |= (state | GTK_STATE_FLAG_PRELIGHT);
    }

  if (priv->active_child != NULL && priv->active_child_active)
    {
      found = child_flags_find_or_add (flags, &flags_length, priv->active_child);
      found->state |= (state | GTK_STATE_FLAG_ACTIVE);
    }

  for (i = 0; i < flags_length; i++)
    {
      ChildFlags *flag = &flags[i];
      gtk_style_context_save (context);
      gtk_style_context_set_state (context, flag->state);
      gtk_render_background (context, cr, 0, flag->child->y, allocation.width, flag->child->height);
      gtk_style_context_restore (context);
    }

  if (gtk_widget_has_visible_focus (GTK_WIDGET (list_box)) && priv->cursor_child != NULL)
    {
      gtk_style_context_get_style (context,
                                   "focus-padding", &focus_pad,
                                   NULL);
      gtk_render_focus (context, cr, focus_pad, priv->cursor_child->y + focus_pad,
                        allocation.width - 2 * focus_pad, priv->cursor_child->height - 2 * focus_pad);
    }

  GTK_WIDGET_CLASS (p_list_box_parent_class)->draw ((GtkWidget*) G_TYPE_CHECK_INSTANCE_CAST (list_box, GTK_TYPE_CONTAINER, GtkContainer), cr);

  return TRUE;
}


static void
p_list_box_real_realize (GtkWidget* widget)
{
  PListBox *list_box = P_LIST_BOX (widget);
  GtkAllocation allocation;
  GdkWindowAttr attributes = {0};
  GdkWindow *window;

  gtk_widget_get_allocation (GTK_WIDGET (list_box), &allocation);
  gtk_widget_set_realized (GTK_WIDGET (list_box), TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (GTK_WIDGET (list_box)) |
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK |
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;
  attributes.wclass = GDK_INPUT_OUTPUT;

  window = gdk_window_new (gtk_widget_get_parent_window (GTK_WIDGET (list_box)),
			   &attributes, GDK_WA_X | GDK_WA_Y);
  gtk_style_context_set_background (gtk_widget_get_style_context (GTK_WIDGET (list_box)), window);
  gdk_window_set_user_data (window, (GObject*) list_box);
  gtk_widget_set_window (GTK_WIDGET (list_box), window); /* Passes ownership */
}


static void
p_list_box_apply_filter (PListBox *list_box, GtkWidget *child)
{
  PListBoxPrivate *priv = list_box->priv;
  gboolean do_show;

  do_show = TRUE;
  if (priv->filter_func != NULL)
    do_show = priv->filter_func (child, priv->filter_func_target);

  gtk_widget_set_child_visible (child, do_show);
}

static void
p_list_box_apply_filter_all (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;
  PListBoxChildInfo *child_info;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child_info = g_sequence_get (iter);
      p_list_box_apply_filter (list_box, child_info->widget);
    }
}

/* Children are visible if they are shown by the app (visible)
   and not filtered out (child_visible) by the listbox */
static gboolean
child_is_visible (GtkWidget *child)
{
  return gtk_widget_get_visible (child) && gtk_widget_get_child_visible (child);
}

static PListBoxChildInfo*
p_list_box_get_first_visible (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;
  PListBoxChildInfo *child_info;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
	child_info = g_sequence_get (iter);
	if (child_is_visible (child_info->widget))
	  return child_info;
    }

  return NULL;
}


static PListBoxChildInfo*
p_list_box_get_last_visible (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;
  PListBoxChildInfo *child_info;
  GSequenceIter *iter;

  iter = g_sequence_get_end_iter (priv->children);
  while (!g_sequence_iter_is_begin (iter))
    {
      iter = g_sequence_iter_prev (iter);
      child_info = g_sequence_get (iter);
      if (child_is_visible (child_info->widget))
	return child_info;
    }

  return NULL;
}

static GSequenceIter*
p_list_box_get_previous_visible (PListBox *list_box,
				   GSequenceIter* iter)
{
  PListBoxChildInfo *child_info;

  if (g_sequence_iter_is_begin (iter))
    return NULL;

  do
    {
      iter = g_sequence_iter_prev (iter);
      child_info = g_sequence_get (iter);
      if (child_is_visible (child_info->widget))
	return iter;
    }
  while (!g_sequence_iter_is_begin (iter));

  return NULL;
}

static GSequenceIter*
p_list_box_get_next_visible (PListBox *list_box, GSequenceIter* iter)
{
  PListBoxChildInfo *child_info;

  if (g_sequence_iter_is_end (iter))
    return iter;

  do
    {
      iter = g_sequence_iter_next (iter);
      if (!g_sequence_iter_is_end (iter))
	{
	child_info = g_sequence_get (iter);
	if (child_is_visible (child_info->widget))
	  return iter;
	}
    }
  while (!g_sequence_iter_is_end (iter));

  return iter;
}


static void
p_list_box_update_separator (PListBox *list_box, GSequenceIter* iter)
{
  PListBoxPrivate *priv = list_box->priv;
  PListBoxChildInfo *info;
  GSequenceIter *before_iter;
  GtkWidget *child;
  GtkWidget *before_child;
  PListBoxChildInfo *before_info;
  GtkWidget *old_separator;

  if (iter == NULL || g_sequence_iter_is_end (iter))
    return;

  info = g_sequence_get (iter);
  before_iter = p_list_box_get_previous_visible (list_box, iter);
  child = info->widget;
  if (child)
    g_object_ref (child);
  before_child = NULL;
  if (before_iter != NULL)
    {
      before_info = g_sequence_get (before_iter);
      before_child = before_info->widget;
      if (before_child)
	g_object_ref (before_child);
    }

  if (priv->update_separator_func != NULL &&
      child_is_visible (child))
    {
      old_separator = info->separator;
      if (old_separator)
	g_object_ref (old_separator);
      priv->update_separator_func (&info->separator,
				   child,
				   before_child,
				   priv->update_separator_func_target);
      if (old_separator != info->separator)
	{
	  if (old_separator != NULL)
	    {
	      gtk_widget_unparent (old_separator);
	      g_hash_table_remove (priv->separator_hash, old_separator);
	    }
	  if (info->separator != NULL)
	    {
	      g_hash_table_insert (priv->separator_hash, info->separator, info);
	      gtk_widget_set_parent (info->separator, GTK_WIDGET (list_box));
	      gtk_widget_show (info->separator);
	    }
	  gtk_widget_queue_resize (GTK_WIDGET (list_box));
	}
      if (old_separator)
	g_object_unref (old_separator);
    }
  else
    {
      if (info->separator != NULL)
	{
	  g_hash_table_remove (priv->separator_hash, info->separator);
	  gtk_widget_unparent (info->separator);
	  g_clear_object (&info->separator);
	  gtk_widget_queue_resize (GTK_WIDGET (list_box));
	}
    }
  if (before_child)
    g_object_unref (before_child);
  if (child)
    g_object_unref (child);
}

static PListBoxChildInfo*
p_list_box_lookup_info (PListBox *list_box, GtkWidget* child)
{
  PListBoxPrivate *priv = list_box->priv;

  return g_hash_table_lookup (priv->child_hash, child);
}

static void
child_visibility_changed (GObject* object, GParamSpec* pspec, PListBox *list_box)
{
  PListBoxChildInfo *info;

  if (gtk_widget_get_visible (GTK_WIDGET (list_box)))
    {
      info = p_list_box_lookup_info (list_box, GTK_WIDGET (object));
      if (info != NULL)
	{
	  p_list_box_update_separator (list_box, info->iter);
	  p_list_box_update_separator (list_box,
					 p_list_box_get_next_visible (list_box, info->iter));
	}
    }
}

static void
p_list_box_real_add (GtkContainer* container, GtkWidget* child)
{
  PListBox *list_box = P_LIST_BOX (container);
  PListBoxPrivate *priv = list_box->priv;
  PListBoxChildInfo *info;
  GSequenceIter* iter = NULL;
  info = p_list_box_child_info_new (child);
  g_hash_table_insert (priv->child_hash, child, info);
  if (priv->sort_func != NULL)
    iter = g_sequence_insert_sorted (priv->children, info,
				     (GCompareDataFunc)do_sort, list_box);
  else
    iter = g_sequence_append (priv->children, info);

  info->iter = iter;
  gtk_widget_set_parent (child, GTK_WIDGET (list_box));
  p_list_box_apply_filter (list_box, child);
  if (gtk_widget_get_visible (GTK_WIDGET (list_box)))
    {
      p_list_box_update_separator (list_box, iter);
      p_list_box_update_separator (list_box, p_list_box_get_next_visible (list_box, iter));
    }
  g_signal_connect_object (child, "notify::visible",
			   (GCallback) child_visibility_changed, list_box, 0);
}

static void
p_list_box_real_remove (GtkContainer* container, GtkWidget* child)
{
  PListBox *list_box = P_LIST_BOX (container);
  PListBoxPrivate *priv = list_box->priv;
  gboolean was_visible;
  PListBoxChildInfo *info;
  GSequenceIter *next;

  g_return_if_fail (child != NULL);
  was_visible = gtk_widget_get_visible (child);

  g_signal_handlers_disconnect_by_func (child, (GCallback) child_visibility_changed, list_box);

  info = p_list_box_lookup_info (list_box, child);
  if (info == NULL)
    {
      info = g_hash_table_lookup (priv->separator_hash, child);
      if (info != NULL)
	{
	  g_hash_table_remove (priv->separator_hash, child);
	  g_clear_object (&info->separator);
	  gtk_widget_unparent (child);
	  if (was_visible && gtk_widget_get_visible (GTK_WIDGET (list_box)))
	    gtk_widget_queue_resize (GTK_WIDGET (list_box));
	}
      else
	{
	  g_warning ("egg-list-box.vala:846: Tried to remove non-child %p\n", child);
	}
      return;
    }

  if (info->separator != NULL)
    {
      g_hash_table_remove (priv->separator_hash, info->separator);
      gtk_widget_unparent (info->separator);
      g_clear_object (&info->separator);
    }

  if (info == priv->selected_child)
      p_list_box_update_selected (list_box, NULL);
  if (info == priv->prelight_child)
    priv->prelight_child = NULL;
  if (info == priv->cursor_child)
    priv->cursor_child = NULL;
  if (info == priv->active_child)
    priv->active_child = NULL;

  next = p_list_box_get_next_visible (list_box, info->iter);
  gtk_widget_unparent (child);
  g_hash_table_remove (priv->child_hash, child);
  g_sequence_remove (info->iter);
  if (gtk_widget_get_visible (GTK_WIDGET (list_box)))
    p_list_box_update_separator (list_box, next);

  if (was_visible && gtk_widget_get_visible (GTK_WIDGET (list_box)))
    gtk_widget_queue_resize (GTK_WIDGET (list_box));
}


static void
p_list_box_real_forall_internal (GtkContainer* container,
				   gboolean include_internals,
				   GtkCallback callback,
				   void* callback_target)
{
  PListBox *list_box = P_LIST_BOX (container);
  PListBoxPrivate *priv = list_box->priv;
  GSequenceIter *iter;
  PListBoxChildInfo *child_info;

  iter = g_sequence_get_begin_iter (priv->children);
  while (!g_sequence_iter_is_end (iter))
    {
      child_info = g_sequence_get (iter);
      iter = g_sequence_iter_next (iter);
      if (child_info->separator != NULL && include_internals)
	callback (child_info->separator, callback_target);
      callback (child_info->widget, callback_target);
    }
}

static void
p_list_box_real_compute_expand_internal (GtkWidget* widget,
					   gboolean* hexpand,
					   gboolean* vexpand)
{
  GTK_WIDGET_CLASS (p_list_box_parent_class)->compute_expand (widget,
								hexpand, vexpand);

  /* We don't expand vertically beyound the minimum size */
  if (vexpand)
    *vexpand = FALSE;
}

static GType
p_list_box_real_child_type (GtkContainer* container)
{
  return GTK_TYPE_WIDGET;
}

static GtkSizeRequestMode
p_list_box_real_get_request_mode (GtkWidget* widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
p_list_box_real_get_preferred_height (GtkWidget* widget,
					gint* minimum_height,
					gint* natural_height)
{
  gint natural_width;
  p_list_box_real_get_preferred_width (widget, NULL, &natural_width);
  p_list_box_real_get_preferred_height_for_width (widget, natural_width,
						    minimum_height, natural_height);
}

static void
p_list_box_real_get_preferred_height_for_width (GtkWidget* widget, gint width,
						  gint* minimum_height_out, gint* natural_height_out)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;
  GSequenceIter *iter;
  gint minimum_height;
  gint natural_height;
  GtkStyleContext *context;
  gint focus_width;
  gint focus_pad;

  minimum_height = 0;

  context = gtk_widget_get_style_context (GTK_WIDGET (list_box));
  gtk_style_context_get_style (context,
			       "focus-line-width", &focus_width,
			       "focus-padding", &focus_pad, NULL);

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      PListBoxChildInfo *child_info;
      GtkWidget *child;
      gint child_min = 0;
      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
	continue;

      if (child_info->separator != NULL)
	{
	  gtk_widget_get_preferred_height_for_width (child_info->separator, width, &child_min, NULL);
	  minimum_height += child_min;
	}
      gtk_widget_get_preferred_height_for_width (child, width - 2 * (focus_width + focus_pad),
						 &child_min, NULL);
      minimum_height += child_min + 2 * (focus_width + focus_pad);
    }

  /* We always allocate the minimum height, since handling
     expanding rows is way too costly, and unlikely to
     be used, as lists are generally put inside a scrolling window
     anyway.
  */
  natural_height = minimum_height;
  if (minimum_height_out)
    *minimum_height_out = minimum_height;
  if (natural_height_out)
    *natural_height_out = natural_height;
}

static void
p_list_box_real_get_preferred_width (GtkWidget* widget, gint* minimum_width_out, gint* natural_width_out)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;
  gint minimum_width;
  gint natural_width;
  GtkStyleContext *context;
  gint focus_width;
  gint focus_pad;
  GSequenceIter *iter;
  PListBoxChildInfo *child_info;
  GtkWidget *child;
  gint child_min;
  gint child_nat;

  context = gtk_widget_get_style_context (GTK_WIDGET (list_box));
  gtk_style_context_get_style (context, "focus-line-width", &focus_width, "focus-padding", &focus_pad, NULL);

  minimum_width = 0;
  natural_width = 0;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child_info = g_sequence_get (iter);
      child = child_info->widget;
      if (!child_is_visible (child))
	continue;

      gtk_widget_get_preferred_width (child, &child_min, &child_nat);
      minimum_width = MAX (minimum_width, child_min + 2 * (focus_width + focus_pad));
      natural_width = MAX (natural_width, child_nat + 2 * (focus_width + focus_pad));

      if (child_info->separator != NULL)
	{
	  gtk_widget_get_preferred_width (child_info->separator, &child_min, &child_nat);
	  minimum_width = MAX (minimum_width, child_min);
	  natural_width = MAX (natural_width, child_nat);
	}
    }

  if (minimum_width_out)
    *minimum_width_out = minimum_width;
  if (natural_width_out)
    *natural_width_out = natural_width;
}

static void
p_list_box_real_get_preferred_width_for_height (GtkWidget *widget, gint height,
						  gint *minimum_width, gint *natural_width)
{
  PListBox *list_box = P_LIST_BOX (widget);
  p_list_box_real_get_preferred_width (GTK_WIDGET (list_box), minimum_width, natural_width);
}

static void
p_list_box_real_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;
  GtkAllocation child_allocation;
  GtkAllocation separator_allocation;
  PListBoxChildInfo *child_info;
  GdkWindow *window;
  GtkWidget *child;
  GSequenceIter *iter;
  GtkStyleContext *context;
  gint focus_width;
  gint focus_pad;
  int child_min;


  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = 0;
  child_allocation.height = 0;

  separator_allocation.x = 0;
  separator_allocation.y = 0;
  separator_allocation.width = 0;
  separator_allocation.height = 0;

  gtk_widget_set_allocation (GTK_WIDGET (list_box), allocation);
  window = gtk_widget_get_window (GTK_WIDGET (list_box));
  if (window != NULL)
    gdk_window_move_resize (window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  context = gtk_widget_get_style_context (GTK_WIDGET (list_box));
  gtk_style_context_get_style (context,
			       "focus-line-width", &focus_width,
			       "focus-padding", &focus_pad,
			       NULL);
  child_allocation.x = 0 + focus_width + focus_pad;
  child_allocation.y = 0;
  child_allocation.width = allocation->width - 2 * (focus_width + focus_pad);
  separator_allocation.x = 0;
  separator_allocation.width = allocation->width;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child_info = g_sequence_get (iter);
      child = child_info->widget;
      if (!child_is_visible (child))
	{
	  child_info->y = child_allocation.y;
	  child_info->height = 0;
	  continue;
	}

      if (child_info->separator != NULL)
	{
	  gtk_widget_get_preferred_height_for_width (child_info->separator,
						     allocation->width, &child_min, NULL);
	  separator_allocation.height = child_min;
	  separator_allocation.y = child_allocation.y;
	  gtk_widget_size_allocate (child_info->separator,
				    &separator_allocation);
	  child_allocation.y += child_min;
	}

      child_info->y = child_allocation.y;
      child_allocation.y += focus_width + focus_pad;

      gtk_widget_get_preferred_height_for_width (child, child_allocation.width, &child_min, NULL);
      child_allocation.height = child_min;

      child_info->height = child_allocation.height + 2 * (focus_width + focus_pad);
      gtk_widget_size_allocate (child, &child_allocation);

      child_allocation.y += child_min + focus_width + focus_pad;
    }
}

void
p_list_box_drag_unhighlight_widget (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;

  g_return_if_fail (list_box != NULL);

  if (priv->drag_highlighted_widget == NULL)
    return;

  gtk_drag_unhighlight (priv->drag_highlighted_widget);
  g_clear_object (&priv->drag_highlighted_widget);
}


void
p_list_box_drag_highlight_widget (PListBox *list_box, GtkWidget *child)
{
  PListBoxPrivate *priv = list_box->priv;
  GtkWidget *old_highlight;

  g_return_if_fail (list_box != NULL);
  g_return_if_fail (child != NULL);

  if (priv->drag_highlighted_widget == child)
    return;

  p_list_box_drag_unhighlight_widget (list_box);
  gtk_drag_highlight (child);

  old_highlight = priv->drag_highlighted_widget;
  priv->drag_highlighted_widget = g_object_ref (child);
  if (old_highlight)
    g_object_unref (old_highlight);
}

static void
p_list_box_real_drag_leave (GtkWidget *widget, GdkDragContext *context, guint time_)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;

  p_list_box_drag_unhighlight_widget (list_box);
  if (priv->auto_scroll_timeout_id != 0) {
    g_source_remove (priv->auto_scroll_timeout_id);
    priv->auto_scroll_timeout_id = 0;
  }
}

typedef struct
{
  PListBox *list_box;
  gint move;
} MoveData;

static void
move_data_free (MoveData *data)
{
  g_slice_free (MoveData, data);
}

static gboolean
drag_motion_timeout (MoveData *data)
{
  PListBox *list_box = data->list_box;
  PListBoxPrivate *priv = list_box->priv;

  gtk_adjustment_set_value (priv->adjustment,
			    gtk_adjustment_get_value (priv->adjustment) +
			    gtk_adjustment_get_step_increment (priv->adjustment) * data->move);
  return TRUE;
}

static gboolean
p_list_box_real_drag_motion (GtkWidget *widget, GdkDragContext *context,
			       gint x, gint y, guint time_)
{
  PListBox *list_box = P_LIST_BOX (widget);
  PListBoxPrivate *priv = list_box->priv;
  int move;
  MoveData *data;
  gdouble size;

  /* Auto-scroll during Dnd if cursor is moving into the top/bottom portion of the
     * box. */
  if (priv->auto_scroll_timeout_id != 0)
    {
      g_source_remove (priv->auto_scroll_timeout_id);
      priv->auto_scroll_timeout_id = 0;
    }

  if (priv->adjustment == NULL)
    return FALSE;

  /* Part of the view triggering auto-scroll */
  size = 30;
  move = 0;

  if (y < (gtk_adjustment_get_value (priv->adjustment) + size))
    {
      /* Scroll up */
      move = -1;
    }
  else if (y > ((gtk_adjustment_get_value (priv->adjustment) + gtk_adjustment_get_page_size (priv->adjustment)) - size))
    {
      /* Scroll down */
      move = 1;
    }

  if (move == 0)
    return FALSE;

  data = g_slice_new0 (MoveData);
  data->list_box = list_box;

  priv->auto_scroll_timeout_id =
    g_timeout_add_full (G_PRIORITY_DEFAULT, 150, (GSourceFunc)drag_motion_timeout,
			data, (GDestroyNotify) move_data_free);

  return FALSE;
}

static void
p_list_box_real_activate_cursor_child (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;

  p_list_box_select_and_activate (list_box, priv->cursor_child);
}

static void
p_list_box_real_toggle_cursor_child (PListBox *list_box)
{
  PListBoxPrivate *priv = list_box->priv;

  if (priv->cursor_child == NULL)
    return;

  if (priv->selection_mode == GTK_SELECTION_SINGLE &&
      priv->selected_child == priv->cursor_child)
    p_list_box_update_selected (list_box, NULL);
  else
    p_list_box_select_and_activate (list_box, priv->cursor_child);
}

static void
p_list_box_real_move_cursor (PListBox *list_box,
			       GtkMovementStep step,
			       gint count)
{
  PListBoxPrivate *priv = list_box->priv;
  GdkModifierType state;
  gboolean modify_selection_pressed;
  PListBoxChildInfo *child;
  GdkModifierType modify_mod_mask;
  PListBoxChildInfo *prev;
  PListBoxChildInfo *next;
  gint page_size;
  GSequenceIter *iter;
  gint start_y;
  gint end_y;

  modify_selection_pressed = FALSE;

  if (gtk_get_current_event_state (&state))
    {
      modify_mod_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (list_box),
						      GDK_MODIFIER_INTENT_MODIFY_SELECTION);
      if ((state & modify_mod_mask) == modify_mod_mask)
	modify_selection_pressed = TRUE;
    }

  child = NULL;
  switch (step)
    {
    case GTK_MOVEMENT_BUFFER_ENDS:
      if (count < 0)
	child = p_list_box_get_first_visible (list_box);
      else
	child = p_list_box_get_last_visible (list_box);
      break;
    case GTK_MOVEMENT_DISPLAY_LINES:
      if (priv->cursor_child != NULL)
	{
	  iter = priv->cursor_child->iter;

	  while (count < 0  && iter != NULL)
	    {
	      iter = p_list_box_get_previous_visible (list_box, iter);
	      count = count + 1;
	    }
	  while (count > 0  && iter != NULL)
	    {
	      iter = p_list_box_get_next_visible (list_box, iter);
	      count = count - 1;
	    }

	  if (iter != NULL && !g_sequence_iter_is_end (iter))
	    child = g_sequence_get (iter);
	}
      break;
    case GTK_MOVEMENT_PAGES:
      page_size = 100;
      if (priv->adjustment != NULL)
	page_size = gtk_adjustment_get_page_increment (priv->adjustment);

      if (priv->cursor_child != NULL)
	{
	  start_y = priv->cursor_child->y;
	  end_y = start_y;
	  iter = priv->cursor_child->iter;

	  child = priv->cursor_child;
	  if (count < 0)
	    {
	      /* Up */
	      while (iter != NULL && !g_sequence_iter_is_begin (iter))
		{
		  iter = p_list_box_get_previous_visible (list_box, iter);
		  if (iter == NULL)
		    break;

		  prev = g_sequence_get (iter);
		  if (prev->y < start_y - page_size)
		    break;

		  child = prev;
		}
	    }
	  else
	    {
	      /* Down */
	      while (iter != NULL && !g_sequence_iter_is_end (iter))
		{
		  iter = p_list_box_get_next_visible (list_box, iter);
		  if (g_sequence_iter_is_end (iter))
		    break;

		  next = g_sequence_get (iter);
		  if (next->y > start_y + page_size)
		    break;

		  child = next;
		}
	    }
	  end_y = child->y;
	  if (end_y != start_y && priv->adjustment != NULL)
	    gtk_adjustment_set_value (priv->adjustment,
				      gtk_adjustment_get_value (priv->adjustment) +
				      end_y - start_y);
	}
      break;
    default:
      return;
    }

  if (child == NULL || child == priv->cursor_child)
    {
      GtkDirectionType direction = count < 0 ? GTK_DIR_UP : GTK_DIR_DOWN;

      if (!gtk_widget_keynav_failed (GTK_WIDGET (list_box), direction))
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (list_box));

          if (toplevel)
            gtk_widget_child_focus (toplevel,
                                    direction == GTK_DIR_UP ?
                                    GTK_DIR_TAB_BACKWARD :
                                    GTK_DIR_TAB_FORWARD);

        }

      return;
    }

  p_list_box_update_cursor (list_box, child);
  if (!modify_selection_pressed)
    p_list_box_update_selected (list_box, child);
}
