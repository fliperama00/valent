// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <gio/gio.h>
#include <valent.h>
#include <libvalent-test.h>

#define DEVICE_PATH "/org/freedesktop/UPower/devices/DisplayDevice"


static void
upower_set_battery (GDBusConnection *connection,
                    const char      *name,
                    GVariant        *value)
{
  GVariantDict dict;
  GVariant *args;

  g_variant_dict_init (&dict, NULL);
  g_variant_dict_insert_value (&dict, name, value);
  args = g_variant_new ("(o@a{sv})", DEVICE_PATH, g_variant_dict_end (&dict));

  g_dbus_connection_call (connection,
                          "org.freedesktop.UPower",
                          "/org/freedesktop/UPower",
                          "org.freedesktop.DBus.Mock",
                          "SetDeviceProperties",
                          args,
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          NULL,
                          NULL);
}

static void
test_battery_plugin_actions (ValentTestFixture *fixture,
                             gconstpointer      user_data)
{
  GActionGroup *actions = G_ACTION_GROUP (fixture->device);
  JsonNode *packet;
  g_autoptr (GVariant) state = NULL;
  gboolean charging;
  double percentage;
  const char *icon_name;
  gboolean is_present;
  int64_t time_to_empty = 0;
  int64_t time_to_full = 0;

  VALENT_TEST_CHECK ("Plugin has expected actions");
  g_assert_true (g_action_group_has_action (actions, "battery.state"));

  VALENT_TEST_CHECK ("Plugin sends battery status at connect");
  valent_test_fixture_connect (fixture, TRUE);

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.battery");
  v_assert_packet_cmpint (packet, "currentCharge", ==, 100);
  v_assert_packet_false (packet, "isCharging");
  v_assert_packet_cmpint (packet, "thresholdEvent", ==, 0);
  json_node_unref (packet);

  VALENT_TEST_CHECK ("Plugin action `battery.state` is enabled when connected, "
                     "but only if reported as present.");
  g_assert_false (g_action_group_get_action_enabled (actions, "battery.state"));

  state = g_action_group_get_action_state (actions, "battery.state");

  VALENT_TEST_CHECK ("Plugin action `battery.state` has expected signature");
  g_assert_true (g_variant_is_of_type (state, G_VARIANT_TYPE_VARDICT));
  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  VALENT_TEST_CHECK ("Plugin action `battery.state` has expected value");
  g_assert_false (charging);
  g_assert_cmpfloat (percentage, <=, 0.0);
  g_assert_cmpstr (icon_name, ==, "battery-missing-symbolic");
  g_assert_false (is_present);
  g_assert_cmpint (time_to_empty, ==, 0);
  g_assert_cmpint (time_to_full, ==, 0);
}

