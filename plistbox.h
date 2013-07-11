#ifndef __P_LIST_BOX_H__
#define __P_LIST_BOX_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define P_TYPE_LIST_BOX (p_list_box_get_type ())
#define P_LIST_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), P_TYPE_LIST_BOX, PListBox))
#define P_LIST_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), P_TYPE_LIST_BOX, PListBoxClass))
#define P_IS_LIST_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), P_TYPE_LIST_BOX))
#define P_IS_LIST_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), P_TYPE_LIST_BOX))
#define P_LIST_BOX_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), P_TYPE_LIST_BOX, PListBoxClass))

typedef struct _PListBox PListBox;
typedef struct _PListBoxClass PListBoxClass;
typedef struct _PListBoxPrivate PListBoxPrivate;

struct _PListBox
{
  GtkContainer parent_instance;
  PListBoxPrivate * priv;
};

struct _PListBoxClass
{
  GtkContainerClass parent_class;
  void (*child_selected) (PListBox* self, GtkWidget* child);
  void (*child_activated) (PListBox* self, GtkWidget* child);
  void (*activate_cursor_child) (PListBox* self);
  void (*toggle_cursor_child) (PListBox* self);
  void (*move_cursor) (PListBox* self, GtkMovementStep step, gint count);
  void (*refilter) (PListBox* self);
};

typedef gboolean (*PListBoxFilterFunc) (GtkWidget* child, void* user_data);
typedef gint (*PListBoxSortFunc) (GtkWidget* child1, GtkWidget* child2, void* user_data);
typedef void (*PListBoxUpdateSeparatorFunc) (GtkWidget** separator, GtkWidget* child, GtkWidget* before, void* user_data);

GType p_list_box_get_type (void) G_GNUC_CONST;
GtkWidget*  p_list_box_get_selected_child           (PListBox                    *self);
GtkWidget*  p_list_box_get_child_at_y               (PListBox                    *self,
						       gint                           y);
void        p_list_box_select_child                 (PListBox                    *self,
						       GtkWidget                     *child);
void        p_list_box_set_adjustment               (PListBox                    *self,
						       GtkAdjustment                 *adjustment);
void        p_list_box_add_to_scrolled              (PListBox                    *self,
						       GtkScrolledWindow             *scrolled);
void        p_list_box_set_selection_mode           (PListBox                    *self,
						       GtkSelectionMode               mode);
void        p_list_box_set_filter_func              (PListBox                    *self,
						       PListBoxFilterFunc           f,
						       void                          *f_target,
						       GDestroyNotify                 f_target_destroy_notify);
void        p_list_box_set_separator_funcs          (PListBox                    *self,
						       PListBoxUpdateSeparatorFunc  update_separator,
						       void                          *update_separator_target,
						       GDestroyNotify                 update_separator_target_destroy_notify);
void        p_list_box_refilter                     (PListBox                    *self);
void        p_list_box_resort                       (PListBox                    *self);
void        p_list_box_reseparate                   (PListBox                    *self);
void        p_list_box_set_sort_func                (PListBox                    *self,
						       PListBoxSortFunc             f,
						       void                          *f_target,
						       GDestroyNotify                 f_target_destroy_notify);
void        p_list_box_child_changed                (PListBox                    *self,
						       GtkWidget                     *widget);
void        p_list_box_set_activate_on_single_click (PListBox                    *self,
						       gboolean                       single);
void        p_list_box_drag_unhighlight_widget      (PListBox                    *self);
void        p_list_box_drag_highlight_widget        (PListBox                    *self,
						       GtkWidget                     *widget);
GtkWidget * p_list_box_new                          (void);

G_END_DECLS

#endif
