#!/bin/bash
set -e
CERT_DIR="${1}"
CONTROLLER_ID="${2}"
CA_CERT="root-ca.crt"
CA_KEY="root-ca.key"
CA_CSR="root-csr.pem"
CERT_CONFIG='
  [client]
  basicConstraints = CA:FALSE
  nsCertType = client, email
  nsComment = "Local Test Client Certificate"
  subjectKeyIdentifier = hash
  authorityKeyIdentifier = keyid,issuer
  keyUsage = critical, nonRepudiation, digitalSignature, keyEncipherment
  extendedKeyUsage = clientAuth
'

mkdir -p ${CERT_DIR}
cd ${CERT_DIR}
if [ ! -f "${CA_CERT}" ]; then
  echo "Development CA"
  openssl req -newkey rsa -keyout ${CA_KEY} -out ${CA_CSR} -subj "/O=Test/CN=localhost" --nodes
  openssl req -x509 -sha256 -new -nodes -key ${CA_KEY} -days 3650 -out ${CA_CERT} -subj '/CN=localhost'
fi
if [ -n "${CONTROLLER_ID}" ]; then
  openssl genrsa -out "client.key" 4096
  openssl req -new -key "client.key" -out "client.csr" -sha256 -subj "/CN=${CONTROLLER_ID}"

  openssl x509 -req -days 750 -in "client.csr" -sha256 -CA ${CA_CERT} -CAkey ${CA_KEY} -CAcreateserial -out "client.crt" -extensions client -extfile <(printf ${CERT_CONFIG})
  openssl x509 -in client.crt -issuer_hash -noout > issuer_hash.txt
fi