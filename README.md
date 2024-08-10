# remotechargeport - helper modules for distributed EVerest setups

## Introduction

This repository contains several [EVerest](https://github.com/EVerest/everest-core) modules which
can be used to create a distributed EVerest system appearing and acting as a single one.
EVerest by itself offers the possibility to have multiple charge ports in a single setup,
e.g. chargers with multiple guns. However, such a EVerest system requires (at the moment) that
these charge ports are available on the same system, i.e. that the hardware platform itself offers
these multiple charge ports so that all EVerest module instances runs on the very same board.
But sometimes the hardware platform only offers a single charging port, and then charging station
manufacturers want to use several physical boards - for each physical charging port a dedicated one.

This is where these modules come into play: one of these charge port controller boards is elected
by the charging station manufaturer to play the main role and to run the full-featured EVerest
system, e.g. with authorization modules, with OCPP connection, energy management modules etc.
All other boards, here named as 'satellites', are controlled by this main installation.
By design, this overall setup is limited for up to three satellites, so that it is possible
to build for example a charging station with 4 guns.

On the satellites, a reduced EVerest installation is running, consisting of the most necessary
modules only. Usually this includes the board support module(s) (i.e. the boards hardware layer),
the `EvseManager` instance for this charge port, possible charge port related module instances
like power meter etc. and a `System` module (to handle e.g. firmware updates etc.).
Additionally, on each satellite a module instance of `SatelliteAgent` is used which tunnels various
interfaces to the main board.

On the main board, the local, physically available port/connector is configured as usual for this
board. But additionally, each 'remote' charge port is instantiated using a `SatelliteController`
module. It acts as a place-holder or let's call it proxy module, offering and using the usual
EVerest module interfaces to/of the main system.

`SatelliteController` and `SatelliteAgent` are linked together using an RPC connection via
IP network. The `SatelliteAgent` acts as an RPC server and the `SatelliteController` connects to
it as RPC client. For this, the library [rpclib](http://rpclib.net/) is used.
(When I started the project, I looked out for a simple and easy-to-use library under heavy
time-pressure. That's why the selection might not be the best one - but at the moment, it
just works as expected.)
In other words, if an EVerest module call would be done to e.g. the `EvseManager` module, then this
call is intercepted by the `SatelliteController`, passed via the RPC connection to the
`SatelliteAgent` and is on this system delivered to the `EvseManager`.
Of course, any return values are passed back, as any variables published at random times from the
remote `EvseManager`.
That way, the RPC connection to the remote system is kind of opague for the main system, however,
a short delay cannot be prevented by design.

The decision to use a dedicated RPC connection for each satellite and not to re-use
e.g. MQTT (EVerest already requires an MQTT broker for local communication):

- the generated RPC traffic should not pass multiple times over the same MQTT broker
- I expected the RPC library to handle all kind of encapsulation etc. by itself,
  I did not wanted to deal with developing it
- I expected that it would be much easier when multiple satellites are connected,
  e.g. observing connection state and/or finding the satellites after booting

While the rough functionally of `SatelliteController` and `SatelliteAgent` should be clear now,
one remaining module must be explained: `SystemAggregator`. Usually each physical board runs one
instance of a `System` module which takes care about platform related tasks like reboot, firmware
update, uploading of diagnostics data etc.
When a distributed system is build with many physical boards, then all shall appear to e.g. the
backend system as a single system. And all boards should run the same firmware version, diagnostics
should be collected from all boards etc.
The `SystemAggregator` module acts as proxy module between the usual board specific `System` module
and forwards/multiplies the mentioned functionalities. For example when OCPP module requests
diagnostics upload, then the request is intercepted and each physical board is requested to upload
to the main board. Later all uploads are collected and uploaded as combined file to the
requesting backend system.

# Conceptual Constraints

As is every distributed system, a fundamental system requirement is that both[^1] sides always know
about the state of the peer. To avoid complex logic to keep track of states on both sides, the
simple approach is to just focus on a common starting point.
This starting point is usually the bootup of all physical boards after e.g. power on of the whole
charging station.

The EVerest framework already provides support for a synchronized startup of all modules: there are
`init` and `ready`callbacks. The `SatelliteAgent` module uses the first to initialize the RPC
server up to a given extend and then waits for an incoming RPC connection. When this connection is
established, then it initializes the RPC server finally and gives feedback to the connected
`SatelliteController`. This `SatelliteController` on the other hand connects in the `init` callback
to the `SatelliteAgent` and decides based on the received feedback whether both sides are allowed
to proceed to the `ready` callback.

A two-step approach is used to ensure that re-connects are detected in case of a crash on only one
side. As mentioned, this approach allows to keep the synchronization logic at a minimum - on the
other hand to means, that the only way out of problems is to restart both sides on the charging
stack. It is assumed, that the outer system management (e.g. systemd or similar) is configured to
restart the EVerest system in such a case so that the common synchronzation point is reached again.

[^1]: To keep it simple, a dual system is used here for documentation.

# Requirements

As mentioned, [rpclib](http://rpclib.net/) is used as underlaying RPC framework.
Please note, that you need a slightly modified version from my
[personal fork](https://github.com/mhei/rpclib/branches). This adds a helper
function to determine the local endpoint address with is passed to the
`SatelliteAgent` for example when a diagnostics upload should be transfered
to the main system.

# Build and Install

In the first step, you have to build and install the RPC library:

```bash
git clone -b mhei-master https://github.com/mhei/rpclib.git
mkdir rpclib/build
cd rpclib/build
cmake ..
make
sudo make install
```

In the next step, it is expected that you are familiar with the EVerest build process and that you
have a working EVerest workspace. Then just clone this repository into that workspace,
in parallel to the `everest-core` directory.

For example:

```bash
cd everest-workspace
git clone https://github.com/mhei/remotechargeport.git
mkdir remotechargeport/build
cd remotechargeport/build
cmake ..
make
```

You will find the binaries in the corresponding sub-directories of `modules`.

# Yocto Integration

For [Yocto](https://www.yoctoproject.org/) builds, recipes and complementary files are maintained
within the [chargebyte](https://www.chargebyte.com)'s
[EVerest meta layer](https://github.com/chargebyte/meta-chargebyte-everest),
see for example:
https://github.com/chargebyte/meta-chargebyte-everest/tree/kirkstone/recipes-core/remotechargeport

# Configuration

TODO: add example configurations for main and satellite system

# Releases and Versioning

Similar to EVerest, a date-based versioning is used. The aim is to have releases with corresponding
versions that are compatible with the upstream EVerest releases.

Note, that this means that there might be a delay after a new upstream EVerest release is tagged.

And a small note: the second version number has a leading zero here so that it sorts better.

# Open TODOs

- The newer error handling framework of EVerest is not yet implemented.
- The configuration of the satellites is currently done with IPv4 addresses or hostnames.
  IP addresses require either a DHCP server which is capable of pinning MAC addresses
  to IP addresses (so that each IP client always receives the same IP address for a given
  MAC address), or a really static IP setup. One idea I have in mind is to use mDNS to discover
  the satellites via its board serial number or similar.
  Not sure yet, whether this improves field deployment experience.

# Report a Bug

To report a bug, you can:
* fill a bug report on the issue tracker
  http://github.com/mhei/remotechargeport/issues
* or send an email to mhei@heimpold.de

# Legal Notes

Trade names, trademarks and registered trademarks are used without special marking throughout the
source code and/or documentation. All are the property of their respective owners.

This project started in my personal, private time. My employer allows me and my colleagues to
develop the project during our working hours. However, no guarantee can be given regarding
completeness, functionality and/or guaranteed support etc.
So as for every open-source project: use at you own risk and contribute back in case it helps you!
