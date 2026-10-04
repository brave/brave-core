use std::cmp;
use std::fmt;
use std::net::Ipv4Addr;
use std::str::FromStr;
use std::hash::{Hash, Hasher};
use crate::{IpNetworkError, IpNetworkParseError};
use crate::helpers;
use crate::iterator;
use std::collections::HashMap;
use std::collections::hash_map::Entry;

/// IPv4 Network.
#[derive(Clone, Copy, Debug, Eq, PartialOrd, Ord)]
pub struct Ipv4Network {
    pub(crate) network_address: Ipv4Addr,
    pub(crate) netmask: u8,
}

impl Ipv4Network {
    /// IPv4 address length in bits.
    pub const LENGTH: u8 = 32;

    /// Default route that contains all IP addresses, IP network 0.0.0.0/0
    pub const DEFAULT_ROUTE: Self = Self {
        network_address: Ipv4Addr::UNSPECIFIED,
        netmask: 0,
    };

    /// Constructs new `Ipv4Network` based on [`Ipv4Addr`] and `netmask`.
    ///
    /// Returns error if netmask is bigger than 32 or if host bits are set in `network_address`.
    ///
    /// [`Ipv4Addr`]: https://doc.rust-lang.org/std/net/struct.Ipv4Addr.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert_eq!(ip_network.network_address(), Ipv4Addr::new(192, 168, 1, 0));
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    #[allow(clippy::new_ret_no_self)]
    pub fn new(network_address: Ipv4Addr, netmask: u8) -> Result<Self, IpNetworkError> {
        if netmask > Self::LENGTH {
            return Err(IpNetworkError::NetmaskError(netmask));
        }

        if u32::from(network_address).trailing_zeros() < u32::from(Self::LENGTH - netmask) {
            return Err(IpNetworkError::HostBitsSet);
        }

        Ok(Self {
            network_address,
            netmask,
        })
    }

    /// Constructs new `Ipv4Network` based on [`Ipv4Addr`] and `netmask` with truncating host bits
    /// from given `network_address`.
    ///
    /// Returns error if netmask is bigger than 32.
    ///
    /// [`Ipv4Addr`]: https://doc.rust-lang.org/std/net/struct.Ipv4Addr.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new_truncate(Ipv4Addr::new(192, 168, 1, 100), 24)?;
    /// assert_eq!(ip_network.network_address(), Ipv4Addr::new(192, 168, 1, 0));
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn new_truncate(network_address: Ipv4Addr, netmask: u8) -> Result<Self, IpNetworkError> {
        if netmask > Self::LENGTH {
            return Err(IpNetworkError::NetmaskError(netmask));
        }

        let network_address =
            Ipv4Addr::from(u32::from(network_address) & helpers::bite_mask(netmask));

        Ok(Self {
            network_address,
            netmask,
        })
    }

    /// Returns network IP address (first address in range).
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert_eq!(ip_network.network_address(), Ipv4Addr::new(192, 168, 1, 0));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    #[inline]
    pub fn network_address(&self) -> Ipv4Addr {
        self.network_address
    }

    /// Returns broadcast address of network (last address in range).
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert_eq!(ip_network.broadcast_address(), Ipv4Addr::new(192, 168, 1, 255));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn broadcast_address(&self) -> Ipv4Addr {
        Ipv4Addr::from(u32::from(self.network_address) | !helpers::bite_mask(self.netmask))
    }

    /// Returns network mask as integer.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    #[inline]
    pub fn netmask(&self) -> u8 {
        self.netmask
    }

