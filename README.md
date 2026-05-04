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
  hawkbit_server                = hawkbit.example.com
  target_name                   = target-1234
  auth_token                    = bhVahL1Il1shie2aj2poojeChee6ahShu
  #gateway_token                = chietha8eiD8Ujaxerifoxoh6Aed1koof
  #ssl_key                      = pkcs11:token=mytoken;object=mykey
  #ssl_cert                     = /path/to/certificate.pem
  bundle_download_location      = /tmp/bundle.raucb
  #tenant_id                    = DEFAULT
  #ssl                          = true
  #ssl_verify                   = true
  #ssl_engine                   = pkcs11
  #connect_timeout              = 20
  #timeout                      = 60
  #retry_wait                   = 300
  #low_speed_time               = 60
  #low_speed_rate               = 100
  #resume_downloads             = false
  #stream_bundle                = false
  #post_update_reboot           = false
  #log_level                    = message
  #send_download_authentication = true
  #soft_update_check_dbus_service = de.example.SoftUpdateCheck
  #soft_update_check_force_on_unavailable = true

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


Soft Update Permission Check
----------------------------

hawkBit supports four rollout action types: **Forced**, **Soft**, **Time Forced**, and
**Download Only**. For soft updates the server signals that installation is at the device's
discretion (DDI fields `download=attempt`, `update=attempt`). Time Forced actions report as
Soft until their deadline, at which point hawkBit switches them to Forced transparently.

The updater can optionally consult an external D-Bus service before proceeding with a soft
update, allowing another application (e.g. a UI or a workload manager) to gate the
installation — for example, to defer it until the device is idle.

**How it works:**

1. On each poll cycle, if a soft update is pending the updater calls `IsReadyForUpdate()` on the
   configured D-Bus service (system bus, object path `/`, interface named after the service).
2. If the method returns `True` the update proceeds normally.
3. If the method returns `False` the update is silently skipped and retried on the next poll
   (controlled by `retry_wait`). No error is reported to hawkBit — the action stays open.
4. Once the service starts returning `True` the update is applied on the very next poll.
5. **Forced** and **Download Only** updates always bypass this check.

**Configuration:**

```ini
[client]
# D-Bus well-known name of the permission service
soft_update_check_dbus_service = de.example.SoftUpdateCheck

# Fallback when the service is unreachable: true = force the update (default), false = skip
soft_update_check_force_on_unavailable = true
```

**Expected D-Bus interface** (object path `/`, interface = service name):

```xml
<interface name='de.example.SoftUpdateCheck'>
  <method name='IsReadyForUpdate'>
    <arg type='b' name='ready' direction='out'/>
  </method>
</interface>
```

If `soft_update_check_dbus_service` is not set (the default), soft updates are applied unconditionally.

When `soft_update_check_force_on_unavailable` is `false` and `soft_update_check_unavailable_max_retries` is set to a positive integer, the updater will force the update after that many consecutive poll cycles where the D-Bus service was unreachable. This prevents a permanently unavailable service from blocking updates indefinitely. The retry counter resets when the service becomes reachable again or when a new deployment is received.

**D-Bus policy file (system bus):**

Because the updater connects to the **system bus**, the D-Bus daemon will reject calls to the
permission service unless a policy file explicitly grants access. Without it the service will be
unreachable and the updater will either skip the update or force it through, depending on
`soft_update_check_force_on_unavailable`.

Create a policy file for your service name, e.g.
`/etc/dbus-1/system.d/de.example.SoftUpdateCheck.conf`:

```xml
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
  "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <!-- allow any process to own the service name and communicate with it -->
  <policy context="default">
    <allow own="de.example.SoftUpdateCheck"/>
    <allow send_destination="de.example.SoftUpdateCheck"/>
    <allow receive_sender="de.example.SoftUpdateCheck"/>
  </policy>
</busconfig>
```

Replace `de.example.SoftUpdateCheck` with the actual service name configured in
`soft_update_check_dbus_service`. After installing the file, reload the D-Bus daemon:

```shell
$ sudo systemctl reload dbus
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
$ sudo apt install libcairo2-dev libgirepository1.0-dev nginx-full libnginx-mod-http-ndk libnginx-mod-http-lua
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

If you want to run the soft update permission check tests, install the D-Bus policy file described
in the `Soft Update Permission Check` section above first (the test dummy connects to the system
bus and requires it):

```shell
$ sudo systemctl reload dbus
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
