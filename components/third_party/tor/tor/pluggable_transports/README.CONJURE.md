# Conjure

[Conjure](https://jhalderm.com/pub/papers/conjure-ccs19.pdf) is an anti-censorship tool in the refraction networking (a.k.a. decoy routing) lineage of circumvention systems. The key innovation of Conjure is to turn the unused IP address space of deploying ISPs into a large pool of **phantom** proxies that users can connect to. Due to the size of unused IPv6 address space and the potential for collateral damage against real websites hosted by the deploying ISPs, Conjure provides an effective solution to the problem of censors enumerating deployed bridges or proxies.

Conjure is currenty deployed on the University of Colorado network and a small to mid size ISP in Michigan.

# Conjure Pluggable Transport for Tor

This repository is an implementation of both the client and bridge side of a Tor pluggable transport that uses the deployed Conjure network to allow users to connect to the Tor network. The client side calls the [`gotapdance` library](https://github.com/refraction-networking/gotapdance) to communicate with deployed Conjure stations and route client traffic through the phantom proxies assigned by the station. The bridge side receives [haproxy](https://www.haproxy.org/download/1.8/doc/proxy-protocol.txt) connections from the Conjure station that wrap the proxied client traffic.

For more information on how it works, see the [documentation wiki](https://gitlab.torproject.org/tpo/anti-censorship/pluggable-transports/conjure/-/wikis/How-it-Works).

# Dependencies

To build the client, you will need to install the following dependencies:
```
apt-get install libczmq-dev
```

# Deployment details

We currently have deployed a low capacity Conjure bridge named [Haunt](https://metrics.torproject.org/rs.html#details/A84C946BF4E14E63A3C92E140532A4594F2C24CD). To connect through this bridge, use the `torrc` file in the `client/` directory as follows:

```
cd client/
go build
tor -f torrc
```

# Warnings

This tool and the deployment is still under active development. We are still working on securing the connection between the deployed Conjure stations and the Conjure bridge. We are also working on improving the censorship resistance of the registration connection between the client and the station. Do not expect this to work out of the box in all areas.

The Conjure station sometimes suffers from a heavy load of users. When this happens, connections will fail. If you are testing this out, try waiting awhile and trying again later.

# Conjure development

Due to the complex nature of the Conjure deployment, it can be difficult to set up a local development environment. Check out [phantombox](https://gitlab.torproject.org/cohosh/phantombox) for an automated libvirt-based setup that works on Linux.
