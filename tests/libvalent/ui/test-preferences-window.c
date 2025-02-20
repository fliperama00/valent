// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <valent.h>
#include <libvalent-test.h>

#define VALENT_TYPE_TEST_SUBJECT (g_type_from_name ("ValentPreferencesWindow"))


static void
test_preferences_window_basic (void)
{
  GtkWindow *window;
  PeasEngine *engine;
  PeasPluginInfo *info;

  window = g_object_new (VALENT_TYPE_TEST_SUBJECT,
                        NULL);
  g_object_add_weak_pointer (G_OBJECT (window), (gpointer)&window);

  gtk_window_present (window);
  valent_test_await_pending ();

  /* Unload/Load the plugin */
  engine = valent_get_plugin_engine ();
  info = peas_engine_get_plugin_info (engine, "mock");
  peas_engine_unload_plugin (engine, info);
  peas_engine_load_plugin (engine, info);

  gtk_window_destroy (window);
  valent_test_await_nullptr (&window);
}

static void
test_preferences_window_navigation (void)
{
  GtkWindow *window;

  window = g_object_new (VALENT_TYPE_TEST_SUBJECT,
                        NULL);
  g_object_add_weak_pointer (G_OBJECT (window), (gpointer)&window);

  gtk_window_present (window);
  valent_test_await_pending ();

  /* Main -> Plugin */
  gtk_widget_activate_action (GTK_WIDGET (window), "win.page", "s", "mock");

  /* Close */
  gtk_widget_activate_action (GTK_WIDGET (window), "window.close", NULL);

  g_assert_null (window);
}

static void
test_preferences_window_rename (void)
{
  GtkWindow *window;

  window = g_object_new (VALENT_TYPE_TEST_SUBJECT,
                        NULL);
  g_object_add_weak_pointer (G_OBJECT (window), (gpointer)&window);

  gtk_window_present (window);
  valent_test_await_pending ();

  /* Rename Dialog */
  gtk_widget_activate_action (GTK_WIDGET (window), "win.rename", NULL);
  valent_test_await_pending ();

  gtk_window_destroy (window);
  valent_test_await_nullptr (&window);
}

int
main (int   argc,
      char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_test_add_func ("/libvalent/ui/preferences-window",
                   test_preferences_window_basic);

  g_test_add_func ("/libvalent/ui/preferences-window/navigation",
                   test_preferences_window_navigation);

  g_test_add_func ("/libvalent/ui/preferences-window/rename",
                   test_preferences_window_rename);

  return g_test_run ();
}

