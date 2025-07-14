Release 1.4 (released Jul 14, 2025)
-----------------------------------

.. rubric:: Enhancements

* Add support for hawkBit "download-only" deployments. [#160] (by Vyacheslav
  Yurkov)
* Share connection between curl requests. [#170] (by Robin van der Gracht)
* Add support for SSL client certificate authentication, see the new config
  options ``ssl_cert``, ``ssl_key`` and ``ssl_engine``. [#169] (by Florian
  Bezannier, Robin van der Gracht)
* Add ``send_download_authentication`` option to allow disabling sending
  authentication data for bundle downloads (needed if external storage
  providers are used). [#174] (by Kevin Fardel)

.. rubric:: Bug Fixes

* Allow ``stream_bundle=true`` without setting ``bundle_download_location``.
  [#150]

.. rubric:: Testing

* Add ``workflow_dispatch`` trigger allowing manually triggered CI runs. [#154]
* Print subprocess's stdout/stderr on timeout errors for debugging purposes.
  [#163] (by Vyacheslav Yurkov)
* Add CodeQL workflow. [#167]
* Add libcairo2-dev to test dependencies. [#182]
* Bind hawkbit docker container and nginx to localhost only. [#185]
* Drop hawkBit option ``anonymous.download.enabled`` removed in >= 0.8.0.
  [#190]
* Use hawkBit's ``server.forward-headers-strategy=NATIVE`` option allowing a
  reverse proxy between rauc-hawkbit-updater and hawkBit. [#169] (by Robin van
  der Gracht)
* Add SSL client certificate authentication tests using nginx with a test PKI.
  [#169] (by Robin van der Gracht)
* Move nginx configs to dedicated files. [#188]
* Make partial download tests more reliable with nginx lua scripting. [#188]
* Fix non-root nginx execution in some rare cases. [#179] (by Thibaud Dufour)
* Add test for ``send_download_authentication=false``. [#174] (by Kevin Fardel)

.. rubric:: Documentation

* Use correct ``stream_bundle`` configuration option in ``README.md``. [#145]
  (by Lukas Reinecke)
* Improve documentation of ``stream_bundle`` configuration option. [#146]
* Update links to hawkBit documentation. [#164] (by Vyacheslav Yurkov)
* Mention minimal build requirements. [#167]
* Fix readthedocs builds. [#167], [#173]
* Provide full-blown config in ``README.md`` and minimal one in the reference
  documentation. [#195]

.. rubric:: Build System

* Lower ``warning_level`` to 2, because ``-Wpedantic`` is not supported for
  compiling GLib-based code. [#182]

Release 1.3 (released Oct 14, 2022)
-----------------------------------

.. rubric:: Enhancements

* Add option ``stream_bundle=true`` to allow using RAUC's HTTP(S) bundle
  streaming capabilities instead of downloading and storing bundles separately.
  [#130]
* Make error messages more consistent. [#138]

.. rubric:: Build System

* Switch to meson [#113]

Release 1.2 (released Jul 1, 2022)
----------------------------------

.. rubric:: Enhancements

* Let rauc-hawkbit-updater use the recent InstallBundle() DBus method instead of
  legacy Install() method. [#129]

.. rubric:: Bug Fixes

* Fixed NULL pointer dereference if build_api_url() is called for base
  deployment URL without having GLIB_USING_SYSTEM_PRINTF defined [#115]
* Fixed compilation against musl by not including glibc-specific
  bits/types/struct_tm.h [#123] (by Zygmunt Krynicki)

.. rubric:: Code

* Drop some unused variables [#126]

.. rubric:: Testing

* Enable and fix testing for IPv6 addresses [#116]
* Enhance test output by not aborting too early on process termination [#128]
* Set proper names for python logger [#127]

.. rubric:: Documentation

* Corrected retry_wait default value in reference [#118]
* Suggest using systemd-tmpfiles for creating and managing tmp directories
  as storage location for plain bundles [#124] (by Jonas Licht)
* Update and clarify python3 venv usage and dependencies for testing [#125]

Release 1.1 (released Nov 15, 2021)
-----------------------------------

.. rubric:: Enhancements

* RAUC hawkBit Updater does now handle hawkBit cancellation requests.
  This allows to cancel deployments that were not yet
  received/downloaded/installed.
  Once the installation has begun, cancellations are rejected. [#89]
* RAUC hawkBit Updater now explicitly rejects deployments with multiple
  chunks/artifacts as these are conceptually unsupported by RAUC. [#103]
* RAUC hawkBit Updater now implements waiting and retrying when receiving
  HTTP errors 409 (Conflict) or 429 (Too Many Requests) on DDI API calls.
  [#102]
* Enable TCP keep-alive probing to recognize and deal with connection outages
  earlier. [#101]
* New configuration options ``low_speed_time`` and ``low_speed_time`` allow
  to adjust the detection of slow connections to match the expected
  environmental conditions. [#101]
* A new option ``resume_downloads`` allows to configure RAUC hawkBit Updater
  to resume aborted downloads if possible. [#101]
* RAUC hawkBit Updater now evaluates the deployment API's 'skip' options for
  download and update (as e.g. used for maintenance window handling).
  Depending on what attributes are set, this will skip installation after
  download or even the entire update. [#111]

.. rubric:: Testing

* replaced manual injection of temporary env modification by monkeypatch
  fixture
* test cases for all new features were added

.. rubric:: Documentation

* Added note on requirements for storage location when using plain bundle
  format

Release 1.0 (released Sep 15, 2021)
-----------------------------------

This is the initial release of RAUC hawkBit Updater.
