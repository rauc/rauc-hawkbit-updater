#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-only

"""
D-Bus dummy service for soft update permission checks.

Publishes a de.example.SoftUpdateCheck service on the system bus
exposing an IsReadyForUpdate() method that returns a configurable boolean.
"""

DBUS_SERVICE_NAME = 'de.example.SoftUpdateCheck'


class SoftUpdateCheck:
    """
    D-Bus interface `de.example.SoftUpdateCheck.conf`, to be used with
    `pydbus.SystemBus.publish()`
    """
    dbus = f"""
    <node>
      <interface name='{DBUS_SERVICE_NAME}'>
        <method name='IsReadyForUpdate'>
          <arg type='b' name='ready' direction='out'/>
        </method>
      </interface>
    </node>
    """

    def __init__(self, permitted=True):
        self._permitted = permitted

    def IsReadyForUpdate(self):
        result = 'granted' if self._permitted else 'denied'
        print(f'IsReadyForUpdate() called, returning {result}')
        return self._permitted


if __name__ == '__main__':
    import argparse
    from gi.repository import GLib
    from pydbus import SystemBus

    parser = argparse.ArgumentParser()
    parser.add_argument('--denied', action='store_true',
                        help='Return False from IsReadyForUpdate() (deny permission)')
    args = parser.parse_args()

    loop = GLib.MainLoop()
    bus = SystemBus()
    service = SoftUpdateCheck(permitted=not args.denied)
    with bus.publish(DBUS_SERVICE_NAME, ('/', service)):
        print('Interface published')
        loop.run()
