use std::cmp::Ordering;
use std::fmt;
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
use std::str::FromStr;
use crate::{IpNetworkError, IpNetworkParseError};
use crate::helpers;
use crate::{Ipv4Network, Ipv6Network};

/// Holds IPv4 or IPv6 network.
#[derive(Clone, Copy, Eq, PartialEq, Debug, Hash, PartialOrd, Ord)]
pub enum IpNetwork {
    V4(Ipv4Network),
    V6(Ipv6Network),
}

impl IpNetwork {
    /// Constructs new `IpNetwork` based on [`IpAddr`] and `netmask`.
    ///
    /// [`IpAddr`]: https://doc.rust-lang.org/std/net/enum.IpAddr.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::{IpAddr, Ipv4Addr};
    /// use std::str::FromStr;
    /// use ip_network::{IpNetwork, Ipv4Network};
    ///
    /// let network_address = IpAddr::V4(Ipv4Addr::new(192, 168, 1, 0));
    /// let ip_network = IpNetwork::new(network_address, 24)?;
    /// assert_eq!(ip_network.network_address(), IpAddr::V4(Ipv4Addr::new(192, 168, 1, 0)));
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    #[allow(clippy::new_ret_no_self)]
    pub fn new<I: Into<IpAddr>>(network_address: I, netmask: u8) -> Result<Self, IpNetworkError> {
        Ok(match network_address.into() {
            IpAddr::V4(ip) => IpNetwork::V4(Ipv4Network::new(ip, netmask)?),
            IpAddr::V6(ip) => IpNetwork::V6(Ipv6Network::new(ip, netmask)?),
        })
    }

    /// Constructs new `IpNetwork` based on [`IpAddr`] and `netmask` with truncating host bits
    /// from given `network_address`.
    ///
    /// Returns error if netmask is bigger than 32 for IPv4 and 128 for IPv6.
    ///
    /// [`Ipv4Addr`]: https://doc.rust-lang.org/std/net/struct.IpAddr.html
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::{IpAddr, Ipv4Addr};
    /// use ip_network::IpNetwork;
    ///
    /// let network_address = IpAddr::V4(Ipv4Addr::new(192, 168, 1, 128));
    /// let ip_network = IpNetwork::new_truncate(network_address, 24)?;
    /// assert_eq!(ip_network.network_address(), IpAddr::V4(Ipv4Addr::new(192, 168, 1, 0)));
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn new_truncate<I: Into<IpAddr>>(
        network_address: I,
        netmask: u8,
    ) -> Result<Self, IpNetworkError> {
        Ok(match network_address.into() {
            IpAddr::V4(ip) => IpNetwork::V4(Ipv4Network::new_truncate(ip, netmask)?),
            IpAddr::V6(ip) => IpNetwork::V6(Ipv6Network::new_truncate(ip, netmask)?),
        })
    }