static void
test_battery_plugin_handle_update (ValentTestFixture *fixture,
                                   gconstpointer      user_data)
{
  GActionGroup *actions = G_ACTION_GROUP (fixture->device);
  JsonNode *packet;
  GVariant *state;
  gboolean charging;
  double percentage;
  const char *icon_name;
  gboolean is_present;
  int64_t time_to_empty;
  int64_t time_to_full;
  gboolean watch = FALSE;

  valent_test_watch_signal (actions,
                            "action-state-changed::battery.state",
                            &watch);

  VALENT_TEST_CHECK ("Plugin action `battery.state` starts disabled");
  g_assert_false (g_action_group_get_action_enabled (actions, "battery.state"));

  VALENT_TEST_CHECK ("Plugin sends battery status at connect");
  valent_test_fixture_connect (fixture, TRUE);

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.battery");
  v_assert_packet_cmpint (packet, "currentCharge", ==, 100);
  v_assert_packet_false (packet, "isCharging");
  v_assert_packet_cmpint (packet, "thresholdEvent", ==, 0);
  json_node_unref (packet);

  VALENT_TEST_CHECK ("Plugin handles \"empty\" battery update");
  packet = valent_test_fixture_lookup_packet (fixture, "empty-battery");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_await_boolean (&watch);

  VALENT_TEST_CHECK ("Plugin action `battery.state` is enabled if a status "
                     "packet is received");
  g_assert_true (g_action_group_get_action_enabled (actions, "battery.state"));

  VALENT_TEST_CHECK ("Plugin updates `battery.state` action to expected value");
  state = g_action_group_get_action_state (actions, "battery.state");

  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  g_assert_true (charging);
  g_assert_cmpfloat (percentage, <=, 0.0);
  g_assert_cmpstr (icon_name, ==, "battery-empty-charging-symbolic");
  g_assert_true (is_present);
  g_assert_cmpint (time_to_empty, ==, 0);
  g_assert_cmpint (time_to_full, >, 0);

  g_clear_pointer (&state, g_variant_unref);

  VALENT_TEST_CHECK ("Plugin handles \"caution\" battery update");
  packet = valent_test_fixture_lookup_packet (fixture, "caution-battery");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_await_boolean (&watch);

  VALENT_TEST_CHECK ("Plugin updates `battery.state` action to expected value");
  state = g_action_group_get_action_state (actions, "battery.state");

  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  g_assert_true (charging);
  g_assert_cmpfloat (percentage, >=, 15.0);
  g_assert_cmpstr (icon_name, ==, "battery-caution-charging-symbolic");
  g_assert_true (is_present);
  g_assert_cmpint (time_to_empty, ==, 0);
  g_assert_cmpint (time_to_full, >, 0);

  g_clear_pointer (&state, g_variant_unref);

  VALENT_TEST_CHECK ("Plugin handles \"low\" battery update");
  packet = valent_test_fixture_lookup_packet (fixture, "low-battery");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_await_boolean (&watch);

  VALENT_TEST_CHECK ("Plugin updates `battery.state` action to expected value");
  state = g_action_group_get_action_state (actions, "battery.state");

  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  g_assert_true (charging);
  g_assert_cmpfloat (percentage, >=, 25.0);
  g_assert_cmpstr (icon_name, ==, "battery-low-charging-symbolic");
  g_assert_true (is_present);
  g_assert_cmpint (time_to_empty, ==, 0);
  g_assert_cmpint (time_to_full, >, 0);

  g_clear_pointer (&state, g_variant_unref);

  VALENT_TEST_CHECK ("Plugin handles \"good\" battery update");
  packet = valent_test_fixture_lookup_packet (fixture, "good-battery");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_await_boolean (&watch);

  VALENT_TEST_CHECK ("Plugin updates `battery.state` action to expected value");
  state = g_action_group_get_action_state (actions, "battery.state");

  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  g_assert_false (charging);
  g_assert_cmpfloat (percentage, >=, 55.0);
  g_assert_cmpstr (icon_name, ==, "battery-good-symbolic");
  g_assert_true (is_present);
  g_assert_cmpint (time_to_empty, >, 0);
  g_assert_cmpint (time_to_full, ==, 0);

  g_clear_pointer (&state, g_variant_unref);

  VALENT_TEST_CHECK ("Plugin handles \"full\" battery update");
  packet = valent_test_fixture_lookup_packet (fixture, "full-battery");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_await_boolean (&watch);

  VALENT_TEST_CHECK ("Plugin updates `battery.state` action to expected value");
  state = g_action_group_get_action_state (actions, "battery.state");

  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  g_assert_false (charging);
  g_assert_cmpfloat (percentage, >=, 65.0);
  g_assert_cmpstr (icon_name, ==, "battery-full-symbolic");
  g_assert_true (is_present);
  g_assert_cmpint (time_to_empty, >, 0);
  g_assert_cmpint (time_to_full, ==, 0);

  g_clear_pointer (&state, g_variant_unref);

  VALENT_TEST_CHECK ("Plugin handles \"charged\" battery update");
  packet = valent_test_fixture_lookup_packet (fixture, "charged-battery");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_await_boolean (&watch);

  VALENT_TEST_CHECK ("Plugin updates `battery.state` action to expected value");
  state = g_action_group_get_action_state (actions, "battery.state");

  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  g_assert_false (charging);
  g_assert_cmpfloat (percentage, >=, 100.0);
  g_assert_cmpstr (icon_name, ==, "battery-full-charged-symbolic");
  g_assert_true (is_present);
  g_assert_cmpint (time_to_empty, >, 0);
  g_assert_cmpint (time_to_full, ==, 0);

  g_clear_pointer (&state, g_variant_unref);

  VALENT_TEST_CHECK ("Plugin handles \"missing\" battery update");
  packet = valent_test_fixture_lookup_packet (fixture, "missing-battery");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_await_boolean (&watch);

  VALENT_TEST_CHECK ("Plugin updates `battery.state` action to expected value");
  state = g_action_group_get_action_state (actions, "battery.state");

  g_assert_true (g_variant_lookup (state, "charging", "b", &charging));
  g_assert_true (g_variant_lookup (state, "percentage", "d", &percentage));
  g_assert_true (g_variant_lookup (state, "icon-name", "&s", &icon_name));
  g_assert_true (g_variant_lookup (state, "is-present", "b", &is_present));
  g_assert_true (g_variant_lookup (state, "time-to-empty", "x", &time_to_empty));
  g_assert_true (g_variant_lookup (state, "time-to-full", "x", &time_to_full));

  g_assert_false (charging);
  g_assert_cmpfloat (percentage, <=, 15.0);
  g_assert_cmpstr (icon_name, ==, "battery-missing-symbolic");
  g_assert_false (is_present);

  g_clear_pointer (&state, g_variant_unref);

  valent_test_watch_clear (actions, &watch);
}

