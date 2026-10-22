# SPDX-License-Identifier: LGPL-2.1-only

"""
Tests for soft update behavior and the optional D-Bus update readiness check.

Soft updates (deployment.download="attempt", deployment.update="attempt") differ from forced
updates in that the client may optionally consult an external D-Bus service before proceeding.
The service is configured via `soft_update_check_dbus_service` in the [client] section.
"""

import os
import sys

import pytest
from pexpect import EOF

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


def test_soft_update_unavailable_retry_no_max(hawkbit, config, adjust_config, assign_bundle):
    """
    When max_retries is not set (defaults to 0) and the D-Bus service is unreachable, the
    warning shows the attempt count without an upper bound. The update is skipped; the counter
    never triggers a forced install.
    """
    adjusted = adjust_config({'client': {
        'soft_update_check_dbus_service': DBUS_SERVICE_NAME,
        'soft_update_check_force_on_unavailable': 'false',
    }})
    assign_bundle(params=SOFT_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{adjusted}" -r')

    assert 'Action type: Soft.' in out
    assert 'Soft update permission service unavailable (attempt 1):' in err
    assert 'attempt 1/' not in err  # no "/N" upper bound in the message
    assert 'Forcing update.' not in err
    assert 'Software bundle installed successfully.' not in out
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] not in ('finished', 'error')


def test_soft_update_unavailable_retry_skip_below_max(hawkbit, config, adjust_config,
                                                      assign_bundle):
    """
    When max_retries=3 and the D-Bus service is unreachable on the first poll, the warning
    shows 'attempt 1/3' and the update is skipped. The max has not been reached so no
    forcing occurs.
    """
    adjusted = adjust_config({'client': {
        'soft_update_check_dbus_service': DBUS_SERVICE_NAME,
        'soft_update_check_force_on_unavailable': 'false',
        'soft_update_check_unavailable_max_retries': '3',
    }})
    assign_bundle(params=SOFT_PARAMS)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{adjusted}" -r')

    assert 'Action type: Soft.' in out
    assert 'Soft update permission service unavailable (attempt 1/3):' in err
    assert 'Forcing update.' not in err
    assert 'Software bundle installed successfully.' not in out
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] not in ('finished', 'error')


def test_soft_update_unavailable_retry_force_after_max_retries(hawkbit, config, adjust_config,
                                                                assign_bundle,
                                                                rauc_dbus_install_success):
    """
    When max_retries=2 and the D-Bus service is unreachable for 2 consecutive poll cycles,
    the client forces the update on the 2nd cycle. This prevents a permanently unavailable
    service from blocking updates indefinitely.

    Note: this test runs in daemon mode and waits for 2 poll cycles at the minimum hawkBit
    polling interval (00:00:30), so it takes ~35 seconds to complete.
    """
    adjusted = adjust_config({'client': {
        'soft_update_check_dbus_service': DBUS_SERVICE_NAME,
        'soft_update_check_force_on_unavailable': 'false',
        'soft_update_check_unavailable_max_retries': '2',
    }})
    assign_bundle(params=SOFT_PARAMS)

    proc = run_pexpect(f'rauc-hawkbit-updater -c "{adjusted}"', timeout=90)

    # Poll 1: service unavailable, skip
    proc.expect('Soft update permission service unavailable \\(attempt 1/2\\):')

    # Poll 2: service still unavailable, max retries reached, update forced
    proc.expect('Soft update permission service unavailable for 2 consecutive poll cycles')
    proc.expect('Forcing update\\.')
    proc.expect('Software bundle installed successfully\\.')

    proc.terminate(force=True)
    proc.expect(EOF)

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'


def test_soft_update_unavailable_then_denied(hawkbit, config, adjust_config, assign_bundle):
    """
    Service unavailable on poll 1 (counter=1), then available but denying on poll 2: the
    update is skipped and the action remains open.

    Verifies that the counter resets on recovery (no spurious forcing) and that an explicit
    denial is still honored once the service comes back.

    Note: runs in daemon mode, takes ~35 seconds (one 30s inter-poll sleep).
    """
    adjusted = adjust_config({'client': {
        'soft_update_check_dbus_service': DBUS_SERVICE_NAME,
        'soft_update_check_force_on_unavailable': 'false',
        'soft_update_check_unavailable_max_retries': '3',
    }})
    assign_bundle(params=SOFT_PARAMS)

    proc = run_pexpect(f'rauc-hawkbit-updater -c "{adjusted}"', timeout=90)

    # Poll 1: service not running, counter increments to 1, update skipped
    proc.expect('Soft update permission service unavailable \\(attempt 1/3\\):')

    # Bring up the service between polls — it will deny permission
    dbus_proc = run_pexpect(
        f'{sys.executable} -m soft_update_check_dbus_dummy --denied',
        cwd=os.path.dirname(__file__)
    )
    dbus_proc.expect('Interface published')

    # Poll 2: service reachable, returns False — explicit denial, counter reset to 0
    proc.expect('Soft update permission denied, skipping update\\.')

    proc.terminate(force=True)
    proc.expect(EOF)
    assert dbus_proc.isalive()
    dbus_proc.terminate(force=True)
    dbus_proc.expect(EOF)

    status = hawkbit.get_action_status()
    assert status[0]['type'] not in ('finished', 'error')


def test_soft_update_unavailable_then_permitted(hawkbit, config, adjust_config, assign_bundle,
                                                rauc_dbus_install_success):
    """
    Service unavailable on poll 1 (counter=1), then available and granting permission on
    poll 2: the update is applied.

    Verifies that the counter resets on recovery and the update proceeds normally once the
    service grants permission, without any forced-update path being taken.

    Note: runs in daemon mode, takes ~35 seconds (one 30s inter-poll sleep).
    """
    adjusted = adjust_config({'client': {
        'soft_update_check_dbus_service': DBUS_SERVICE_NAME,
        'soft_update_check_force_on_unavailable': 'false',
        'soft_update_check_unavailable_max_retries': '3',
    }})
    assign_bundle(params=SOFT_PARAMS)

    proc = run_pexpect(f'rauc-hawkbit-updater -c "{adjusted}"', timeout=90)

    # Poll 1: service not running, counter increments to 1, update skipped
    proc.expect('Soft update permission service unavailable \\(attempt 1/3\\):')

    # Bring up the service between polls — it will grant permission
    dbus_proc = run_pexpect(
        f'{sys.executable} -m soft_update_check_dbus_dummy',
        cwd=os.path.dirname(__file__)
    )
    dbus_proc.expect('Interface published')

    # Poll 2: service reachable, returns True — permission granted, install proceeds
    proc.expect('Soft update permission granted\\.')
    proc.expect('Software bundle installed successfully\\.')

    proc.terminate(force=True)
    proc.expect(EOF)
    assert dbus_proc.isalive()
    dbus_proc.terminate(force=True)
    dbus_proc.expect(EOF)

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'