    /// Returns network IP address.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::{IpAddr, Ipv4Addr};
    /// use ip_network::IpNetwork;
    ///
    /// let ip_network = IpNetwork::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert_eq!(ip_network.network_address(), IpAddr::V4(Ipv4Addr::new(192, 168, 1, 0)));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn network_address(&self) -> IpAddr {
        match self {
            IpNetwork::V4(ip_network) => IpAddr::V4(ip_network.network_address()),
            IpNetwork::V6(ip_network) => IpAddr::V6(ip_network.network_address()),
        }
    }

    /// Returns network mask as integer.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::{IpAddr, Ipv4Addr};
    /// use ip_network::IpNetwork;
    ///
    /// let ip_network = IpNetwork::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert_eq!(ip_network.netmask(), 24);
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn netmask(&self) -> u8 {
        match self {
            IpNetwork::V4(ip_network) => ip_network.netmask(),
            IpNetwork::V6(ip_network) => ip_network.netmask(),
        }
    }

    /// Returns `true` if `IpNetwork` contains `Ipv4Network` struct.
    pub fn is_ipv4(&self) -> bool {
        match self {
            IpNetwork::V4(_) => true,
            IpNetwork::V6(_) => false,
        }
    }

    /// Returns `true` if `IpNetwork` contains `Ipv6Network` struct.
    pub fn is_ipv6(&self) -> bool {
        !self.is_ipv4()
    }

    /// Returns `true` if `IpNetwork` contains `IpAddr`. For different network type
    /// (for example IpNetwork is IPv6 and IpAddr is IPv4) always returns `false`.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
    /// use ip_network::IpNetwork;
    ///
    /// let ip_network = IpNetwork::new(Ipv4Addr::new(192, 168, 1, 0), 24)?;
    /// assert!(ip_network.contains(Ipv4Addr::new(192, 168, 1, 25)));
    /// assert!(!ip_network.contains(Ipv6Addr::new(0x2001, 0xdb8, 0, 0, 0, 1, 0, 0)));
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    pub fn contains<I: Into<IpAddr>>(&self, ip: I) -> bool {
        match (self, ip.into()) {
            (IpNetwork::V4(network), IpAddr::V4(ip)) => network.contains(ip),
            (IpNetwork::V6(network), IpAddr::V6(ip)) => network.contains(ip),
            _ => false,
        }
    }

    /// Returns `true` if the network is default route, that contains all IP addresses.
    pub fn is_default_route(&self) -> bool {
        match self {
            IpNetwork::V4(ip_network) => ip_network.is_default_route(),
            IpNetwork::V6(ip_network) => ip_network.is_default_route(),
        }
    }

    /// Returns `true` if the network is part of multicast network range.
    pub fn is_multicast(&self) -> bool {
        match self {
            IpNetwork::V4(ip_network) => ip_network.is_multicast(),
            IpNetwork::V6(ip_network) => ip_network.is_multicast(),
        }
    }

    /// Returns `true` if this is a part of network reserved for documentation.
    pub fn is_documentation(&self) -> bool {
        match self {
            IpNetwork::V4(ip_network) => ip_network.is_documentation(),
            IpNetwork::V6(ip_network) => ip_network.is_documentation(),
        }
    }

    /// Returns `true` if this network is inside loopback address range.
    pub fn is_loopback(&self) -> bool {
        match self {
            IpNetwork::V4(ip_network) => ip_network.is_loopback(),
            IpNetwork::V6(ip_network) => ip_network.is_loopback(),
        }
    }

    /// Returns `true` if the network appears to be globally routable.
    pub fn is_global(&self) -> bool {
        match self {
            IpNetwork::V4(ip_network) => ip_network.is_global(),
            IpNetwork::V6(ip_network) => ip_network.is_global(),
        }
    }

    /// Converts string in format IPv4 (X.X.X.X/Y) or IPv6 (X:X::X/Y) CIDR notation to `IpNetwork`.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::{IpNetwork, Ipv4Network};
    ///
    /// let ip_network = IpNetwork::from_str_truncate("192.168.1.1/24").unwrap();
    /// assert_eq!(ip_network, IpNetwork::V4(Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24).unwrap()));
    /// ```
    pub fn from_str_truncate(s: &str) -> Result<Self, IpNetworkParseError> {
        let (ip, netmask) =
            helpers::split_ip_netmask(s).ok_or(IpNetworkParseError::InvalidFormatError)?;

        let network_address =
            IpAddr::from_str(ip).map_err(|_| IpNetworkParseError::AddrParseError)?;
        let netmask =
            u8::from_str(netmask).map_err(|_| IpNetworkParseError::InvalidNetmaskFormat)?;

        IpNetwork::new_truncate(network_address, netmask)
            .map_err(IpNetworkParseError::IpNetworkError)
    }

    /// Return an iterator of the collapsed IpNetworks.
    pub fn collapse_addresses(addresses: &[Self]) -> Vec<Self> {
        let mut ipv4_networks = vec![];
        let mut ipv6_networks = vec![];
        for address in addresses {
            match address {
                IpNetwork::V4(ip_network) => ipv4_networks.push(*ip_network),
                IpNetwork::V6(ip_network) => ipv6_networks.push(*ip_network),
            }
        }

        let mut collapsed = Ipv4Network::collapse_addresses(&ipv4_networks)
            .into_iter()
            .map(IpNetwork::from)
            .collect::<Vec<_>>();
        collapsed.extend(
            Ipv6Network::collapse_addresses(&ipv6_networks)
                .into_iter()
                .map(IpNetwork::from),
        );
        collapsed
    }
}

impl fmt::Display for IpNetwork {
    /// Converts `IpNetwork` to string in format X.X.X.X/Y for IPv4 and X:X::X/Y for IPv6 (CIDR notation).
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::{IpNetwork, Ipv4Network};
    ///
    /// let ip_network = IpNetwork::V4(Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24)?);
    /// assert_eq!(ip_network.to_string(), "192.168.1.0/24");
    /// # Ok::<(), ip_network::IpNetworkError>(())
    /// ```
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            IpNetwork::V4(ref network) => network.fmt(f),
            IpNetwork::V6(ref network) => network.fmt(f),
        }
    }
}

