import os


class MtlsConfig:
    def __init__(self, cert_dir_location):
        self.certs_dir= str(cert_dir_location) + "/certs/"
        self.ca_cert= self.certs_dir + "root-ca.crt"
        self.ca_key= self.certs_dir + "root-ca.key"
        self.ca_csr= self.certs_dir + "root-csr.pem"
        self.client_cert= self.certs_dir + "client.crt"
        self.client_key= self.certs_dir + "client.key"
        self.issuer_hash = self. certs_dir + "issuer_hash.txt"
    def client_cert_exist(self):
        return os.path.isfile(self.client_cert)

    def ca_cert_exist(self):
        return os.path.isfile(self.ca_cert)

    def get_issuer_hash(self):
        return open(self.issuer_hash, "r").readline().strip()
