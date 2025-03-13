Using the RAUC hawkbit Updater
==============================

.. _authentication-section:

Authentication
--------------

Target token
^^^^^^^^^^^^

As described on the `hawkBit Authentication page <https://eclipse.dev/hawkbit/concepts/authentication/>`_
in the "DDI API Authentication Modes" section, a device can be authenticated
with a security token. A security token can be either a "Target" token or a
"Gateway" token. The "Target" security token is specific to a single target
defined in hawkBit. In the RAUC hawkBit updater's configuration file it's
referred to as ``auth_token``.

Gateway token
^^^^^^^^^^^^^

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

Mutual TLS with client key/certificate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

hawkBit also offers a certificate-based authentication mechanism, also known
as mutual TLS (mTLS), which eliminates the need to share a security token with
the server. This is the preferred authentication mode for targets connecting to
bosch-iot-suite.com. The target needs to send a complete (self-contained)
certificate chain along with the request which is then validated by a trusted
reverse proxy. The certificate chain can contain multiple certificates,
e.g. a target-specific client certificate, an intermediate certificate, and
a root certificate. A full certificate chain is required because the reverse
proxy only keeps fingerprints of issuer(s) certificates.
In RAUC hawkBit updater's configuration file the options are called
``ssl_key`` and ``ssl_cert``. They need to be set to the target's private
key and a full certificate chain. If a file is supplied it needs to be in PEM
format.
Optionally, the ``ssl_engine`` option can be set if an OpenSSL engine
needs to be loaded to access the private key. In that case the format of the
value supplied to ``ssl_key`` depends on the engine configured.

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
