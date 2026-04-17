# SPDX-License-Identifier: LGPL-2.1-only

"""
Tests for soft update behavior and the optional D-Bus update readiness check.

Soft updates (deployment.download="attempt", deployment.update="attempt") differ from forced
updates in that the client may optionally consult an external D-Bus service before proceeding.
The service is configured via `soft_update_check_dbus_service` in the [client] section.
"""

import pytest

from helper import run, run_pexpect
from soft_update_check_dbus_dummy import DBUS_SERVICE_NAME


SOFT_PARAMS = {'type': 'soft'}
FORCED_PARAMS = {'type': 'forced'}


@pytest.fixture
def soft_update_config(config, adjust_config):
    """Adjusts the base config to add the soft update check D-Bus service name."""
    return adjust_config({'client': {'soft_update_check_dbus_service': DBUS_SERVICE_NAME}})


def test_soft_update_no_check_configured(hawkbit, config, assign_bundle,
                                         rauc_dbus_install_success):
    """
    Soft update without soft_update_check_dbus_service configured: update proceeds
    unconditionally, same as a forced update.
    """
    assign_bundle(params=SOFT_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'Action type: Soft.' in out
    assert 'Software bundle installed successfully.' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'


def test_soft_update_permitted(hawkbit, soft_update_config, assign_bundle,
                               rauc_dbus_install_success, soft_update_check_permitted):
    """
    Soft update with readiness check configured and service returning True: update proceeds
    after permission is granted.
    """
    assign_bundle(params=SOFT_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{soft_update_config}" -r')

    assert 'Action type: Soft.' in out
    assert 'Soft update permission granted.' in out
    assert 'Software bundle installed successfully.' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'


def test_soft_update_denied(hawkbit, soft_update_config, assign_bundle,
                            soft_update_check_denied):
    """
    Soft update with readiness check configured and service returning False: update is skipped
    without error and hawkBit action remains open.
    """
    assign_bundle(params=SOFT_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{soft_update_config}" -r')

    assert 'Action type: Soft.' in out
    assert 'Soft update permission denied, skipping update.' in out
    assert 'Software bundle installed successfully.' not in out
    assert err == ''
    assert exitcode == 0

    # action must remain open (not finished/error) since the update was skipped
    status = hawkbit.get_action_status()
    assert status[0]['type'] not in ('finished', 'error')


def test_soft_update_no_dbus_service(hawkbit, soft_update_config, assign_bundle,
                                     rauc_dbus_install_success):
    """
    Soft update with readiness check configured but the D-Bus service not running: update is
    forced through by default (soft_update_check_force_on_unavailable defaults to true).
    """
    assign_bundle(params=SOFT_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{soft_update_config}" -r')

    assert 'Action type: Soft.' in out
    assert 'Soft update permission service unavailable:' in err
    assert 'Forcing update.' in err
    assert 'Software bundle installed successfully.' in out
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'


def test_soft_update_no_dbus_service_skip_on_unavailable(hawkbit, config, adjust_config,
                                                         assign_bundle):
    """
    Soft update with readiness check configured, service not running, and
    soft_update_check_force_on_unavailable=false: update is skipped and hawkBit action remains
    open.
    """
    adjusted = adjust_config({'client': {
        'soft_update_check_dbus_service': DBUS_SERVICE_NAME,
        'soft_update_check_force_on_unavailable': 'false',
    }})
    assign_bundle(params=SOFT_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{adjusted}" -r')

    assert 'Action type: Soft.' in out
    assert 'Soft update permission check failed:' in err
    assert 'Software bundle installed successfully.' not in out
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] not in ('finished', 'error')


def test_forced_update_bypasses_check(hawkbit, soft_update_config, assign_bundle,
                                      rauc_dbus_install_success, soft_update_check_denied):
    """
    Forced update with readiness check configured and service returning False: the check is
    not consulted for forced updates, so the update proceeds unconditionally.
    """
    assign_bundle(params=FORCED_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{soft_update_config}" -r')

    assert 'Action type: Forced.' in out
    assert 'Soft update permission' not in out
    assert 'Software bundle installed successfully.' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'
