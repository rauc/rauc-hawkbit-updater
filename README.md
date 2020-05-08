RAUC hawkBit Updater
====================

[![Build Status](https://github.com/rauc/rauc-hawkbit-updater/workflows/tests/badge.svg)](https://github.com/rauc/rauc-hawkbit-updater/actions)
[![License](https://img.shields.io/badge/license-LGPLv2.1-blue.svg)](https://raw.githubusercontent.com/rauc/rauc-hawkbit-updater/master/LICENSE)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/rauc/rauc-hawkbit-updater.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/rauc/rauc-hawkbit-updater/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/rauc/rauc-hawkbit-updater.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/rauc/rauc-hawkbit-updater/context:cpp)

The RAUC hawkBit updater is a simple commandline tool / daemon written in C (glib).
It is a port of the RAUC hawkBit Client written in Python.
The daemon runs on your target and operates as an interface between the
[RAUC D-Bus API](https://github.com/rauc/rauc)
and the [hawkBit DDI API](https://github.com/eclipse/hawkbit).

Quickstart
----------

The RAUC hawkBit updater is primarily meant to be used as a daemon,
but it also allows you to do a one shot instantly checking and install
new software.

To quickly getting started with hawkBit server, follow
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
  bundle_download_location  = /tmp/bundle.raucb
  retry_wait                = 60
  connect_timeout           = 20
  timeout                   = 60
  log_level                 = debug
  post_update_reboot        = false

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
  ./rauc-hawkbit-updater -c config.conf
```


Debugging
---------

When setting the log level to 'debug' the RAUC hawkBit client will print
JSON payload sent and received. This can be done by using option -d.

```shell
  ./rauc-hawkbit-updater -d -c config.conf
```


Compile
-------

```shell
  mkdir build
  cd build
  cmake ..
  make
```


Usage / options
---------------

```shell
/usr/bin/rauc-hawkbit-updater --help
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


Authentication
--------------

As described on the [hawkBit Authentication page](https://www.eclipse.org/hawkbit/concepts/authentication/)
in the "DDI API Authentication Modes" section, a device can be authenticated
with a security token. A security token can be either a "Target" token or a
"Gateway" token. The "Target" security token is specific to a single target
defined in hawkBit. In the RAUC hawkBit updater's configuration file it's
referred to as "auth_token".

Targets can also be connected through a gateway which manages the targets
directly and as a result these targets are indirectly connected to the hawkBit
update server. The "Gateway" token is used to authenticate this gateway and
allow it to manage all the targets under its tenant. With RAUC hawkBit updater
such token can be used to authenticate all targets on the server. I.e. same
gateway token can be used in a configuration file replicated on many targets.
In the RAUC hawkBit updater's configuration file it's called "gateway_token".
Although gateway token is very handy during development or testing, it's
recommended to use this token with care because it can be used to
authenticate any device.
