// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Andy Holmes <andrew.g.r.holmes@gmail.com>

#define G_LOG_DOMAIN "valent-application-plugin"

#include "config.h"

#include <gio/gio.h>

#include "valent-application-plugin.h"
#include "valent-debug.h"
#include "valent-extension.h"


/**
 * ValentApplicationPlugin:
 *
 * An abstract base class for application plugins.
 *
 * #ValentApplicationPlugin is a base class for plugins that operate in the
 * scope of the application. This usually means integrating the application with
 * the host environment (eg. XDG Autostart).
 *
 * ## Implementation Notes
 *
 * Implementations may handle application events by overriding the appropriate
 * virtual function, including [vfunc@Valent.ApplicationPlugin.activate] to
 * handle activation, [vfunc@Valent.ApplicationPlugin.command_line] to handle
 * CLI options, or [vfunc@Valent.ApplicationPlugin.open] to handle files.
 *
 * For plugin preferences see [class@Valent.PreferencesPage].
 *
 * ## `.plugin` File
 *
 * Application plugins have no special fields in the `.plugin` file.
 *
 * Since: 1.0
 */

typedef struct
{
  gpointer  reserved[1];
} ValentApplicationPluginPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ValentApplicationPlugin, valent_application_plugin, VALENT_TYPE_EXTENSION)

/**
 * ValentApplicationPluginClass:
 * @activate: the virtual function pointer for valent_application_plugin_activate()
 * @command_line: the virtual function pointer for valent_application_plugin_command_line()
 * @dbus_register: the virtual function pointer for valent_application_plugin_dbus_register()
 * @dbus_unregister: the virtual function pointer for valent_application_plugin_dbus_unregister()
 * @open: the virtual function pointer for valent_application_plugin_open()
 * @shutdown: the virtual function pointer for valent_application_plugin_shutdown()
 * @startup: the virtual function pointer for valent_application_plugin_startup()
 *
 * The virtual function table for #ValentApplicationPlugin.
 */


/* LCOV_EXCL_START */
static gboolean
valent_application_plugin_real_activate (ValentApplicationPlugin *plugin)
{
  g_assert (VALENT_IS_APPLICATION_PLUGIN (plugin));

  return FALSE;
}

static int
valent_application_plugin_real_command_line (ValentApplicationPlugin *plugin,
                                             GApplicationCommandLine *command_line)
{
  g_assert (VALENT_IS_APPLICATION_PLUGIN (plugin));
  g_assert (G_IS_APPLICATION_COMMAND_LINE (command_line));

  return 0;
}

static gboolean
valent_application_plugin_real_dbus_register (ValentApplicationPlugin  *plugin,
                                              GDBusConnection          *connection,
                                              const char               *object_path,
                                              GError                  **error)
{
  g_assert (VALENT_IS_APPLICATION_PLUGIN (plugin));
  g_assert (G_IS_DBUS_CONNECTION (connection));
  g_assert (g_variant_is_object_path (object_path));
  g_assert (error == NULL || *error == NULL);

  return TRUE;
}

static void
valent_application_plugin_real_dbus_unregister (ValentApplicationPlugin *plugin,
                                                GDBusConnection         *connection,
                                                const char              *object_path)
{
  g_assert (VALENT_IS_APPLICATION_PLUGIN (plugin));
  g_assert (G_IS_DBUS_CONNECTION (connection));
  g_assert (g_variant_is_object_path (object_path));
}

static gboolean
valent_application_plugin_real_open (ValentApplicationPlugin  *plugin,
                                     GFile                   **files,
                                     int                       n_files,
                                     const char               *hint)
{
  g_assert (VALENT_IS_APPLICATION_PLUGIN (plugin));
  g_assert (files != NULL);
  g_assert (n_files > 0);
  g_assert (hint != NULL);

  return FALSE;
}

