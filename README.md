RAUC hawkBit Updater
====================

[![Build Status](https://github.com/rauc/rauc-hawkbit-updater/workflows/tests/badge.svg)](https://github.com/rauc/rauc-hawkbit-updater/actions)
[![License](https://img.shields.io/badge/license-LGPLv2.1-blue.svg)](https://raw.githubusercontent.com/rauc/rauc-hawkbit-updater/master/LICENSE)
[![CodeQL](https://github.com/rauc/rauc-hawkbit-updater/workflows/CodeQL/badge.svg)](https://github.com/rauc/rauc-hawkbit-updater/actions/workflows/codeql.yml)
[![Documentation](https://readthedocs.org/projects/rauc-hawkbit-updater/badge/?version=latest)](https://rauc-hawkbit-updater.readthedocs.io/en/latest/?badge=latest)
[![Matrix](https://img.shields.io/matrix/rauc:matrix.org?label=matrix%20chat)](https://app.element.io/#/room/#rauc:matrix.org)

The RAUC hawkBit updater is a simple command-line tool/daemon written in C (glib).
It is a port of the RAUC hawkBit Client written in Python.
The daemon runs on your target and operates as an interface between the
[RAUC D-Bus API](https://github.com/rauc/rauc)
and the [hawkBit DDI API](https://github.com/eclipse/hawkbit).

Quickstart
----------

The RAUC hawkBit updater is primarily meant to be used as a daemon,
but it also allows you to do a one-shot instantly checking and install
new software.

To quickly get started with hawkBit server, follow
[this](https://github.com/eclipse/hawkbit#getting-started)
instruction.

Setup target (device) configuration file:

```ini
  [client]
  hawkbit_server            = 127.0.0.1:8080
  ssl                       = false
  ssl_verify                = false
  tenant_id                 = DEFAULT
  target_name               = test-target
  auth_token                = bhVahL1Il1shie2aj2poojeChee6ahShu
  #gateway_token            = bhVahL1Il1shie2aj2poojeChee6ahShu
  #ssl_engine               = pkcs11
  #ssl_key                  = pkcs11:token=mytoken;object=mykey
  #ssl_cert                 = /path/to/certificate.pem
  bundle_download_location  = /tmp/bundle.raucb
  retry_wait                = 60
  connect_timeout           = 20
  timeout                   = 60
  log_level                 = debug
  post_update_reboot        = false
  #stream_bundle            = true

  [device]
  product                   = Terminator
  model                     = T-1000
  serialnumber              = 8922673153
  hw_revision               = 2
  key1                      = value
  key2                      = value
```

All key/values under [device] group are sent to hawkBit as data (attributes).
The attributes in hawkBit can be used in target filters.

Finally start the updater as daemon:

```shell
$ ./rauc-hawkbit-updater -c config.conf
```


Debugging
---------

When setting the log level to 'debug' the RAUC hawkBit client will print
JSON payload sent and received. This can be done by using option -d.

```shell
$ ./rauc-hawkbit-updater -d -c config.conf
```


Compile
-------

Install build pre-requisites:

* meson
* libcurl
* libjson-glib

```shell
$ sudo apt-get update
$ sudo apt-get install meson libcurl4-openssl-dev libjson-glib-dev
```

```shell
$ meson setup build
$ ninja -C build
```

Test Suite
----------

Prepare test suite:

```shell
$ sudo apt install libcairo2-dev libgirepository1.0-dev nginx-full
$ python3 -m venv venv
$ source venv/bin/activate
(venv) $ pip install --upgrade pip
(venv) $ pip install -r test-requirements.txt
```

Run hawkBit docker container:

```shell
$ docker pull hawkbit/hawkbit-update-server
$ docker run -d --name hawkbit -p ::1:8080:8080 -p 127.0.0.1:8080:8080 \
    hawkbit/hawkbit-update-server \
    --hawkbit.server.security.dos.filter.enabled=false \
    --hawkbit.server.security.dos.maxStatusEntriesPerAction=-1 \
    --server.forward-headers-strategy=NATIVE
```

Run test suite:

```shell
(venv) $ ./test/wait-for-hawkbit-online && dbus-run-session -- pytest -v
```

Pass `-o log_cli=true` to pytest in order to enable live logging for all test cases.

Usage / Options
---------------

```shell
$ /usr/bin/rauc-hawkbit-updater --help
Usage:
  rauc-hawkbit-updater [OPTION?]

Help Options:
  -h, --help               Show help options

Application Options:
  -c, --config-file        Configuration file
  -v, --version            Version information
  -d, --debug              Enable debug output
  -r, --run-once           Check and install new software and exit
  -s, --output-systemd     Enable output to systemd
```
