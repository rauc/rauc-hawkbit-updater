/**
 * SPDX-License-Identifier: LGPL-2.1-only
 * SPDX-FileCopyrightText: 2018-2020 Lasse K. Mikkelsen <lkmi@prevas.dk>, Prevas A/S (www.prevas.com)
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <glib.h>
#ifdef WITH_SYSTEMD
#include <systemd/sd-journal.h>
#endif

/**
 * @brief     Setup Glib log handler
 *
 * @param[in] domain Log domain
 * @param[in] level  Log level
 * @param[in] p_output_to_systemd output to systemd journal
 */
void setup_logging(const gchar *domain, GLogLevelFlags level, gboolean output_to_systemd);

#endif // __LOG_H__
