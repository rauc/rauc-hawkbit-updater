import pytest

from helper import run

@pytest.mark.parametrize('mode', ('download', 'streaming'))
def test_install_success_mtls(hawkbit, adjust_config, bundle_assigned, mtls_download_port,
                              pki_dir, ssl_issuer_hash, rauc_dbus_install_success, mode):
    """
    Assign bundle to target and test successful download and installation via
    mTLS. Make sure installation result is received correctly by hawkBit.
    """
    config = adjust_config(
        {'client': {
            'hawkbit_server': f'localhost:{mtls_download_port}',
            'ssl': 'true',
            'ssl_key': f'{pki_dir}/client.key',
            'ssl_cert': f'{pki_dir}/client.crt',
            'ssl_verify': 'false',
            'stream_bundle': 'true' if mode == 'streaming' else 'false'}
        },
        remove={'client': 'auth_token'}
    )

    hawkbit.set_config('authentication.header.authority', ssl_issuer_hash)
    hawkbit.set_config('authentication.header.enabled', True)

    out, err, exitcode = run(f'rauc-hawkbit-updater -c "{config}" -r')

    assert 'New software ready for download' in out

    if mode == 'download':
        assert 'Download complete' in out

    assert 'Software bundle installed successfully.' in out
    assert err == ''
    assert exitcode == 0

    status = hawkbit.get_action_status()
    assert status[0]['type'] == 'finished'