static void
test_battery_plugin_send_update (ValentTestFixture *fixture,
                                 gconstpointer      user_data)
{
  g_autoptr (GDBusConnection) connection = NULL;
  JsonNode *packet;

  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, NULL);

  VALENT_TEST_CHECK ("Plugin sends battery status at connect");
  valent_test_fixture_connect (fixture, TRUE);

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.battery");
  v_assert_packet_cmpint (packet, "currentCharge", ==, 100);
  v_assert_packet_false (packet, "isCharging");
  v_assert_packet_cmpint (packet, "thresholdEvent", ==, 0);
  json_node_unref (packet);

  VALENT_TEST_CHECK ("Plugin sends battery level updates");
  upower_set_battery (connection, "Percentage", g_variant_new_double (42.0));

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.battery");
  v_assert_packet_cmpint (packet, "currentCharge", ==, 42);
  v_assert_packet_false (packet, "isCharging");
  v_assert_packet_cmpint (packet, "thresholdEvent", ==, 0);
  json_node_unref (packet);

  VALENT_TEST_CHECK ("Plugin sends battery charging updates");
  upower_set_battery (connection, "State", g_variant_new_uint32 (1));

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.battery");
  v_assert_packet_cmpint (packet, "currentCharge", ==, 42);
  v_assert_packet_true (packet, "isCharging");
  v_assert_packet_cmpint (packet, "thresholdEvent", ==, 0);
  json_node_unref (packet);

  VALENT_TEST_CHECK ("Plugin sends battery threshold updates");
  upower_set_battery (connection, "WarningLevel", g_variant_new_uint32 (3));

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.battery");
  v_assert_packet_cmpint (packet, "currentCharge", ==, 42);
  v_assert_packet_true (packet, "isCharging");
  v_assert_packet_cmpint (packet, "thresholdEvent", ==, 1);
  json_node_unref (packet);
}

static const char *schemas[] = {
  "/tests/kdeconnect.battery.json",
  "/tests/kdeconnect.battery.request.json",
};

static void
test_battery_plugin_fuzz (ValentTestFixture *fixture,
                          gconstpointer      user_data)

{
  valent_test_fixture_connect (fixture, TRUE);
  g_test_log_set_fatal_handler (valent_test_mute_fuzzing, NULL);

  for (unsigned int s = 0; s < G_N_ELEMENTS (schemas); s++)
    valent_test_fixture_schema_fuzz (fixture, schemas[s]);
}

int
main (int   argc,
      char *argv[])
{
  const char *path = "plugin-battery.json";

  valent_test_init (&argc, &argv, NULL);

  g_test_add ("/plugins/battery/actions",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_battery_plugin_actions,
              valent_test_fixture_clear);

  g_test_add ("/plugins/battery/handle-update",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_battery_plugin_handle_update,
              valent_test_fixture_clear);

  g_test_add ("/plugins/battery/send-update",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_battery_plugin_send_update,
              valent_test_fixture_clear);

  g_test_add ("/plugins/battery/fuzz",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_battery_plugin_fuzz,
              valent_test_fixture_clear);

  return g_test_run ();
}
