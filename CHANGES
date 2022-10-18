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