    /// Returns network mask as IPv4 address.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert_eq!(ip_network.full_netmask(), Ipv4Addr::new(255, 255, 255, 0));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn full_netmask(&self) -> Ipv4Addr {
        Ipv4Addr::from(helpers::bite_mask(self.netmask))
    }

    /// Returns [`true`] if given [`IPv4Addr`] is inside this network.
    ///
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    /// [`Ipv4Addr`]: https://doc.rust-lang.org/std/net/struct.Ipv4Addr.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert!(ip_network.contains(Ipv4Addr::new(192, 168, 1, 2)));
    /// assert!(!ip_network.contains(Ipv4Addr::new(192, 168, 2, 2)));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn contains(&self, ip: Ipv4Addr) -> bool {
        u32::from(ip) & helpers::bite_mask(self.netmask) == u32::from(self.network_address)
    }

    /// Returns iterator over host IP addresses in range (without network and broadcast address). You
    /// can also use this method to check how much hosts address are in range by calling [`len()`] method
    /// on iterator (see Examples).
    ///
    /// [`len()`]: https://doc.rust-lang.org/std/iter/trait.ExactSizeIterator.html#method.len
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip = Ipv4Addr::new(192, 168, 1, 0);
    /// let mut hosts = Ipv4Network::new(ip, 24)?.hosts();
    /// assert_eq!(254, hosts.len());
    /// assert_eq!(hosts.next().unwrap(), Ipv4Addr::new(192, 168, 1, 1));
    /// assert_eq!(hosts.last().unwrap(), Ipv4Addr::new(192, 168, 1, 254));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn hosts(&self) -> impl ExactSizeIterator<Item = Ipv4Addr> {
        iterator::Ipv4RangeIterator::hosts(*self)
    }

    /// Returns network with smaller netmask by one. If netmask is already zero, `None` will be returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip = Ipv4Addr::new(192, 168, 1, 0);
    /// let mut hosts = Ipv4Network::new(ip, 24)?;
    /// assert_eq!(hosts.supernet(), Some(Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 23)?));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn supernet(&self) -> Option<Self> {
        if self.netmask == 0 {
            None
        } else {
            Some(Self::new_truncate(self.network_address, self.netmask - 1).unwrap())
        }
    }

    /// Returns iterator over networks with bigger netmask by one. If netmask is already 32,
    /// iterator is empty.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// let mut iterator = ip_network.subnets();
    /// assert_eq!(iterator.next().unwrap(), Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 25)?);
    /// assert_eq!(iterator.last().unwrap(), Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 128), 25)?);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn subnets(&self) -> impl ExactSizeIterator<Item = Ipv4Network> {
        let new_netmask = cmp::min(self.netmask + 1, Self::LENGTH);
        iterator::Ipv4NetworkIterator::new(*self, new_netmask)
    }

    /// Returns `Ipv4NetworkIterator` over networks with defined netmask.
    ///
    /// # Panics
    ///
    /// This method panics when prefix is bigger than 32 or when prefix is lower or equal than netmask.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip = Ipv4Addr::new(192, 168, 1, 0);
    /// let mut iterator = Ipv4Network::new(ip, 24)?.subnets_with_prefix(25);
    /// assert_eq!(iterator.next().unwrap(), Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 25)?);
    /// assert_eq!(iterator.last().unwrap(), Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 128), 25)?);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn subnets_with_prefix(&self, prefix: u8) -> impl ExactSizeIterator<Item = Ipv4Network> {
        iterator::Ipv4NetworkIterator::new(*self, prefix)
    }

    /// Returns [`true`] for the default route network (0.0.0.0/0), that contains all IPv4 addresses.
    ///
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(0, 0, 0, 0), 0)?.is_default_route());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_default_route(&self) -> bool {
        self.netmask == 0
    }

    /// Returns [`true`] for network in local identification range (0.0.0.0/8).
    ///
    /// This property is defined by [IETF RFC 1122].
    ///
    /// [IETF RFC 1122]: https://tools.ietf.org/html/rfc1122
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(0, 0, 0, 0), 8)?.is_local_identification());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_local_identification(&self) -> bool {
        self.network_address.octets()[0] == 0 && self.netmask >= 8
    }

    /// Returns [`true`] for the special 'unspecified' network (0.0.0.0/32).
    ///
    /// This property is defined in _UNIX Network Programming, Second Edition_,
    /// W. Richard Stevens, p. 891; see also [ip7].
    ///
    /// [ip7]: http://man7.org/linux/man-pages/man7/ip.7.html
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(0, 0, 0, 0), 32)?.is_unspecified());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_unspecified(&self) -> bool {
        self.netmask == Self::LENGTH && self.network_address.is_unspecified()
    }

    /// Returns [`true`] if this network is inside loopback address range (127.0.0.0/8).
    ///
    /// This property is defined by [IETF RFC 1122].
    ///
    /// [IETF RFC 1122]: https://tools.ietf.org/html/rfc1122
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 8)?.is_loopback());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_loopback(&self) -> bool {
        self.network_address.is_loopback()
    }

    /// Returns [`true`] if this is a broadcast network (255.255.255.255/32).
    ///
    /// A broadcast address has all octets set to 255 as defined in [IETF RFC 919].
    ///
    /// [IETF RFC 919]: https://tools.ietf.org/html/rfc919
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(255, 255, 255, 255), 32)?.is_broadcast());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_broadcast(&self) -> bool {
        self.network_address.is_broadcast()
    }

    /// Returns [`true`] if this whole network range is inside private address ranges.
    ///
    /// The private address ranges are defined in [IETF RFC 1918] and include:
    ///
    ///  - 10.0.0.0/8
    ///  - 172.16.0.0/12
    ///  - 192.168.0.0/16
    ///
    /// [IETF RFC 1918]: https://tools.ietf.org/html/rfc1918
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?.is_private());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_private(&self) -> bool {
        let octets = self.network_address.octets();
        match octets[0] {
            10 if self.netmask >= 8 => true,
            172 if octets[1] >= 16 && octets[1] <= 31 && self.netmask >= 12 => true,
            192 if octets[1] == 168 && self.netmask >= 16 => true,
            _ => false,
        }
    }

    /// Returns [`true`] if this whole network is inside IETF Protocol Assignments range (192.0.0.0/24).
    ///
    /// This property is defined by [IETF RFC 6890, Section 2.1].
    ///
    /// [IETF RFC 6890, Section 2.1]: https://tools.ietf.org/html/rfc6890#section-2.1
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(192, 0, 0, 0), 24)?.is_ietf_protocol_assignments());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_ietf_protocol_assignments(&self) -> bool {
        let octets = self.network_address.octets();
        octets[0] == 192 && octets[1] == 0 && octets[2] == 0 && self.netmask >= 24
    }

    /// Returns [`true`] if this whole network is inside Shared Address Space (100.64.0.0/10).
    ///
    /// This property is defined by [IETF RFC 6598].
    ///
    /// [IETF RFC 6598]: https://tools.ietf.org/html/rfc6598
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(100, 64, 0, 0), 10)?.is_shared_address_space());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_shared_address_space(&self) -> bool {
        let octets = self.network_address.octets();
        octets[0] == 100 && octets[1] & 0xc0 == 64
    }

    /// Returns [`true`] if the network is is inside link-local range (169.254.0.0/16).
    ///
    /// This property is defined by [IETF RFC 3927].
    ///
    /// [IETF RFC 3927]: https://tools.ietf.org/html/rfc3927
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(169, 254, 1, 0), 24)?.is_link_local());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_link_local(&self) -> bool {
        self.network_address.is_link_local() && self.netmask >= 16
    }

    /// Returns [`true`] if this whole network is inside multicast address range (224.0.0.0/4).
    ///
    /// Multicast network addresses have a most significant octet between 224 and 239,
    /// and is defined by [IETF RFC 5771].
    ///
    /// [IETF RFC 5771]: https://tools.ietf.org/html/rfc5771
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(224, 168, 1, 0), 24)?.is_multicast());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_multicast(&self) -> bool {
        self.network_address.octets()[0] & 0xf0 == 224 && self.netmask >= 4
    }

    /// Returns [`true`] if this whole network is inside benchmarking address range (198.18.0.0/15).
    ///
    /// This property is defined by [IETF RFC 2544].
    ///
    /// [IETF RFC 2544]: https://tools.ietf.org/html/rfc2544
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(198, 19, 1, 0), 24)?.is_benchmarking());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_benchmarking(&self) -> bool {
        // Not necessary to check netmask
        let octets = self.network_address.octets();
        octets[0] == 198 && octets[1] & 0xfe == 18
    }

    /// Returns [`true`] if this whole network is inside reserved address range (240.0.0.0/4), except
    /// broadcast address (255.255.255.255/32).
    ///
    /// Reserved network addresses have a most significant octet between 240 and 255,
    /// and is defined by [IETF RFC 1112].
    ///
    /// [IETF RFC 1112]: https://tools.ietf.org/html/rfc1112
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(240, 168, 1, 0), 24)?.is_reserved());
    /// assert!(!Ipv4Network::new(Ipv4Addr::new(255, 255, 255, 255), 32)?.is_reserved());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_reserved(&self) -> bool {
        // Not necessary to check netmask
        self.network_address.octets()[0] & 0xf0 == 240 && !self.network_address.is_broadcast()
    }

    /// Returns [`true`] if this network is in a range designated for documentation.
    ///
    /// This is defined in [IETF RFC 5737]:
    ///
    /// - 192.0.2.0/24 (TEST-NET-1)
    /// - 198.51.100.0/24 (TEST-NET-2)
    /// - 203.0.113.0/24 (TEST-NET-3)
    ///
    /// [IETF RFC 5737]: https://tools.ietf.org/html/rfc5737
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(Ipv4Network::new(Ipv4Addr::new(192, 0, 2, 0), 24)?.is_documentation());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_documentation(&self) -> bool {
        self.network_address.is_documentation() && self.netmask >= 24
    }

    /// Returns [`true`] if the network appears to be globally routable.
    /// See [IANA IPv4 Special-Purpose Address Registry][ipv4-sr].
    ///
    /// The following return [`false`]:
    ///
    /// - local identification (0.0.0.0/8)
    /// - private address (10.0.0.0/8, 172.16.0.0/12 and 192.168.0.0/16)
    /// - Shared Address Space (100.64.0.0/10)
    /// - the loopback address (127.0.0.0/8)
    /// - the link-local address (169.254.0.0/16)
    /// - IETF Protocol Assignments	(192.0.0.0/24, except 192.0.0.9/32 and 192.0.0.10/32)
    /// - the broadcast address (255.255.255.255/32)
    /// - test addresses used for documentation (192.0.2.0/24, 198.51.100.0/24 and 203.0.113.0/24)
    /// - benchmarking (198.18.0.0/15)
    /// - reserved range (240.0.0.0/4)
    ///
    /// [ipv4-sr]: https://www.iana.org/assignments/iana-ipv4-special-registry/iana-ipv4-special-registry.xhtml
    /// [`true`]: https://doc.rust-lang.org/std/primitive.bool.html
    /// [`false`]: https://doc.rust-lang.org/std/primitive.bool.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert!(!Ipv4Network::new(Ipv4Addr::new(10, 254, 0, 0), 16)?.is_global());
    /// assert!(!Ipv4Network::new(Ipv4Addr::new(192, 168, 10, 65), 32)?.is_global());
    /// assert!(!Ipv4Network::new(Ipv4Addr::new(172, 16, 10, 65), 32)?.is_global());
    /// assert!(!Ipv4Network::new(Ipv4Addr::new(0, 0, 0, 0), 32)?.is_global());
    /// assert!(Ipv4Network::new(Ipv4Addr::new(80, 9, 12, 3), 32)?.is_global());
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn is_global(&self) -> bool {
        let octets = self.network_address.octets();
        // These address are only two globally routable from IETF Protocol Assignments.
        if self.netmask == 32 && (octets == [192, 0, 0, 9] || octets == [192, 0, 0, 10]) {
            return true;
        }

        !self.is_local_identification()
            && !self.is_private()
            && !self.is_ietf_protocol_assignments()
            && !self.is_shared_address_space()
            && !self.is_loopback()
            && !self.is_link_local()
            && !self.is_broadcast()
            && !self.is_documentation()
            && !self.is_benchmarking()
            && !self.is_reserved()
    }

    /// Return a vector of the summarized network range given the first and last IPv4 addresses.
    /// Implementation of this method was inspired by Python [`ipaddress.summarize_address_range`]
    /// method. If first IP address is bigger than last, empty vector is returned.
    ///
    /// [`ipaddress.summarize_address_range`]: https://docs.python.org/3/library/ipaddress.html#ipaddress.summarize_address_range
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ranges = Ipv4Network::summarize_address_range(
    ///     Ipv4Addr::new(10, 254, 0, 0),
    ///     Ipv4Addr::new(10, 255, 255, 255),
    /// );
    ///
    /// assert_eq!(Ipv4Network::new(Ipv4Addr::new(10, 254, 0, 0), 15)?, ranges[0]);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn summarize_address_range(first: Ipv4Addr, last: Ipv4Addr) -> Vec<Self> {
        let mut first_int = u32::from(first);
        let last_int = u32::from(last);

        let mut vector = vec![];

        while first_int <= last_int {
            let bit_length_diff = if last_int - first_int == u32::MAX {
                Self::LENGTH
            } else {
                helpers::bit_length(last_int - first_int + 1) - 1
            };

            let nbits = cmp::min(first_int.trailing_zeros() as u8, bit_length_diff);

            vector.push(Self {
                network_address: Ipv4Addr::from(first_int),
                netmask: Self::LENGTH - nbits,
            });

            if nbits == Self::LENGTH {
                break;
            }

            match first_int.checked_add(1 << nbits) {
                Some(x) => first_int = x,
                None => break,
            }
        }

        vector
    }

    /// Return an iterator of the collapsed Ipv4Networks.
    ///
    /// Implementation of this method was inspired by Python [`ipaddress.collapse_addresses`]
    ///
    /// [`ipaddress.collapse_addresses`]: https://docs.python.org/3/library/ipaddress.html#ipaddress.collapse_addresses
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let collapsed = Ipv4Network::collapse_addresses(&[
    ///     Ipv4Network::new(Ipv4Addr::new(192, 0, 2, 0), 25)?,
    ///     Ipv4Network::new(Ipv4Addr::new(192, 0, 2, 128), 25)?,
    /// ]);
    ///
    /// assert_eq!(Ipv4Network::new(Ipv4Addr::new(192, 0, 2, 0), 24)?, collapsed[0]);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn collapse_addresses(addresses: &[Self]) -> Vec<Self> {
        let mut subnets = HashMap::new();

        let mut to_merge = addresses.to_vec();
        while let Some(net) = to_merge.pop() {
            let supernet = net.supernet().unwrap_or(Ipv4Network::DEFAULT_ROUTE);
            match subnets.entry(supernet) {
                Entry::Vacant(vacant) => {
                    vacant.insert(net);
                }
                Entry::Occupied(occupied) => {
                    if *occupied.get() != net {
                        occupied.remove();
                        to_merge.push(supernet);
                    }
                }
            }
        }

        let mut output: Vec<Ipv4Network> = vec![];
        let mut values = subnets.values().collect::<Vec<_>>();
        values.sort_unstable();

        for net in values {
            if let Some(last) = output.last() {
                // Since they are sorted, last.network_address <= net.network_address is a given.
                if last.broadcast_address() >= net.broadcast_address() {
                    continue;
                }
            }
            output.push(*net);
        }
        output
    }

    /// Converts string in format X.X.X.X/Y (CIDR notation) to `Ipv4Network`, but truncating host bits.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip_network = Ipv4Network::from_str_truncate("192.168.1.255/24")?;
    /// assert_eq!(ip_network.network_address(), Ipv4Addr::new(192, 168, 1, 0));
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkParseError>(())
    /// ```
    pub fn from_str_truncate(s: &str) -> Result<Self, IpNetworkParseError> {
        let (ip, netmask) =
            helpers::split_ip_netmask(s).ok_or(IpNetworkParseError::InvalidFormatError)?;

        let network_address =
            Ipv4Addr::from_str(ip).map_err(|_| IpNetworkParseError::AddrParseError)?;
        let netmask =
            u8::from_str(netmask).map_err(|_| IpNetworkParseError::InvalidNetmaskFormat)?;

        Self::new_truncate(network_address, netmask).map_err(IpNetworkParseError::IpNetworkError)
    }
}

