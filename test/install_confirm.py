#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-only
# SPDX-FileCopyrightText: 2023 Vyacheslav Yurkov <uvv.mail@gmail.com>

from pathlib import Path

from gi.repository import GLib
from pydbus.generic import signal

class InstallConfirmation:
    """
    D-Bus interface `de.pengutronix.rauc.InstallConfirmation`, to be used with
    `pydbus.{SessionBus,SystemBus}.publish()`

    The interface is defined via the xml read in the dbus class property.
    The D-Bus methods are Python methods.

    See https://github.com/LEW21/pydbus/blob/master/doc/tutorial.rst#class-preparation
    """
    dbus = Path('../src/rauc-install-confirmation.xml').read_text()
    interface = 'de.pengutronix.rauc.InstallConfirmation'

    ConfirmationStatus = signal()

    def __init__(self, confirmation=False, error_code=0, details=''):
        self._confirmation_result = confirmation
        self._expected_error_code = error_code
        self._details = details

    def ConfirmInstallationRequest(self, action_id, version):
        self._action_id = action_id
        def confirm_installation():
            """Reports confirmation status back to the caller."""

            self.ConfirmationStatus(self._action_id, self._confirmation_result,
                self._expected_error_code, self._details)

            # do not call again
            return False

        print(f'Confirmation requested for version {version}')

        GLib.timeout_add_seconds(1, confirm_installation)

if __name__ == '__main__':
    import argparse
    from pydbus import SessionBus

    parser = argparse.ArgumentParser()
    parser.add_argument('--confirmed', action='store_true',
                        help='Confirmation status to return with D-Bus Confrimed signal')
    parser.add_argument('--denied', dest='confirmed', action='store_false',
                        help='Confirmation status to return with D-Bus Confrimed signal')
    parser.add_argument('--details', default='',
                        help='Details string to return with D-Bus Confrimed signal')
    parser.add_argument('--error-code', type=int, default=0,
                        help='Code to emit as D-Bus Confirmed signal')
    parser.set_defaults(confirmed=False)
    args = parser.parse_args()

    loop = GLib.MainLoop()
    bus = SessionBus()
    confirmation_plugin = InstallConfirmation(args.confirmed, args.error_code, args.details)
    with bus.publish('de.pengutronix.rauc.InstallConfirmation', ('/', confirmation_plugin)):
        print('Confirmation interface published')
        loop.run()
