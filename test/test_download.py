# SPDX-License-Identifier: LGPL-2.1-only
# SPDX-FileCopyrightText: 2021 Bastian Krause <bst@pengutronix.de>, Pengutronix

import re

from helper import run

def test_download_inexistent_location(hawkbit, bundle_assigned, adjust_config):
    """
    Assign bundle to target and test download to an inexistent location specified in config.
    """
    location = '/tmp/does_not_exist/foo'
    config = adjust_config(
        {'client': {'bundle_download_location': location}}
    )
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'New software ready for download' in out
    # same warning from feedback() and from hawkbit_pull_cb()
    assert err == \
            f'WARNING: Failed to calculate free space for {location}: No such file or directory\n'*2
    assert exitcode == 1

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'error'
    assert f'Failed to calculate free space for {location}: No such file or directory' in \
            status[0]['messages']

def test_download_unallowed_location(hawkbit, bundle_assigned, adjust_config):
    """
    Assign bundle to target and test download to an unallowed location specified in config.
    """
    location = '/root/foo'
    config = adjust_config(
        {'client': {'bundle_download_location': location}}
    )
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'Start downloading' in out
    assert err.strip() == \
            f'WARNING: Download failed: Failed to open {location} for download: Permission denied'
    assert exitcode == 1

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'error'
    assert f'Download failed: Failed to open {location} for download: Permission denied' in \
            status[0]['messages']

def test_download_too_slow(hawkbit, bundle_assigned, adjust_config, rate_limited_port):
    """Assign bundle to target and test too slow download of bundle."""
    # limit to 50 bytes/s
    port = rate_limited_port(50)
    config = adjust_config({
        'client': {
            'hawkbit_server': f'{hawkbit.host}:{port}',
            'low_speed_time': '3',
            'low_speed_rate': '100',
        }
    })

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r', timeout=90)

    assert 'Start downloading: ' in out
    assert err.strip() == 'WARNING: Download failed: Timeout was reached'
    assert exitcode == 1

def test_download_partials_without_resume(hawkbit, bundle_assigned, adjust_config,
                                          partial_download_port):
    """
    Assign bundle to target and test download of partial bundle parts without having
    download resuming configured.
    """
    config = adjust_config(
        {'client': {'hawkbit_server': f'{hawkbit.host}:{partial_download_port}'}}
    )

    # ignore failing installation
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'Start downloading: ' in out
    assert err.strip() == 'WARNING: Download failed: Transferred a partial file'
    assert exitcode == 1

def test_download_partials_with_resume(hawkbit, bundle_assigned, adjust_config,
                                       partial_download_port):
    """
    Assign bundle to target and test download of partial bundle parts with download resuming
    configured.
    """
    config = adjust_config({
        'client': {
            'hawkbit_server': f'{hawkbit.host}:{partial_download_port}',
            'resume_downloads': 'true',
        }
    })

    # ignore failing installation
    out, _, _ = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert re.findall('Resuming download from offset [1-9]', out)
    assert 'Download complete.' in out
    assert 'File checksum OK.' in out

def test_download_slow_with_resume(hawkbit, bundle_assigned, adjust_config, rate_limited_port):
    """
    Assign bundle to target and test slow download of bundle with download resuming enabled. That
    should lead to resuming downloads.
    """
    port = rate_limited_port(50000)
    config = adjust_config({
        'client': {
            'hawkbit_server': f'{hawkbit.host}:{port}',
            'resume_downloads': 'true',
            'low_speed_time': '1',
            'low_speed_rate': '100000',
        }
    })

    # ignore failing installation
    out, _, _ = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'Timeout was reached, resuming download..' in out
    assert 'Resuming download from offset' in out
    assert 'Download complete.' in out
    assert 'File checksum OK.' in out

def test_download_only(hawkbit, config, assign_bundle):
    """Test "downloadonly" deployment."""
    assign_bundle(params={'type': 'downloadonly'})

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')
    assert 'Start downloading' in out
    assert 'hawkBit requested to skip installation, not invoking RAUC yet.' in out
    assert 'Download complete' in out
    assert 'File checksum OK' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert any(['download' in s['type'] for s in status])

    # check last status message
    assert 'File checksum OK.' in status[0]['messages']