impl fmt::Display for Ipv4Network {
    /// Converts `Ipv4Network` to string in format X.X.X.X/Y (CIDR notation).
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// assert_eq!(Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?.to_string(), "192.168.1.0/24");
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        write!(fmt, "{}/{}", self.network_address, self.netmask)
    }
}

impl FromStr for Ipv4Network {
    type Err = IpNetworkParseError;

    /// Converts string in format X.X.X.X/Y (CIDR notation) to `Ipv4Network`.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    /// use std::str::FromStr;
    ///
    /// let ip_network = Ipv4Network::from_str("192.168.1.0/24")?;
    /// assert_eq!(ip_network.network_address(), Ipv4Addr::new(192, 168, 1, 0));
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkParseError>(())
    /// ```
    fn from_str(s: &str) -> Result<Ipv4Network, IpNetworkParseError> {
        let (ip, netmask) =
            helpers::split_ip_netmask(s).ok_or(IpNetworkParseError::InvalidFormatError)?;

        let network_address =
            Ipv4Addr::from_str(ip).map_err(|_| IpNetworkParseError::AddrParseError)?;
        let netmask =
            u8::from_str(netmask).map_err(|_| IpNetworkParseError::InvalidNetmaskFormat)?;

        Self::new(network_address, netmask).map_err(IpNetworkParseError::IpNetworkError)
    }
}

