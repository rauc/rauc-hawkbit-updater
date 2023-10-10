Using the RAUC hawkbit Updater
==============================

.. _authentication-section:

Authentication
--------------
Authentication via tokens
^^^^^^^^^^^^^^^^^^^^^^^^^

As described on the `hawkBit Authentication page <https://eclipse.dev/hawkbit/concepts/authentication/>`_
in the "DDI API Authentication Modes" section, a device can be authenticated
with a security token. A security token can be either a "Target" token or a
"Gateway" token. The "Target" security token is specific to a single target
defined in hawkBit. In the RAUC hawkBit updater's configuration file it's
referred to as ``auth_token``.

Targets can also be connected through a gateway which manages the targets
directly and as a result these targets are indirectly connected to the hawkBit
update server. The "Gateway" token is used to authenticate this gateway and
allow it to manage all the targets under its tenant. With RAUC hawkBit updater
such token can be used to authenticate all targets on the server. I.e. same
gateway token can be used in a configuration file replicated on many targets.
In the RAUC hawkBit updater's configuration file it's called ``gateway_token``.
Although gateway token is very handy during development or testing, it's
recommended to use this token with care because it can be used to
authenticate any device.

Authentication via Certificates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As can be seen in the system configuration settings of hawkBit, there is a
third option to authenticate the targets. An "Allow targets to authenticate via
a certificate authenticated by a reverse proxy" option. To use this
authentication method a TLS reverse proxy server needs to be set up.
The client and reverse proxy server perform a "SSL-handshake" that means the
client validates the server certificate of the reverse proxy server with its
list of trusted certificates.

The clients request has:

- to have a TLS connection to the reverse proxy server (`ssl` config option must be true)
- to contain the client certificate
- to have the common name of the server certificate match the server
  name set in the configuration file as "hawkbit_server"

The purpose of the reverse proxy is to:

- disband the TLS connection
- check if sent client certificate is valid
- extract the common name and fingerprint of the client certificate
- forward the common name and fingerprint as HTTP headers to the
  hawkBit server

When the hawkBit server receives the request it checks if:

- sent common name matches with the controller ID of the target
- sent fingerprint(s) matches the expected fingerprint(s) which is set
  in the system configuration settings of hawkBit

The client certificate will only be used if a valid path
to a certificate and its key is given in the configuration file.
You can use token and certficate authentication mutualy, with the certificate being
used to authenticate to the reverse proxy and the token to authenticate to
hawkbit.

If the CA of the reverse proxy server is untrusted set ``ssl_verify`` to ``false``.

Here an example of how the configuration file might look like:

    | [client]
    | hawkbit_server            = CN_server_certificate:443
    | ssl                       = true
    | ssl_verify                = true
    | tenant_id                 = DEFAULT
    | target_name               = test-target
    | client_cert              = /path/to/client_certificate.pem
    | client_key               = /path/to/client_certificate.key
    | bundle_download_location  = /tmp/bundle.raucb
    | retry_wait                = 60
    | connect_timeout           = 20
    | timeout                   = 60
    | log_level                 = debug
    | post_update_reboot        = false
    |
    | [device]
    | product                   = Terminator
    | model                     = T-1000
    | serialnumber              = 8922673153
    | hw_revision               = 2
    | key1                      = value
    | key2                      = value

Streaming Support
-----------------

By default, rauc-hawkbit-updater downloads the bundle to a temporary
storage location and then invokes RAUC to install the bundle.
In order to save bundle storage and also potentially download bandwidth
(when combined with adaptive updates), rauc-hawkbit-updater can also leverage
`RAUC's built-in HTTP streaming support <https://rauc.readthedocs.io/en/latest/advanced.html#http-streaming>`_.

To enable it, set ``stream_bundle=true`` in the :ref:`sec_ref_config_file`.

.. note:: rauc-hawkbit-updater will add required authentication headers and
   options to its RAUC D-Bus `InstallBundle API call <https://rauc.readthedocs.io/en/latest/reference.html#gdbus-method-de-pengutronix-rauc-installer-installbundle>`_.

Plain Bundle Support
--------------------

RAUC takes ownership of `plain format bundles <https://rauc.readthedocs.io/en/latest/reference.html#plain-format>`_
during installation.
Thus rauc-hawkbit-updater can remove these bundles after installation only if
it they are located in a directory belonging to the user executing
rauc-hawkbit-updater.

systemd Example
^^^^^^^^^^^^^^^

To store the bundle in such a directory, a configuration file for
systemd-tmpfiles can be created and placed in
``/usr/lib/tmpfiles.d/rauc-hawkbit-updater.conf``.
This tells systemd-tmpfiles to create a directory in ``/tmp`` with proper
ownership:

.. code-block:: cfg

  d /tmp/rauc-hawkbit-updater     - rauc-hawkbit rauc-hawkbit - -

The bundle location needs to be set in rauc-hawkbit-updater's config:

.. code-block:: cfg

  bundle_download_location = /tmp/rauc-hawkbit-updater/bundle.raucb
