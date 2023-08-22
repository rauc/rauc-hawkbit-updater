# SPDX-License-Identifier: LGPL-2.1-only
# SPDX-FileCopyrightText: 2021 Enrico Jörns <e.joerns@pengutronix.de>, Pengutronix
# SPDX-FileCopyrightText: 2021-2022 Bastian Krause <bst@pengutronix.de>, Pengutronix

import re
from configparser import ConfigParser

import pytest

from helper import run

def test_version():
    """Test version argument."""
    out, err, exitcode = run('rauc-hawkbit-updater -v')

    assert exitcode == 0
    assert out.startswith('Version ')
    assert err == ''

def test_invalid_arg():
    """Test invalid argument."""
    out, err, exitcode = run('rauc-hawkbit-updater --invalidarg')

    assert exitcode == 1
    assert out == ''
    assert err.strip() == 'option parsing failed: Unknown option --invalidarg'

def test_config_unspecified():
    """Test call without config argument."""
    out, err, exitcode = run('rauc-hawkbit-updater')

    assert exitcode == 2
    assert out == ''
    assert err.strip() == 'No configuration file given'

def test_config_file_non_existent():
    """Test call with inexistent config file."""
    out, err, exitcode = run('rauc-hawkbit-updater -c does-not-exist.conf')

    assert exitcode == 3
    assert out == ''
    assert err.strip() == 'No such configuration file: does-not-exist.conf'

def test_config_no_auth(adjust_config):
    """Test config without authentication option in client section."""
    config = adjust_config(remove={'client': 'auth_token'})

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 4
    assert out == ''
    assert err.strip() == \
            "Loading config file failed: Neither token nor ssl authentication set"

def test_config_multiple_token_auth_methods(adjust_config):
    """Test config with auth_token and gateway_token options in client section."""
    config = adjust_config({'client': {'gateway_token': 'wrong-gateway-token'}})

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 4
    assert out == ''
    assert err.strip() == \
            "Loading config file failed: Both 'auth_token' and 'gateway_token' set"

def test_config_multiple_auth_methods(adjust_config):
    """Test config with both token and ssl auth options in client section."""
    config = adjust_config(
            {'client': {'ssl': 'true', 'ssl_key': 'key', 'ssl_cert': 'cert'}}
    )

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 4
    assert out == ''
    assert err.strip() == \
            "Loading config file failed: Both token and ssl authentication set"

def test_register_and_check_invalid_gateway_token(adjust_config):
    """Test config with invalid gateway_token."""
    config = adjust_config(
            {'client': {'gateway_token': 'wrong-gateway-token'}},
            remove={'client': 'auth_token'}
    )

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 1
    assert 'MESSAGE: Checking for new software...' in out
    assert err.strip() == 'WARNING: Failed to authenticate. Check if gateway_token is correct?'

@pytest.mark.parametrize("trailing_space", ('no_trailing_space', 'trailing_space'))
def test_register_and_check_valid_gateway_token(hawkbit, adjust_config, trailing_space):
    """Test config with valid gateway_token."""
    gateway_token = hawkbit.get_config('authentication.gatewaytoken.key')
    config = adjust_config(
            {'client': {'gateway_token': gateway_token}},
            remove={'client': 'auth_token'},
            add_trailing_space=(trailing_space == 'trailing_space'),
    )

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 0
    assert 'MESSAGE: Checking for new software...' in out
    assert err == ''

def test_register_and_check_invalid_auth_token(adjust_config):
    """Test config with invalid auth_token."""
    config = adjust_config({'client': {'auth_token': 'wrong-auth-token'}})

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 1
    assert 'MESSAGE: Checking for new software...' in out
    assert err.strip() == 'WARNING: Failed to authenticate. Check if auth_token is correct?'

@pytest.mark.parametrize("trailing_space", ('no_trailing_space', 'trailing_space'))
def test_register_and_check_valid_auth_token(adjust_config, trailing_space):
    """Test config with valid auth_token."""
    config = adjust_config(
            add_trailing_space=(trailing_space == 'trailing_space'),
    )
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 0
    assert 'MESSAGE: Checking for new software...' in out
    assert err == ''

def test_register_and_check_no_download_location_no_streaming(adjust_config):
    """Test config without bundle_download_location and without stream_bundle."""
    config = adjust_config(remove={'client': 'bundle_download_location'})
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 4
    assert out == ''
    assert err.strip() == \
            "Loading config file failed: 'bundle_download_location' is required if 'stream_bundle' is disabled"

def test_identify(hawkbit, config):
    """
    Test that supplying target meta information works and information are received correctly by
    hawkBit.
    """
    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 0
    assert 'Providing meta information to hawkbit server' in out
    assert err == ''

    ref_config = ConfigParser()
    ref_config.read(config)

    assert dict(ref_config.items('device')) == hawkbit.get_attributes()

@pytest.mark.parametrize("multi_object", ('chunks', 'artifacts'))
def test_unsupported_multi_objects(hawkbit, config, assign_bundle, multi_object):
    """
    Test that deployments with multiple software modules (called chunks in the DDI API) or multiple
    artifacts are rejected.
    """
    expected_error = rf'Deployment \d*? unsupported: cannot handle multiple {multi_object}.'

    if multi_object == 'chunks':
        assign_param = {'swmodules_num': 2}
    elif multi_object == 'artifacts':
        assign_param = {'artifacts_num': 2}

    assign_bundle(**assign_param)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert exitcode == 1
    assert re.fullmatch(f'(WARNING: {expected_error}\n){{2}}', err)

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'error'
    assert re.fullmatch(expected_error, status[0]['messages'][0])
