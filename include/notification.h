#ifndef NOTIFICATION_H_
#define NOTIFICATION_H_

#include <dbus-1.0/dbus/dbus.h>
#include "log.h"

/** @brief initialises DBus connection.
 *  @param[in] logger logger
 *  @return 0 on success, -1 on error
 */
int dbus_conn_new(logger *logger);

/** @brief initialises DBusMessage for notification.
 *  @param[in] app_name app_name
 *  @param[in] title notification title
 *  @param[in] body notification body
 *  @param[in] icon notification icon
 *  @return pointer to msg
 */
DBusMessage *notification_new(const char *app_name, const char *title, const char *body, const char *icon);

/** @brief shows notification.
 *  @param logger logger
 *  @param msg notification
 *  @return 0 on success, -1 on error
 */
int notification_show(logger *logger, DBusMessage *msg);

/** @brief unreferences notification.
 *  @param msg notification
 */
void notification_unref(DBusMessage *msg);

/** @brief prepares and sends notification.
 *  @param[in] logger logger
 *  @param[in] notification body
 */
void send_notification(logger *logger, const char *body);

#endif