static void
valent_application_plugin_real_shutdown (ValentApplicationPlugin *plugin)
{
  g_assert (VALENT_IS_APPLICATION_PLUGIN (plugin));
}

static void
valent_application_plugin_real_startup (ValentApplicationPlugin *plugin)
{
  g_assert (VALENT_IS_APPLICATION_PLUGIN (plugin));
}
/* LCOV_EXCL_STOP */

/*
 * GObject
 */
static void
valent_application_plugin_class_init (ValentApplicationPluginClass *klass)
{
  klass->activate = valent_application_plugin_real_activate;
  klass->command_line = valent_application_plugin_real_command_line;
  klass->dbus_register = valent_application_plugin_real_dbus_register;
  klass->dbus_unregister = valent_application_plugin_real_dbus_unregister;
  klass->open = valent_application_plugin_real_open;
  klass->shutdown = valent_application_plugin_real_shutdown;
  klass->startup = valent_application_plugin_real_startup;
}

static void
valent_application_plugin_init (ValentApplicationPlugin *adapter)
{
}

/**
 * valent_application_plugin_activate: (virtual activate)
 * @plugin: a #ValentApplicationPlugin
 *
 * Handle activation of the application.
 *
 * Implementations should override this method to handle activation, as
 * a result of [signal@Gio.Application::activate] being emitted on the primary
 * instance of the application.
 *
 * Returns: %TRUE if handled, or %FALSE if not
 *
 * Since: 1.0
 */
gboolean
valent_application_plugin_activate (ValentApplicationPlugin *plugin)
{
  gboolean ret;

  VALENT_ENTRY;

  g_return_val_if_fail (VALENT_IS_APPLICATION_PLUGIN (plugin), FALSE);

  ret = VALENT_APPLICATION_PLUGIN_GET_CLASS (plugin)->activate (plugin);

  VALENT_RETURN (ret);
}

/**
 * valent_application_plugin_command_line: (virtual command_line)
 * @plugin: a #ValentApplicationPlugin
 * @command_line: a #GApplicationCommandLine
 *
 * Handle the given command-line options.
 *
 * Implementations should override this method to handle command-line options,
 * as a result of [signal@Gio.Application::command-line] being emitted on the
 * primary instance of the application.
 *
 * Returns: an integer that is set as the exit status for the calling process
 *
 * Since: 1.0
 */
int
valent_application_plugin_command_line (ValentApplicationPlugin *plugin,
                                        GApplicationCommandLine *command_line)
{
  int ret;

  VALENT_ENTRY;

  g_return_val_if_fail (VALENT_IS_APPLICATION_PLUGIN (plugin), 1);
  g_return_val_if_fail (G_IS_APPLICATION_COMMAND_LINE (command_line), 1);

  ret = VALENT_APPLICATION_PLUGIN_GET_CLASS (plugin)->command_line (plugin,
                                                                    command_line);

  VALENT_RETURN (ret);
}

/**
 * valent_application_plugin_dbus_register: (virtual dbus_register)
 * @plugin: a #ValentApplicationPlugin
 * @connection: a `Gio.DBusCOnnection`
 * @object_path: a D-Bus object path
 * @error: (nullable): a `GError`
 *
 * Handle the D-Bus registration phase of the application.
 *
 * Implementations may override this method to export extra objects on the
 * bus, that need to exist before the application tries to own the bus name.
 *
 * D-Bus registration will be aborted if %FALSE is returned, so implementations
 * may return %TRUE and report the error by other means if it is not intended
 * to be fatal.
 *
 * Returns: %TRUE, or %FALSE with @error set
 *
 * Since: 1.0
 */
