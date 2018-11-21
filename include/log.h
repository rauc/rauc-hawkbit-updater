#ifndef __LOG_H__
#define __LOG_H__

#include <glib-2.0/glib.h>
#include <syslog.h>
#ifdef WITH_SYSTEMD
#include <systemd/sd-journal.h>
#endif

void setup_logging(const gchar *domain, GLogLevelFlags level, gboolean output_to_systemd);

#endif // __LOG_H__
