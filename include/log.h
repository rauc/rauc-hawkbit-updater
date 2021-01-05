/**
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (C) 2018-2020 Prevas A/S (www.prevas.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
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
