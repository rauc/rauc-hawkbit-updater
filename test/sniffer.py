import os
from scapy.all import *
from scapy.layers.http import HTTPRequest

from helper import run


class Sniffer:
    def __init__(self, hawkbit):
        interfaces_list = os.listdir('/sys/class/net/')
        self.packets = []
        self.sniffer = AsyncSniffer(
            filter=f"tcp and port {hawkbit.port}", timeout = 10,
            prn=self.packet_handler, iface=interfaces_list
        )

    def packet_handler(self, packet):
        raise NotImplementedError

    def run_command_with_sniffer(self, command):
        self.sniffer.start()
        run(command)
        self.sniffer.stop()


class DownloadSniffer(Sniffer):
    def __init__(self, hawkbit):
        """Iniatialize sniffer with download URL as expected url"""
        self.expected_url = (
            f"{hawkbit.host}:{hawkbit.port}/DEFAULT/controller/v1/"
            f"{hawkbit.id['target']}/softwaremodules/"
            f"{hawkbit.id['softwaremodule']}/artifacts/bundle.raucb_0"
        )
        super().__init__(hawkbit)

    def packet_handler(self, packet):
        """
        Handler trigger when a packet is detected on hawkbit port.
        We want to find HTTP packet for a download request. If we find a
        request with url matching expected_url we store it in packets list.
        """
        if packet.haslayer(HTTPRequest):
            url = packet[HTTPRequest].Host.decode() + packet[HTTPRequest].Path.decode()
            if url == self.expected_url:
                self.packets.append(packet[HTTPRequest])
