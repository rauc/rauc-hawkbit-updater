.. _sec_ref:

Reference
=========

.. contents::
   :local:
   :depth: 1

.. _sec_ref_config_file:

Configuration File
------------------

Minimal configuration example:

.. code-block:: cfg

  [client]
  hawkbit_server            = hawkbit.example.com
  target_name               = target-1234
  auth_token                = bhVahL1Il1shie2aj2poojeChee6ahShu
  bundle_download_location  = /tmp/bundle.raucb

  [device]
  key1                      = valueA
  key2                      = valueB

**[client] section**

Configures how to connect to a hawkBit server, etc.

Mandatory options:

``hawkbit_server=<host>[:<port>]``
  The IP or hostname of the hawkbit server to connect to
  (Punycode representation must be used for host names containing Unicode
  characters).
  The ``port`` can be provided optionally, separated by a colon.

``target_name=<name>``
  Unique ``name`` string to identify controller.

``auth_token=<token>``
  Controller-specific authentication token.
  This is set for each device individually.
  For details, refer to https://eclipse.dev/hawkbit/concepts/authentication/.

  .. note::
    Either ``auth_token``, ``gateway_token`` or ``ssl_key``/``ssl_cert`` must
    be provided.

``gateway_token=<token>``
  Gateway authentication token.
  This is a tenant-wide token and must explicitly be enabled in hakwBit first.
  It is actually meant to authenticate a gateway that itself
  manages/authenticates multiple targets, thus use with care.
  For details, refer to https://eclipse.dev/hawkbit/concepts/authentication/.

  .. note::
    Either ``auth_token``, ``gateway_token`` or ``ssl_key``/``ssl_cert`` must
    be provided.

``ssl_key=<path/URI>``
  Set SSL private key for TLS/SSL client certificate authentication.
  Only used if ``ssl_cert`` is also set.
  See https://curl.se/libcurl/c/CURLOPT_SSLKEY.html.
  See also ``ssl_engine``, ``send_download_authentication``.

  .. note::
    Either ``auth_token``, ``gateway_token`` or ``ssl_key``/``ssl_cert`` must
    be provided.

``ssl_cert=<path/URI>``
  Set SSL client certificate for TLS/SSL client certificate authentication.
  Only used if ``ssl_key`` is also set.
  See https://curl.se/libcurl/c/CURLOPT_SSLCERT.html.
  See also ``ssl_engine``, ``send_download_authentication``.

  .. note::
    Either ``auth_token``, ``gateway_token`` or ``ssl_key``/``ssl_cert`` must
    be provided.

``bundle_download_location=<path>``
  Full path to where the bundle should be downloaded to.
  E.g. set to ``/tmp/_bundle.raucb`` to let rauc-hawkbit-updater use this
  location within ``/tmp``.

  .. note:: Option can be ommited if ``stream_bundle`` is enabled.

Optional options:

``tenant_id=<ID>``
  ID of the tenant to connect to. Defaults to ``DEFAULT``.

``ssl=<boolean>``
  Whether to use SSL connections (``https``) or not (``http``).
  Defaults to ``true``.

``ssl_verify=<boolean>``
  Whether to enforce SSL verification or not.
  Defaults to ``true``.

``ssl_engine=<engine/provider name>``
  Set OpenSSL engine/provider for TLS/SSL client certificate authentication.
  Only used if both ``ssl_key`` and ``ssl_cert`` are set.
  See https://curl.se/libcurl/c/CURLOPT_SSLENGINE.html.
  See also ``send_download_authentication``.

``connect_timeout=<seconds>``
  HTTP connection setup timeout [seconds].
  Defaults to ``20`` seconds.
  Has no effect on bundle downloading when used with ``stream_bundle=true``.

``timeout=<seconds>``
  HTTP request timeout [seconds].
  Defaults to ``60`` seconds.

``retry_wait=<seconds>``
  Time to wait before retrying in case an error occurred [seconds].
  Defaults to ``300`` seconds.

``low_speed_time=<seconds>``
  Time to be below ``low_speed_rate`` to trigger the low speed abort.
  Defaults to ``60``.
  See https://curl.se/libcurl/c/CURLOPT_LOW_SPEED_TIME.html.
  Has no effect when used with ``stream_bundle=true``.

``low_speed_rate=<bytes per second>``
  Average transfer speed to be below during ``low_speed_time`` seconds to
  consider transfer as "too slow" and abort it.
  Defaults to ``100``.
  See https://curl.se/libcurl/c/CURLOPT_LOW_SPEED_LIMIT.html.
  Has no effect when used with ``stream_bundle=true``.

``resume_downloads=<boolean>``
  Whether to resume aborted downloads or not.
  Defaults to ``false``.
  Has no effect when used with ``stream_bundle=true``.

``stream_bundle=<boolean>``
  Whether to install bundles via
  `RAUC's HTTP streaming installation support <https://rauc.readthedocs.io/en/latest/advanced.html#http-streaming>`_.
  Defaults to ``false``.
  rauc-hawkbit-updater does not download the bundle in this case, but rather
  hands the hawkBit bundle URL and the :ref:`authentication header <authentication-section>` to RAUC.

  .. important::
    hawkBit's default configuration limits the number of HTTP range requests to
    ~1000 per action and 200 per second.
    Depending on the bundle size and bandwidth available, streaming a bundle
    might exceed these limitations.
    Starting hawkBit with ``--hawkbit.server.security.dos.filter.enabled=false``
    ``--hawkbit.server.security.dos.maxStatusEntriesPerAction=-1`` disables
    these limitations.

  .. note::
    hawkBit generates an "ActionStatus" for each range request, see
    `this hawkBit issue <https://github.com/eclipse/hawkbit/issues/1249>`_.

``post_update_reboot=<boolean>``
  Whether to reboot the system after a successful update.
  Defaults to ``false``.

  .. important::
    Note that this results in an immediate reboot without contacting the system
    manager and without terminating any processes or unmounting any file systems.
    This may result in data loss.

``log_level=<level>``
  Log level to print, where ``level`` is a string of

  * ``debug``
  * ``info``
  * ``message``
  * ``critical``
  * ``error``
  * ``fatal``

  Defaults to ``message``.

``send_download_authentication=<boolean>``
  Whether to send authentication data (token or client certificate) for
  download requests.
  hawkBit can be configured to use external storage providers for artifact
  downloads.
  rauc-hawkbit-updater's default behavior is to send authentication data, same
  as for all other DDI API requests.
  Sending unexpected authentication data can lead to errors in such
  configuration (e.g. on Azure Blob Storage or AWS S3).
  Defaults to ``true``.

.. _keyring-section:

**[device] section**

This section allows to set a custom list of key-value pairs that will be used
as config data target attribute for device registration.
They can be used for target filtering.

.. important::
  The [device] section is mandatory and at least one key-value pair must be
  configured.