impl From<Ipv4Addr> for Ipv4Network {
    /// Converts `Ipv4Addr` to `Ipv4Network` with netmask 32.
    #[inline]
    fn from(ip: Ipv4Addr) -> Self {
        Self {
            network_address: ip,
            netmask: Self::LENGTH,
        }
    }
}

impl PartialEq for Ipv4Network {
    #[inline]
    fn eq(&self, other: &Ipv4Network) -> bool {
        self.netmask == other.netmask && self.network_address == other.network_address
    }
}

impl Hash for Ipv4Network {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.network_address.hash(state);
        self.netmask.hash(state);
    }
}

impl IntoIterator for Ipv4Network {
    type Item = Ipv4Addr;
    type IntoIter = iterator::Ipv4RangeIterator;

    /// Returns iterator over all IP addresses in range including network and broadcast addresses.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::Ipv4Network;
    ///
    /// let ip = Ipv4Addr::new(192, 168, 1, 0);
    /// let mut iter = Ipv4Network::new(ip, 24)?.into_iter();
    /// assert_eq!(iter.next().unwrap(), Ipv4Addr::new(192, 168, 1, 0));
    /// assert_eq!(iter.next().unwrap(), Ipv4Addr::new(192, 168, 1, 1));
    /// assert_eq!(iter.last().unwrap(), Ipv4Addr::new(192, 168, 1, 255));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    fn into_iter(self) -> Self::IntoIter {
        Self::IntoIter::new(self.network_address, self.broadcast_address())
    }
}

