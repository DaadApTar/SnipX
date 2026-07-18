#include "notification.h"
#include "log.h"

DBusError dbus_error;
DBusConnection *dbus_connection;

int dbus_conn_new(logger *logger) {
  dbus_error_init(&dbus_error);

  dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

  if (dbus_error_is_set(&dbus_error)) {
    log_print(logger, LOG_WARNING, "DBus connection error: %s\n", dbus_error.message);
    dbus_error_free(&dbus_error);
    return -1;
  }

  return 0;
}

DBusMessage *notification_new(const char *app_name, const char *title, const char *body, const char *icon) {
  DBusMessage *msg = dbus_message_new_method_call(
    "org.freedesktop.Notifications",
    "/org/freedesktop/Notifications",
    "org.freedesktop.Notifications",
    "Notify"
    );

  unsigned int replaces_id = 0;
  int timeout = 5000;

  DBusMessageIter iter, array_iter, dict_iter;

  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &app_name);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &replaces_id);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &icon);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &title);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &body);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &array_iter);
  dbus_message_iter_close_container(&iter, &array_iter);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
  dbus_message_iter_close_container(&iter, &dict_iter);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &timeout);

  return msg;
}

int notification_show(logger *logger, DBusMessage *msg) {
  if (!dbus_connection) {
    return -1;
  }
  DBusMessage *reply = dbus_connection_send_with_reply_and_block(dbus_connection, msg, -1, &dbus_error);

  if (dbus_error_is_set(&dbus_error)) {
    log_print(logger, LOG_WARNING, "Notify error: %s\n", dbus_error.message);
    dbus_error_free(&dbus_error);
    return -1;
  }

  if (reply) dbus_message_unref(reply);

  return 0;
}

void notification_unref(DBusMessage *msg) {
  dbus_message_unref(msg);
}

void send_notification(logger *logger, const char *body) {
  DBusMessage *msg = notification_new("SnipX", "SnipX", body, "");
  notification_show(logger, msg);
  notification_unref(msg);
}