impl FromStr for IpNetwork {
    type Err = IpNetworkParseError;

    /// Converts string in format IPv4 (X.X.X.X/Y) or IPv6 (X:X::X/Y) CIDR notation to `IpNetwork`.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use std::str::FromStr;
    /// use ip_network::{IpNetwork, Ipv4Network};
    ///
    /// let ip_network = IpNetwork::from_str("192.168.1.0/24").unwrap();
    /// assert_eq!(ip_network, IpNetwork::V4(Ipv4Network::new(Ipv4Addr::new(192, 168, 1, 0), 24).unwrap()));
    /// ```
    fn from_str(s: &str) -> Result<IpNetwork, IpNetworkParseError> {
        let (ip, netmask) =
            helpers::split_ip_netmask(s).ok_or(IpNetworkParseError::InvalidFormatError)?;

        let network_address =
            IpAddr::from_str(ip).map_err(|_| IpNetworkParseError::AddrParseError)?;
        let netmask =
            u8::from_str(netmask).map_err(|_| IpNetworkParseError::InvalidNetmaskFormat)?;

        IpNetwork::new(network_address, netmask).map_err(IpNetworkParseError::IpNetworkError)
    }
}

impl From<Ipv4Addr> for IpNetwork {
    /// Converts `Ipv4Addr` to `IpNetwork` with netmask 32.
    #[inline]
    fn from(ip: Ipv4Addr) -> Self {
        IpNetwork::V4(Ipv4Network::from(ip))
    }
}

impl From<Ipv6Addr> for IpNetwork {
    /// Converts `Ipv6Addr` to `IpNetwork` with netmask 128.
    #[inline]
    fn from(ip: Ipv6Addr) -> Self {
        IpNetwork::V6(Ipv6Network::from(ip))
    }
}

impl From<IpAddr> for IpNetwork {
    /// Converts `IpAddr` to `IpNetwork` with netmask 32 for IPv4 address and 128 for IPv6 address.
    fn from(ip: IpAddr) -> Self {
        match ip {
            IpAddr::V4(ip) => IpNetwork::from(ip),
            IpAddr::V6(ip) => IpNetwork::from(ip),
        }
    }
}

impl From<Ipv4Network> for IpNetwork {
    #[inline]
    fn from(network: Ipv4Network) -> Self {
        IpNetwork::V4(network)
    }
}

impl From<Ipv6Network> for IpNetwork {
    #[inline]
    fn from(network: Ipv6Network) -> Self {
        IpNetwork::V6(network)
    }
}

impl PartialEq<Ipv4Network> for IpNetwork {
    fn eq(&self, other: &Ipv4Network) -> bool {
        match self {
            IpNetwork::V4(v4) => v4 == other,
            IpNetwork::V6(_) => false,
        }
    }
}

impl PartialEq<Ipv6Network> for IpNetwork {
    fn eq(&self, other: &Ipv6Network) -> bool {
        match self {
            IpNetwork::V4(_) => false,
            IpNetwork::V6(v6) => v6 == other,
        }
    }
}

impl PartialEq<IpNetwork> for Ipv4Network {
    fn eq(&self, other: &IpNetwork) -> bool {
        match other {
            IpNetwork::V4(v4) => self == v4,
            IpNetwork::V6(_) => false,
        }
    }
}

impl PartialEq<IpNetwork> for Ipv6Network {
    fn eq(&self, other: &IpNetwork) -> bool {
        match other {
            IpNetwork::V4(_) => false,
            IpNetwork::V6(v6) => self == v6,
        }
    }
}

impl PartialOrd<Ipv4Network> for IpNetwork {
    fn partial_cmp(&self, other: &Ipv4Network) -> Option<Ordering> {
        match self {
            IpNetwork::V4(v4) => v4.partial_cmp(other),
            IpNetwork::V6(_) => Some(Ordering::Greater),
        }
    }
}

impl PartialOrd<IpNetwork> for Ipv4Network {
    fn partial_cmp(&self, other: &IpNetwork) -> Option<Ordering> {
        match other {
            IpNetwork::V4(v4) => self.partial_cmp(v4),
            IpNetwork::V6(_) => Some(Ordering::Less),
        }
    }
}

impl PartialOrd<Ipv6Network> for IpNetwork {
    fn partial_cmp(&self, other: &Ipv6Network) -> Option<Ordering> {
        match self {
            IpNetwork::V4(_) => Some(Ordering::Less),
            IpNetwork::V6(v6) => v6.partial_cmp(other),
        }
    }
}