#[cfg(test)]
mod tests {
    use std::net::Ipv4Addr;
    use crate::{IpNetworkError, Ipv4Network};
    use std::str::FromStr;
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};

    fn return_test_ipv4_network() -> Ipv4Network {
        Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 16).unwrap()
    }

    #[test]
    fn default_route() {
        let network = Ipv4Network::DEFAULT_ROUTE;
        assert!(network.is_default_route());
    }

    #[test]
    fn new_host_bits_set() {
        let ip = Ipv4Addr::new(127, 0, 0, 1);
        let ip_network = Ipv4Network::new(ip, 8);
        assert!(ip_network.is_err());
        assert_eq!(IpNetworkError::HostBitsSet, ip_network.unwrap_err());
    }

    #[test]
    fn new_host_bits_set_no_31() {
        let ip = Ipv4Addr::new(127, 0, 0, 2);
        let ip_network = Ipv4Network::new(ip, 31);
        assert!(ip_network.is_ok());
    }

    #[test]
    fn new_host_bits_set_no_32() {
        let ip = Ipv4Addr::new(127, 0, 0, 1);
        let ip_network = Ipv4Network::new(ip, 32);
        assert!(ip_network.is_ok());
    }

    #[test]
    fn new_host_bits_set_no_zero() {
        let ip = Ipv4Addr::new(0, 0, 0, 0);
        let ip_network = Ipv4Network::new(ip, 0);
        assert!(ip_network.is_ok());
    }

    #[test]
    fn new_big_invalid_netmask() {
        let ip = Ipv4Addr::new(127, 0, 0, 1);
        let ip_network = Ipv4Network::new(ip, 33);
        assert!(ip_network.is_err());
        assert_eq!(IpNetworkError::NetmaskError(33), ip_network.unwrap_err());
    }

    #[test]
    fn new_truncate_host_bits_set() {
        let ip = Ipv4Addr::new(127, 0, 0, 1);
        let ip_network = Ipv4Network::new_truncate(ip, 8).unwrap();
        assert_eq!(ip_network.network_address(), Ipv4Addr::new(127, 0, 0, 0));
    }

    #[test]
    fn new_truncate_big_invalid_netmask() {
        let ip = Ipv4Addr::new(127, 0, 0, 1);
        let ip_network = Ipv4Network::new_truncate(ip, 33);
        assert!(ip_network.is_err());
        assert_eq!(IpNetworkError::NetmaskError(33), ip_network.unwrap_err());
    }

    #[test]
    fn basic_getters() {
        let ip_network = return_test_ipv4_network();
        assert_eq!(ip_network.network_address(), Ipv4Addr::new(192, 168, 0, 0));
        assert_eq!(ip_network.netmask(), 16);
        assert_eq!(
            ip_network.broadcast_address(),
            Ipv4Addr::new(192, 168, 255, 255)
        );
        assert_eq!(ip_network.full_netmask(), Ipv4Addr::new(255, 255, 0, 0));
        assert_eq!(
            ip_network.supernet(),
            Some(Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 15).unwrap())
        );
        assert_eq!(ip_network.hosts().len(), 256 * 256 - 2);
    }

    #[test]
    fn host_network_without_hosts() {
        let ip = Ipv4Addr::new(127, 0, 0, 1);
        let ip_network = Ipv4Network::new(ip, 32).unwrap();
        assert_eq!(0, ip_network.hosts().len());
    }

    #[test]
    fn supernet_none() {
        let ipv4_network = Ipv4Network::new(Ipv4Addr::new(0, 0, 0, 0), 0).unwrap();
        assert_eq!(None, ipv4_network.supernet());
    }

    #[test]
    fn iterator() {
        let ip_network = return_test_ipv4_network();
        assert_eq!(ip_network.into_iter().len(), 256 * 256);
    }

    #[test]
    fn iterator_for() {
        let mut i = 0;
        for _ in return_test_ipv4_network() {
            i += 1;
        }
        assert_eq!(i, 256 * 256);
    }

    #[test]
    fn contains() {
        let ip_network = return_test_ipv4_network();
        assert!(!ip_network.contains(Ipv4Addr::new(192, 167, 255, 255)));
        assert!(ip_network.contains(Ipv4Addr::new(192, 168, 0, 0)));
        assert!(ip_network.contains(Ipv4Addr::new(192, 168, 255, 255)));
        assert!(!ip_network.contains(Ipv4Addr::new(192, 169, 0, 0)));
    }

    #[test]
    fn subnets() {
        let ip_network = return_test_ipv4_network();
        let mut subnets = ip_network.subnets();
        assert_eq!(subnets.len(), 2);
        assert_eq!(
            subnets.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 17).unwrap()
        );
        assert_eq!(
            subnets.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(192, 168, 128, 0), 17).unwrap()
        );
        assert!(subnets.next().is_none());
    }

    #[test]
    fn subnets_none() {
        let ipv4_network = Ipv4Network::new(Ipv4Addr::new(0, 0, 0, 0), 32).unwrap();
        assert_eq!(0, ipv4_network.subnets().len());
    }

    #[test]
    fn subnets_with_prefix() {
        let ip_network = return_test_ipv4_network();
        let mut subnets = ip_network.subnets_with_prefix(18);
        assert_eq!(subnets.len(), 4);
        assert_eq!(
            subnets.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 18).unwrap()
        );
        assert_eq!(
            subnets.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(192, 168, 64, 0), 18).unwrap()
        );
        assert_eq!(
            subnets.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(192, 168, 128, 0), 18).unwrap()
        );
        assert_eq!(
            subnets.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(192, 168, 192, 0), 18).unwrap()
        );
        assert!(subnets.next().is_none());
    }

    #[test]
    fn parse() {
        let ip_network: Ipv4Network = "192.168.0.0/16".parse().unwrap();
        assert_eq!(ip_network, return_test_ipv4_network());
    }

    #[test]
    fn format() {
        let ip_network = return_test_ipv4_network();
        assert_eq!(ip_network.to_string(), "192.168.0.0/16");
    }

    #[test]
    fn cmd_different_ip() {
        let a = Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 8).unwrap();
        let b = Ipv4Network::new(Ipv4Addr::new(128, 0, 0, 0), 8).unwrap();
        assert!(b > a);
    }

    #[test]
    fn cmd_different_netmask() {
        let a = Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 8).unwrap();
        let b = Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 16).unwrap();
        assert!(b > a);
    }

    #[test]
    fn is_private() {
        let is_private = |ip, netmask| Ipv4Network::new(ip, netmask).unwrap().is_private();

        assert!(is_private(Ipv4Addr::new(10, 0, 0, 0), 8));
        assert!(!is_private(Ipv4Addr::new(10, 0, 0, 0), 7));
        assert!(is_private(Ipv4Addr::new(10, 0, 0, 0), 32));
        assert!(!is_private(Ipv4Addr::new(11, 0, 0, 0), 32));

        assert!(is_private(Ipv4Addr::new(172, 16, 0, 0), 12));
        assert!(is_private(Ipv4Addr::new(172, 16, 0, 0), 32));
        assert!(is_private(Ipv4Addr::new(172, 31, 255, 255), 32));
        assert!(!is_private(Ipv4Addr::new(172, 32, 0, 0), 32));

        assert!(is_private(Ipv4Addr::new(192, 168, 0, 0), 16));
        assert!(is_private(Ipv4Addr::new(192, 168, 0, 0), 32));
        assert!(!is_private(Ipv4Addr::new(192, 168, 0, 0), 15));
    }

    #[test]
    fn is_global() {
        let is_global = |ip, netmask| Ipv4Network::new(ip, netmask).unwrap().is_global();

        assert!(is_global(Ipv4Addr::new(0, 0, 0, 0), 4));
        assert!(!is_global(Ipv4Addr::new(0, 0, 0, 0), 8));
        assert!(!is_global(Ipv4Addr::new(0, 0, 0, 0), 16));

        assert!(!is_global(Ipv4Addr::new(100, 64, 0, 0), 10)); // Shared Address Space
        assert!(!is_global(Ipv4Addr::new(100, 127, 0, 0), 16)); // Shared Address Space

        assert!(!is_global(Ipv4Addr::new(10, 0, 0, 0), 8));
        assert!(is_global(Ipv4Addr::new(10, 0, 0, 0), 7));
        assert!(!is_global(Ipv4Addr::new(10, 0, 0, 0), 32));
        assert!(is_global(Ipv4Addr::new(11, 0, 0, 0), 32));

        assert!(!is_global(Ipv4Addr::new(172, 16, 0, 0), 12));
        assert!(!is_global(Ipv4Addr::new(172, 16, 0, 0), 32));
        assert!(!is_global(Ipv4Addr::new(172, 31, 255, 255), 32));
        assert!(is_global(Ipv4Addr::new(172, 32, 0, 0), 32));

        assert!(!is_global(Ipv4Addr::new(192, 168, 0, 0), 16));
        assert!(!is_global(Ipv4Addr::new(192, 168, 0, 0), 32));
        assert!(is_global(Ipv4Addr::new(192, 168, 0, 0), 15));

        assert!(!is_global(Ipv4Addr::new(127, 0, 0, 0), 8));
        assert!(!is_global(Ipv4Addr::new(169, 254, 0, 0), 16));
        assert!(!is_global(Ipv4Addr::new(255, 255, 255, 255), 32));
        assert!(!is_global(Ipv4Addr::new(192, 0, 2, 0), 24));
        assert!(!is_global(Ipv4Addr::new(198, 51, 100, 0), 24));
        assert!(!is_global(Ipv4Addr::new(203, 0, 113, 0), 24));

        assert!(!is_global(Ipv4Addr::new(198, 18, 0, 0), 15)); // benchmarking
        assert!(!is_global(Ipv4Addr::new(198, 19, 0, 0), 16)); // benchmarking

        assert!(!is_global(Ipv4Addr::new(240, 0, 0, 0), 4));
        assert!(!is_global(Ipv4Addr::new(240, 0, 0, 0), 8));
        assert!(!is_global(Ipv4Addr::new(255, 0, 0, 0), 8));

        assert!(!is_global(Ipv4Addr::new(192, 0, 0, 0), 24)); // IETF Protocol Assignments
    }

    #[test]
    fn hash_same_for_same_input() {
        use std::hash::{Hash, Hasher};
        use std::collections::hash_map::DefaultHasher;

        let mut hasher = DefaultHasher::new();
        let ip = Ipv4Addr::new(127, 0, 0, 0);
        let network = Ipv4Network::new(ip, 8).unwrap();
        network.hash(&mut hasher);
        let first_hash = hasher.finish();

        let mut hasher = DefaultHasher::new();
        let ip = Ipv4Addr::new(127, 0, 0, 0);
        let network = Ipv4Network::new(ip, 8).unwrap();
        network.hash(&mut hasher);
        let second_hash = hasher.finish();

        assert_eq!(first_hash, second_hash);
    }

    #[test]
    fn summarize_address_range() {
        let networks = Ipv4Network::summarize_address_range(
            Ipv4Addr::new(194, 249, 198, 0),
            Ipv4Addr::new(194, 249, 198, 159),
        );
        assert_eq!(networks.len(), 2);
        assert_eq!(
            networks[0],
            Ipv4Network::new(Ipv4Addr::new(194, 249, 198, 0), 25).unwrap()
        );
        assert_eq!(
            networks[1],
            Ipv4Network::new(Ipv4Addr::new(194, 249, 198, 128), 27).unwrap()
        );
    }

    #[test]
    fn summarize_address_range_whole_range() {
        let networks = Ipv4Network::summarize_address_range(
            Ipv4Addr::new(0, 0, 0, 0),
            Ipv4Addr::new(255, 255, 255, 255),
        );
        assert_eq!(networks.len(), 1);
        assert_eq!(
            networks[0],
            Ipv4Network::new(Ipv4Addr::new(0, 0, 0, 0), 0).unwrap()
        );
    }

    #[test]
    fn summarize_address_range_first_is_bigger() {
        let networks = Ipv4Network::summarize_address_range(
            Ipv4Addr::new(255, 255, 255, 255),
            Ipv4Addr::new(0, 0, 0, 0),
        );
        assert_eq!(0, networks.len());
    }

    #[test]
    fn collapse_addresses() {
        let addresses = [
            Ipv4Network::from_str("192.0.2.0/26").unwrap(),
            Ipv4Network::from_str("192.0.2.64/26").unwrap(),
            Ipv4Network::from_str("192.0.2.128/26").unwrap(),
            Ipv4Network::from_str("192.0.2.192/26").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(1, collapsed.len());
        assert_eq!(Ipv4Network::from_str("192.0.2.0/24").unwrap(), collapsed[0]);
    }

    #[test]
    fn collapse_addresses_2() {
        let addresses = [
            Ipv4Network::from_str("192.0.2.0/25").unwrap(),
            Ipv4Network::from_str("192.0.2.128/25").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(1, collapsed.len());
        assert_eq!(Ipv4Network::from_str("192.0.2.0/24").unwrap(), collapsed[0]);
    }

    #[test]
    fn collapse_addresses_3() {
        // test only IP addresses including some duplicates
        let addresses = [
            Ipv4Network::from_str("1.1.1.0/32").unwrap(),
            Ipv4Network::from_str("1.1.1.1/32").unwrap(),
            Ipv4Network::from_str("1.1.1.2/32").unwrap(),
            Ipv4Network::from_str("1.1.1.3/32").unwrap(),
            Ipv4Network::from_str("1.1.1.4/32").unwrap(),
            Ipv4Network::from_str("1.1.1.0/32").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(2, collapsed.len());
        assert_eq!(Ipv4Network::from_str("1.1.1.0/30").unwrap(), collapsed[0]);
        assert_eq!(Ipv4Network::from_str("1.1.1.4/32").unwrap(), collapsed[1]);
    }

    #[test]
    fn collapse_addresses_4() {
        // test a mix of IP addresses and networks including some duplicates
        let addresses = [
            Ipv4Network::from_str("1.1.1.0/32").unwrap(),
            Ipv4Network::from_str("1.1.1.1/32").unwrap(),
            Ipv4Network::from_str("1.1.1.2/32").unwrap(),
            Ipv4Network::from_str("1.1.1.3/32").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(1, collapsed.len());
        assert_eq!(Ipv4Network::from_str("1.1.1.0/30").unwrap(), collapsed[0]);
    }

    #[test]
    fn collapse_addresses_5() {
        // test only IP networks
        let addresses = [
            Ipv4Network::from_str("1.1.0.0/24").unwrap(),
            Ipv4Network::from_str("1.1.1.0/24").unwrap(),
            Ipv4Network::from_str("1.1.2.0/24").unwrap(),
            Ipv4Network::from_str("1.1.3.0/24").unwrap(),
            Ipv4Network::from_str("1.1.4.0/24").unwrap(),
            Ipv4Network::from_str("1.1.0.0/22").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(2, collapsed.len());
        assert_eq!(Ipv4Network::from_str("1.1.0.0/22").unwrap(), collapsed[0]);
        assert_eq!(Ipv4Network::from_str("1.1.4.0/24").unwrap(), collapsed[1]);
    }

    #[test]
    fn collapse_addresses_5_order() {
        let addresses = [
            Ipv4Network::from_str("1.1.3.0/24").unwrap(),
            Ipv4Network::from_str("1.1.4.0/24").unwrap(),
            Ipv4Network::from_str("1.1.0.0/22").unwrap(),
            Ipv4Network::from_str("1.1.2.0/24").unwrap(),
            Ipv4Network::from_str("1.1.0.0/24").unwrap(),
            Ipv4Network::from_str("1.1.1.0/24").unwrap(),
        ];

        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(2, collapsed.len());
        assert_eq!(Ipv4Network::from_str("1.1.0.0/22").unwrap(), collapsed[0]);
        assert_eq!(Ipv4Network::from_str("1.1.4.0/24").unwrap(), collapsed[1]);
    }

    #[test]
    fn collapse_addresses_6() {
        //  test that two addresses are supernet'ed properly
        let addresses = [
            Ipv4Network::from_str("1.1.0.0/24").unwrap(),
            Ipv4Network::from_str("1.1.1.0/24").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(1, collapsed.len());
        assert_eq!(Ipv4Network::from_str("1.1.0.0/23").unwrap(), collapsed[0]);
    }

    #[test]
    fn collapse_addresses_7() {
        // test same IP networks
        let addresses = [
            Ipv4Network::from_str("1.1.1.1/32").unwrap(),
            Ipv4Network::from_str("1.1.1.1/32").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(1, collapsed.len());
        assert_eq!(Ipv4Network::from_str("1.1.1.1/32").unwrap(), collapsed[0]);
    }

    #[test]
    fn collapse_addresses_8() {
        let addresses = [
            Ipv4Network::from_str("0.0.0.0/0").unwrap(),
            Ipv4Network::from_str("1.1.1.1/32").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(1, collapsed.len());
        assert_eq!(Ipv4Network::from_str("0.0.0.0/0").unwrap(), collapsed[0]);
    }

    #[test]
    fn collapse_addresses_9() {
        let addresses = [
            Ipv4Network::from_str("1.228.0.0/16").unwrap(),
            Ipv4Network::from_str("1.230.0.0/15").unwrap(),
        ];
        let collapsed = Ipv4Network::collapse_addresses(&addresses);
        assert_eq!(2, collapsed.len());
    }

    #[test]
    fn from_ipv4addr() {
        let ip = Ipv4Addr::new(127, 0, 0, 1);
        let ipv4_network = Ipv4Network::from(ip);
        assert_eq!(ip, ipv4_network.network_address());
        assert_eq!(32, ipv4_network.netmask());
    }

    #[test]
    fn hash() {
        let network1 = Ipv4Network::from_str("192.0.2.0/26").unwrap();
        let network2 = Ipv4Network::from_str("192.0.2.64/26").unwrap();

        let mut hasher1 = DefaultHasher::new();
        network1.hash(&mut hasher1);

        let mut hasher2 = DefaultHasher::new();
        network2.hash(&mut hasher2);

        assert_ne!(hasher1.finish(), hasher2.finish());
    }
}
