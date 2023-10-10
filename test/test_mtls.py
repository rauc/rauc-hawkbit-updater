import pytest

from helper import run

@pytest.mark.parametrize('mode', ('download','streaming'))
def test_install_success_mtls(hawkbit, adjust_config, bundle_assigned,
                                          mtls_download_port,mtls_config,rauc_dbus_install_success_mtls,mode):
    """
    Assign bundle to target and test successful download and installation via MTLS. Make sure installation
    result is received correctly by hawkBit.
    """
    config = adjust_config(
        {'client': {
            'hawkbit_server': f'localhost:{mtls_download_port}',
            "client_cert": mtls_config.client_cert,
            "client_key": mtls_config.client_key,
            "ssl": "true",
            "ssl_verify": "false",
            'stream_bundle': 'true' if mode == 'streaming' else "false"
        }
        },
        remove={'client': ['bundle_download_location','auth_token']} if mode == 'streaming'else {'client':'auth_token'}
    )

    hawkbit.set_config("authentication.header.authority", mtls_config.get_issuer_hash())
    hawkbit.set_config("authentication.header.enabled",True)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'New software ready for download' in out

    if mode == 'download':
        assert 'Download complete' in out

    assert 'Software bundle installed successfully.' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'

@pytest.mark.parametrize('mode', ('download','streaming'))
def test_install_success_mtls_with_token_auth(hawkbit, adjust_config, bundle_assigned,
                              mtls_download_port,mtls_config,rauc_dbus_install_success_mtls,mode):
    """
    Assign bundle to target and test successful download and installation via MTLS with auth token.
    The reverse proxy check the certificate, hawkbit check the auth token.
    Make sure installation result is received correctly by hawkBit.
    """
    config = adjust_config(
        {'client': {
            'hawkbit_server': f'localhost:{mtls_download_port}',
            "client_cert": mtls_config.client_cert,
            "client_key": mtls_config.client_key,
            "ssl": "true",
            "ssl_verify": "false",
            'stream_bundle': 'true' if mode == 'streaming' else "false"
        }
        },
        remove={'client': ['bundle_download_location']} if mode == 'streaming' else {}
    )

    hawkbit.set_config("authentication.header.enabled", False)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'New software ready for download' in out

    if mode == 'download':
        assert 'Download complete' in out

    assert 'Software bundle installed successfully.' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'
