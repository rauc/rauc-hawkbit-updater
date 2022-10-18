Using the RAUC hawkbit Updater
==============================

.. _authentication-section:

Authentication
--------------

As described on the `hawkBit Authentication page <https://www.eclipse.org/hawkbit/concepts/authentication/>`_
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
