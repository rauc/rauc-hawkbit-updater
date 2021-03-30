Using the RAUC hawkbit Updater
==============================

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
