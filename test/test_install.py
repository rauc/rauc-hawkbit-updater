# SPDX-License-Identifier: LGPL-2.1-only
# SPDX-FileCopyrightText: 2021-2022 Bastian Krause <bst@pengutronix.de>, Pengutronix

from datetime import datetime, timedelta
from pathlib import Path

from pexpect import TIMEOUT, EOF
import pytest

from helper import run, run_pexpect, timezone_offset_utc
import subprocess 

@pytest.fixture
def install_config(config, adjust_config):
    def _install_config(mode):
        if mode == 'streaming':
            return adjust_config(
                {'client': {'stream_bundle': 'true'}},
                remove={'client': 'bundle_download_location'},
            )
        return config

    return _install_config

@pytest.fixture
def confirm_workflow_hawkbit(hawkbit):
    hawkbit.set_config('user.confirmation.flow.enabled', True)
    yield
    hawkbit.set_config('user.confirmation.flow.enabled', False)

@pytest.mark.parametrize('mode', ('download', 'streaming'))
def test_install_bundle_no_dbus_iface(hawkbit, install_config, bundle_assigned, mode):
    """Assign bundle to target and test installation without RAUC D-Bus interface available."""
    config = install_config(mode)
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    err_lines = err.splitlines()

    assert 'New software ready for download' in out

    if mode == 'download':
        assert 'Download complete' in out

    assert err_lines.pop(0) == \
            'WARNING: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name de.pengutronix.rauc was not provided by any .service files'
    assert err_lines.pop(0) == 'WARNING: Failed to install software bundle.'

    if mode == 'streaming':
        assert err_lines.pop(0) == 'WARNING: Streaming installation failed'

    assert not err_lines
    assert exitcode == 1

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'error'

@pytest.mark.parametrize('mode', ('download', 'streaming'))
def test_install_success(hawkbit, install_config, bundle_assigned, rauc_dbus_install_success, mode):
    """
    Assign bundle to target and test successful download and installation. Make sure installation
    result is received correctly by hawkBit.
    """
    config = install_config(mode)
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'New software ready for download' in out

    if mode == 'download':
        assert 'Download complete' in out

    assert 'Software bundle installed successfully.' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'

@pytest.mark.parametrize('mode', ('download', 'streaming'))
def test_install_failure(hawkbit, install_config, bundle_assigned, rauc_dbus_install_failure, mode):
    """
    Assign bundle to target and test successful download and failing installation. Make sure
    installation result is received correctly by hawkBit.
    """
    config = install_config(mode)
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'New software ready for download' in out
    assert 'WARNING: Failed to install software bundle.' in err
    assert exitcode == 1

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'error'
    assert 'Failed to install software bundle.' in status[0]['messages']

@pytest.mark.parametrize('mode', ('download', 'streaming'))
@pytest.mark.parametrize("confirm", [True])
def test_install_confirmation_given(hawkbit, adjust_config, install_config, mode,
                                    confirm_workflow_hawkbit, bundle_assigned, rauc_dbus_install_success,
                                    install_confirmation_plugin, confirm):
    """
    Enable user confirmation in Hawkbit config. Make sure we can approve requested installation
    and the result of installation is received correctly by hawkBit.
    """
    import re

    if mode == 'streaming':
        config = adjust_config(
            {'client': {'stream_bundle': 'true', 'require_confirmation': 'true'}},
            remove={'client': 'bundle_download_location'})
    else:
        config = adjust_config({'client':{'require_confirmation': 'true'}})

    confirmed_regex = re.compile("Action .* confirmed")

    plugin = install_confirmation_plugin(confirm)
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'New software ready for download' in out
    assert confirmed_regex.findall(out)
    assert 'Software bundle installed successfully.' in out

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'

denied_message = "Denied because we are not interested in the update"
@pytest.mark.parametrize("confirm", [False])
@pytest.mark.parametrize("error_code", [-120])
@pytest.mark.parametrize("details", [denied_message])
def test_install_confirmation_denied(hawkbit, adjust_config, confirm_workflow_hawkbit, bundle_assigned,
                                     install_confirmation_plugin, confirm, error_code, details):
    """
    Enable user confirmation in Hawkbit config. Deny requested installation
    and check that hawkBit received the feedback.
    """
    import re

    config = adjust_config({'client':{'require_confirmation': 'true'}})
    confirmed_regex = re.compile("Action .* denied")

    plugin = install_confirmation_plugin(confirm, error_code, details)
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert confirmed_regex.findall(out)
    # make sure the response came from the plugin
    assert 'GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown' not in out

    status = hawkbit.get_action_status()
    # If the confirmation is rejected, the action continues to be in state WAITING_FOR_CONFIRMATION
    # until it is confirmed at a later point in time or cancelled.
    assert status[0]['type'] == 'wait_for_confirmation'
    assert f'Confirmation status code: {error_code}' in status[0]['messages']
    assert denied_message in status[0]['messages']

def test_install_confirmation_no_plugin(hawkbit, adjust_config, confirm_workflow_hawkbit, bundle_assigned):
    """
    Enable user confirmation in Hawkbit config.
    Don't start confirmation plugin. The expected result should be a timeout,
    we don't want to force any default on a user
    """
    import re
    config = adjust_config({'client':{'require_confirmation': 'true'}})

    try:
        out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r', timeout=15)
        assert False
    except subprocess.TimeoutExpired as e:
        # We expect timeout instead of "denied" response in case there's no plugin to handle the request
        err = e.stderr
        assert b'GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown' in err
        assert True

    status = hawkbit.get_action_status()
    # If the confirmation is rejected, the action continues to be in state WAITING_FOR_CONFIRMATION
    # until it is confirmed at a later point in time or cancelled.
    assert status[0]['type'] == 'wait_for_confirmation'

@pytest.mark.parametrize('mode', ('download', 'streaming'))
def test_install_maintenance_window(hawkbit, install_config, rauc_bundle, assign_bundle,
                                    rauc_dbus_install_success, mode):
    bundle_size = Path(rauc_bundle).stat().st_size
    maintenance_start = datetime.now() + timedelta(seconds=15)
    maintenance_window = {
        'maintenanceWindow': {
            'schedule' : maintenance_start.strftime('%-S %-M %-H ? %-m * %-Y'),
            'timezone' : timezone_offset_utc(maintenance_start),
            'duration' : '00:01:00'
        }
    }
    assign_bundle(params=maintenance_window)

    config = install_config(mode)
    proc = run_pexpect(f'rauc-hawkbit-updater -c "{config}"')
    proc.expect(r"hawkBit requested to skip installation, not invoking RAUC yet \(maintenance window is 'unavailable'\)")

    if mode == 'download':
        proc.expect('Start downloading')
        proc.expect('Download complete')
        proc.expect('File checksum OK')

    # wait for the maintenance window to become available and the next poll of the base resource
    proc.expect(TIMEOUT, timeout=30)
    proc.expect(r"Continuing scheduled deployment .* \(maintenance window is 'available'\)")
    # RAUC bundle should have been already downloaded completely
    if mode == 'download':
        proc.expect(f'Resuming download from offset {bundle_size}')
        proc.expect('Download complete')
        proc.expect('File checksum OK')

    proc.expect('Software bundle installed successfully')

    # let feedback propagate to hawkBit before termination
    proc.expect(TIMEOUT, timeout=2)
    proc.terminate(force=True)
    proc.expect(EOF)

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'
