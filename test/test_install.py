# SPDX-License-Identifier: LGPL-2.1-only
# SPDX-FileCopyrightText: 2021-2022 Bastian Krause <bst@pengutronix.de>, Pengutronix

from datetime import datetime, timedelta
from pathlib import Path

from pexpect import TIMEOUT, EOF
import pytest

from helper import run, run_pexpect, timezone_offset_utc

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