impl PartialOrd<IpNetwork> for Ipv6Network {
    fn partial_cmp(&self, other: &IpNetwork) -> Option<Ordering> {
        match other {
            IpNetwork::V4(_) => Some(Ordering::Greater),
            IpNetwork::V6(v6) => self.partial_cmp(v6),
        }
    }
}

#[cfg(test)]
mod tests {
    use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
    use crate::{IpNetwork, IpNetworkParseError, IpNetworkError, Ipv4Network, Ipv6Network};
    use std::str::FromStr;

    fn return_test_ipv4_network() -> Ipv4Network {
        Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 16).unwrap()
    }

    fn return_test_ipv6_network() -> Ipv6Network {
        Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0, 0, 0, 0, 0, 0), 32).unwrap()
    }

    #[test]
    fn network_address_ipv4() {
        let ip_network = IpNetwork::V4(return_test_ipv4_network());
        assert_eq!(
            IpAddr::V4(Ipv4Addr::new(192, 168, 0, 0)),
            ip_network.network_address()
        );
    }

    #[test]
    fn network_address_ipv6() {
        let ip_network = IpNetwork::V6(return_test_ipv6_network());
        assert_eq!(
            IpAddr::V6(Ipv6Addr::new(0x2001, 0xdb8, 0, 0, 0, 0, 0, 0)),
            ip_network.network_address()
        );
    }

    #[test]
    fn is_ipv4() {
        let ip_network = IpNetwork::V4(return_test_ipv4_network());
        assert!(ip_network.is_ipv4());
        assert!(!ip_network.is_ipv6());
    }

    #[test]
    fn is_ipv6() {
        let ip_network = IpNetwork::V6(return_test_ipv6_network());
        assert!(ip_network.is_ipv6());
        assert!(!ip_network.is_ipv4());
    }

    #[test]
    fn parse_ipv4() {
        let ip_network: IpNetwork = "192.168.0.0/16".parse().unwrap();
        assert_eq!(ip_network, IpNetwork::V4(return_test_ipv4_network()));
    }

    #[test]
    fn parse_ipv6() {
        let ip_network: IpNetwork = "2001:db8::/32".parse().unwrap();
        assert_eq!(ip_network, IpNetwork::V6(return_test_ipv6_network()));
    }

    #[test]
    fn parse_empty() {
        let ip_network = "".parse::<IpNetwork>();
        assert!(ip_network.is_err());
        assert_eq!(
            IpNetworkParseError::InvalidFormatError,
            ip_network.unwrap_err()
        );
    }

    #[test]
    fn parse_invalid_netmask() {
        let ip_network = "192.168.0.0/a".parse::<IpNetwork>();
        assert!(ip_network.is_err());
        assert_eq!(
            IpNetworkParseError::InvalidNetmaskFormat,
            ip_network.unwrap_err()
        );
    }

    #[test]
    fn parse_invalid_ip() {
        let ip_network = "192.168.0.0a/16".parse::<IpNetwork>();
        assert!(ip_network.is_err());
        assert_eq!(IpNetworkParseError::AddrParseError, ip_network.unwrap_err());
    }

    #[test]
    fn parse_ipv4_host_bits_set() {
        let ip_network = "192.168.0.1/16".parse::<IpNetwork>();
        assert!(ip_network.is_err());
        assert_eq!(
            IpNetworkParseError::IpNetworkError(IpNetworkError::HostBitsSet),
            ip_network.unwrap_err()
        );
    }

    #[test]
    fn parse_ipv6_host_bits_set() {
        let ip_network = "2001:db8::1/32".parse::<IpNetwork>();
        assert!(ip_network.is_err());
        assert_eq!(
            IpNetworkParseError::IpNetworkError(IpNetworkError::HostBitsSet),
            ip_network.unwrap_err()
        );
    }

    #[test]
    fn format_ipv4() {
        let ip_network = IpNetwork::V4(return_test_ipv4_network());
        assert_eq!(ip_network.to_string(), "192.168.0.0/16");
    }

    #[test]
    fn format_ipv6() {
        let ip_network = IpNetwork::V6(return_test_ipv6_network());
        assert_eq!(ip_network.to_string(), "2001:db8::/32");
    }

    #[test]
    fn from_ipv4addr() {
        let ipv4addr = Ipv4Addr::new(1, 2, 3, 4);
        let ip_network = IpNetwork::from(ipv4addr);
        assert_eq!(IpAddr::V4(ipv4addr), ip_network.network_address());
        assert_eq!(32, ip_network.netmask());
    }

    #[test]
    fn from_ipv6addr() {
        let ipv6addr = Ipv6Addr::new(0x2001, 0xdb8, 0, 0, 0, 0, 0, 0);
        let ip_network = IpNetwork::from(ipv6addr);
        assert_eq!(IpAddr::V6(ipv6addr), ip_network.network_address());
        assert_eq!(128, ip_network.netmask());
    }

    #[test]
    fn from_ipaddr() {
        let ipaddr = IpAddr::V6(Ipv6Addr::new(0x2001, 0xdb8, 0, 0, 0, 0, 0, 0));
        let ip_network = IpNetwork::from(ipaddr);
        assert_eq!(ipaddr, ip_network.network_address());
        assert_eq!(128, ip_network.netmask());
    }

    #[test]
    fn from_ipv4_network() {
        let ipv4_network = return_test_ipv4_network();
        let ip_network = IpNetwork::from(ipv4_network);
        assert_eq!(
            IpAddr::V4(ipv4_network.network_address()),
            ip_network.network_address()
        );
        assert_eq!(ipv4_network.netmask(), ip_network.netmask());
    }

    #[test]
    fn from_ipv6_network() {
        let ipv6_network = return_test_ipv6_network();
        let ip_network = IpNetwork::from(ipv6_network);
        assert_eq!(
            IpAddr::V6(ipv6_network.network_address()),
            ip_network.network_address()
        );
        assert_eq!(ipv6_network.netmask(), ip_network.netmask());
    }

    #[test]
    fn equal_ip_network_ipv4_network() {
        let ip_network = IpNetwork::V4(return_test_ipv4_network());
        let ipv4_network = return_test_ipv4_network();
        let different = Ipv4Network::new(Ipv4Addr::new(1, 2, 3, 4), 32).unwrap();
        assert_eq!(ip_network, ipv4_network);
        assert_eq!(ipv4_network, ip_network);
        assert_ne!(ip_network, different);
        assert_ne!(different, ip_network);
        assert_ne!(ip_network, return_test_ipv6_network());
        assert_ne!(return_test_ipv6_network(), ip_network);
    }

    #[test]
    fn equal_ip_network_ipv6_network() {
        let ip_network = IpNetwork::V6(return_test_ipv6_network());
        let ipv6_network = return_test_ipv6_network();
        let different = Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 128).unwrap();
        assert_eq!(ip_network, ipv6_network);
        assert_eq!(ipv6_network, ip_network);
        assert_ne!(ip_network, different);
        assert_ne!(different, ip_network);
        assert_ne!(ip_network, return_test_ipv4_network());
        assert_ne!(return_test_ipv4_network(), ip_network);
    }

    #[test]
    fn compare_ip_network_ipv4_network_same() {
        let ip_network = IpNetwork::V4(return_test_ipv4_network());
        let ipv4_network = return_test_ipv4_network();
        assert!(ip_network <= ipv4_network);
        assert!(ip_network >= ipv4_network);
        assert!(ipv4_network <= ip_network);
        assert!(ipv4_network >= ip_network);
    }

    #[test]
    fn compare_ip_network_ipv6_network_same() {
        let ip_network = IpNetwork::V6(return_test_ipv6_network());
        let ipv6_network = return_test_ipv6_network();
        assert!(ip_network <= ipv6_network);
        assert!(ip_network >= ipv6_network);
        assert!(ipv6_network <= ip_network);
        assert!(ipv6_network >= ip_network);
    }

    #[test]
    fn compare_ip_network_v4_ip_network_v6() {
        let ip_network_v4 = IpNetwork::V4(return_test_ipv4_network());
        let ip_network_v6 = IpNetwork::V6(return_test_ipv6_network());
        assert!(ip_network_v4 < ip_network_v6);
        assert!(ip_network_v6 > ip_network_v4);
    }

    #[test]
    fn collapse_addresses() {
        let addresses: Vec<_> = [
            "192.0.2.0/25",
            "192.0.2.128/25",
            "2001::/100",
            "2001::/120",
            "2001::/96",
        ]
        .iter()
        .map(|i| IpNetwork::from_str(i).unwrap())
        .collect();
        let collapsed = IpNetwork::collapse_addresses(&addresses);
        assert_eq!(2, collapsed.len());
    }
}
