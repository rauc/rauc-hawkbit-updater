from sniffer import DownloadSniffer

def test_download_with_auth_header(config, hawkbit, assign_bundle):
    """Test curl has authentication header by default."""
    assign_bundle(params={'type': 'downloadonly'})

    sniffer = DownloadSniffer(hawkbit)
    sniffer.run_command_with_sniffer(f"rauc-hawkbit-updater -c {config} -r")

    assert len(sniffer.packets) > 0
    for packet in sniffer.packets:
        assert packet.Authorization is not None

def test_download_without_auth_header(adjust_config, hawkbit, assign_bundle):
    """Test curl has no authentication header if we disable it."""
    config = adjust_config({'client': {'disable_download_auth_header': 'true'}})
    assign_bundle(params={'type': 'downloadonly'})

    sniffer = DownloadSniffer(hawkbit)
    sniffer.run_command_with_sniffer(f"rauc-hawkbit-updater -c {config} -r")

    assert len(sniffer.packets) > 0
    for packet in sniffer.packets:
        assert packet.Authorization is None