gboolean
valent_application_plugin_dbus_register (ValentApplicationPlugin  *plugin,
                                         GDBusConnection          *connection,
                                         const char               *object_path,
                                         GError                  **error)
{
  gboolean ret;

  VALENT_ENTRY;

  g_return_val_if_fail (VALENT_IS_APPLICATION_PLUGIN (plugin), FALSE);
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (g_variant_is_object_path (object_path), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = VALENT_APPLICATION_PLUGIN_GET_CLASS (plugin)->dbus_register (plugin,
                                                                     connection,
                                                                     object_path,
                                                                     error);

  VALENT_RETURN (ret);
}

/**
 * valent_application_plugin_dbus_unregister: (virtual dbus_unregister)
 * @plugin: a #ValentApplicationPlugin
 * @connection: a `Gio.DBusCOnnection`
 * @object_path: a D
 *
 * Handle the D-Bus unregistration phase of the application.
 *
 * Implementations should override this method to unexport anything exported in
 * [vfunc@Valent.ApplicationPlugin.dbus_register].
 *
 * Since: 1.0
 */
void
valent_application_plugin_dbus_unregister (ValentApplicationPlugin  *plugin,
                                           GDBusConnection          *connection,
                                           const char               *object_path)
{
  VALENT_ENTRY;

  g_return_if_fail (VALENT_IS_APPLICATION_PLUGIN (plugin));
  g_return_if_fail (G_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (g_variant_is_object_path (object_path));

  VALENT_APPLICATION_PLUGIN_GET_CLASS (plugin)->dbus_unregister (plugin,
                                                                 connection,
                                                                 object_path);

  VALENT_EXIT;
}

/**
 * valent_application_plugin_open: (virtual open)
 * @plugin: a #ValentApplicationPlugin
 * @files: (array length=n_files): an array of #GFiles to open
 * @n_files: the length of the @files array
 * @hint: (not nullable): a hint (or "")
 *
 * Open the given files.
 *
 * Implementations should override this method to handle files and URIs, as
 * a result of [signal@Gio.Application::open] being emitted on the primary
 * instance of the application.
 *
 * Returns: %TRUE if handled, or %FALSE if not
 *
 * Since: 1.0
 */
gboolean
valent_application_plugin_open (ValentApplicationPlugin  *plugin,
                                GFile                   **files,
                                int                       n_files,
                                const char               *hint)
{
  gboolean ret;

  VALENT_ENTRY;

  g_return_val_if_fail (VALENT_IS_APPLICATION_PLUGIN (plugin), FALSE);
  g_return_val_if_fail (files != NULL, FALSE);
  g_return_val_if_fail (n_files > 0, FALSE);
  g_return_val_if_fail (hint != NULL, FALSE);

  ret = VALENT_APPLICATION_PLUGIN_GET_CLASS (plugin)->open (plugin,
                                                            files,
                                                            n_files,
                                                            hint);

  VALENT_RETURN (ret);
}

/**
 * valent_application_plugin_shutdown: (virtual shutdown)
 * @plugin: a #ValentApplicationPlugin
 *
 * Handle the shutdown phase of the application.
 *
 * Implementations should override this method to reverse anything done in
 * [vfunc@Valent.ApplicationPlugin.startup].
 *
 * Since: 1.0
 */
void
valent_application_plugin_shutdown (ValentApplicationPlugin *plugin)
{
  VALENT_ENTRY;

  g_return_if_fail (VALENT_IS_APPLICATION_PLUGIN (plugin));

  VALENT_APPLICATION_PLUGIN_GET_CLASS (plugin)->shutdown (plugin);

  VALENT_EXIT;
}

/**
 * valent_application_plugin_startup: (virtual startup)
 * @plugin: a #ValentApplicationPlugin
 *
 * Handle the startup phase of the application.
 *
 * Implementations may override this method to perform setup task that should
 * only happen on the primary instance.
 *
 * Since: 1.0
 */
void
valent_application_plugin_startup (ValentApplicationPlugin *plugin)
{
  VALENT_ENTRY;

  g_return_if_fail (VALENT_IS_APPLICATION_PLUGIN (plugin));

  VALENT_APPLICATION_PLUGIN_GET_CLASS (plugin)->startup (plugin);

  VALENT_EXIT;
}

