//! This crate provides storage and retrieval of IPv4 and IPv6 network prefixes.
//!
//! Currently, it uses `ip_network` crate, that provides IP network data structure and
//! `treebitmap` as backend, that provides fast lookup times, and a small memory footprint.
//! Backend can be changed in future releases.
//!
//! ## Examples
//!
//! ```rust
//! use std::net::{IpAddr, Ipv6Addr};
//! use ip_network::{IpNetwork, Ipv6Network};
//! use ip_network_table::IpNetworkTable;
//!
//! let mut table = IpNetworkTable::new();
//! let network = IpNetwork::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
//! let ip_address = Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0x1);
//!
//! assert_eq!(table.insert(network, "foo"), None);
//! // Get value for network from table
//! assert_eq!(table.longest_match(ip_address), Some((network, &"foo")));
//! ```

#![warn(rust_2018_idioms)]

use ip_network::{IpNetwork, Ipv4Network, Ipv6Network};
use ip_network_table_deps_treebitmap::IpLookupTable;
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr}; // forked from `treebitmap`

/// Table holding IPv4 and IPv6 network prefixes with value.
#[derive(Default)]
pub struct IpNetworkTable<T> {
    ipv4: IpLookupTable<Ipv4Addr, T>,
    ipv6: IpLookupTable<Ipv6Addr, T>,
}

impl<T> IpNetworkTable<T> {
    /// Constructs a new, empty `IpNetworkTable<T>`.
    pub fn new() -> Self {
        Self::with_capacity(0, 0)
    }

    /// Constructs a new, empty `IpNetworkTable<T>` with the specified capacity.
    pub fn with_capacity(ipv4_size: usize, ipv6_size: usize) -> Self {
        Self {
            ipv4: IpLookupTable::with_capacity(ipv4_size),
            ipv6: IpLookupTable::with_capacity(ipv6_size),
        }
    }

    /// Returns the number of elements in the table. First value is number of IPv4 networks and second is number of IPv6 networks.
    pub fn len(&self) -> (usize, usize) {
        (self.ipv4.len(), self.ipv6.len())
    }

    /// Returns `true` if table is empty.
    pub fn is_empty(&self) -> bool {
        self.ipv4.is_empty() && self.ipv6.is_empty()
    }

    /// Insert a value for the `IpNetwork`. If prefix existed previously, the old value is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::Ipv6Network;
    /// use std::net::Ipv6Addr;
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    ///
    /// assert_eq!(table.insert(network, "foo"), None);
    /// // Insert duplicate
    /// assert_eq!(table.insert(network, "bar"), Some("foo"));
    /// // Value is replaced
    /// assert_eq!(table.insert(network, "null"), Some("bar"));
    /// ```
    pub fn insert<N: Into<IpNetwork>>(&mut self, network: N, data: T) -> Option<T> {
        match network.into() {
            IpNetwork::V4(ipv4_network) => self.ipv4.insert(
                ipv4_network.network_address(),
                ipv4_network.netmask() as u32,
                data,
            ),
            IpNetwork::V6(ipv6_network) => self.ipv6.insert(
                ipv6_network.network_address(),
                ipv6_network.netmask() as u32,
                data,
            ),
        }
    }

    /// Remove a `IpNetwork` from table. If prefix existed, the value is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::Ipv6Network;
    /// use std::net::Ipv6Addr;
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    ///
    /// assert_eq!(table.insert(network, "foo"), None);
    /// // Remove network from table
    /// assert_eq!(table.remove(network), Some("foo"));
    /// // Network is removed
    /// assert_eq!(table.exact_match(network), None);
    /// ```
    pub fn remove<N: Into<IpNetwork>>(&mut self, network: N) -> Option<T> {
        match network.into() {
            IpNetwork::V4(ipv4_network) => self.ipv4.remove(
                ipv4_network.network_address(),
                ipv4_network.netmask() as u32,
            ),
            IpNetwork::V6(ipv6_network) => self.ipv6.remove(
                ipv6_network.network_address(),
                ipv6_network.netmask() as u32,
            ),
        }
    }

    /// Get pointer to value from table based on exact network match.
    /// If network is not in table, `None` is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::Ipv6Network;
    /// use std::net::Ipv6Addr;
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network_a = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// let network_b = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 128).unwrap();
    ///
    /// assert_eq!(table.insert(network_a, "foo"), None);
    /// // Get value for network from table
    /// assert_eq!(table.exact_match(network_a), Some(&"foo"));
    /// // Network B doesnt exists in table
    /// assert_eq!(table.exact_match(network_b), None);
    /// ```
    pub fn exact_match<N: Into<IpNetwork>>(&self, network: N) -> Option<&T> {
        match network.into() {
            IpNetwork::V4(ipv4_network) => self.ipv4.exact_match(
                ipv4_network.network_address(),
                ipv4_network.netmask() as u32,
            ),
            IpNetwork::V6(ipv6_network) => self.ipv6.exact_match(
                ipv6_network.network_address(),
                ipv6_network.netmask() as u32,
            ),
        }
    }

    /// Get mutable pointer to value from table based on exact network match.
    /// If network is not in table, `None` is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::Ipv6Network;
    /// use std::net::Ipv6Addr;
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network_a = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// let network_b = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 128).unwrap();
    ///
    /// assert_eq!(table.insert(network_a, "foo"), None);
    /// // Get value for network from table
    /// assert_eq!(table.exact_match_mut(network_a), Some(&mut "foo"));
    /// // Network B doesnt exists in table
    /// assert_eq!(table.exact_match(network_b), None);
    /// ```
    pub fn exact_match_mut<N: Into<IpNetwork>>(&mut self, network: N) -> Option<&mut T> {
        match network.into() {
            IpNetwork::V4(ipv4_network) => self.ipv4.exact_match_mut(
                ipv4_network.network_address(),
                ipv4_network.netmask() as u32,
            ),
            IpNetwork::V6(ipv6_network) => self.ipv6.exact_match_mut(
                ipv6_network.network_address(),
                ipv6_network.netmask() as u32,
            ),
        }
    }

    /// Find most specific IP network in table that contains given IP address. If no network in table contains
    /// given IP address, `None` is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::{IpNetwork, Ipv6Network};
    /// use std::net::{IpAddr, Ipv6Addr};
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network = IpNetwork::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// let ip_address = Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0x1);
    ///
    /// assert_eq!(table.insert(network, "foo"), None);
    /// // Get value for network from table
    /// assert_eq!(table.longest_match(ip_address), Some((network, &"foo")));
    /// ```
    pub fn longest_match<I: Into<IpAddr>>(&self, ip: I) -> Option<(IpNetwork, &T)> {
        match ip.into() {
            IpAddr::V4(ipv4) => self
                .longest_match_ipv4(ipv4)
                .map(|(n, t)| (IpNetwork::V4(n), t)),
            IpAddr::V6(ipv6) => self
                .longest_match_ipv6(ipv6)
                .map(|(n, t)| (IpNetwork::V6(n), t)),
        }
    }

    /// Find most specific IP network in table that contains given IP address. If no network in table contains
    /// given IP address, `None` is returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::{IpNetwork, Ipv6Network};
    /// use std::net::Ipv6Addr;
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network = IpNetwork::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// let ip_address = Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0x1);
    ///
    /// assert_eq!(table.insert(network, "foo"), None);
    /// // Get value for network from table
    /// assert_eq!(table.longest_match_mut(ip_address), Some((network, &mut "foo")));
    /// ```
    pub fn longest_match_mut<I: Into<IpAddr>>(&mut self, ip: I) -> Option<(IpNetwork, &mut T)> {
        match ip.into() {
            IpAddr::V4(ipv4) => self.ipv4.longest_match_mut(ipv4).map(|(addr, mask, data)| {
                (
                    IpNetwork::V4(Ipv4Network::new(addr, mask as u8).unwrap()),
                    data,
                )
            }),
            IpAddr::V6(ipv6) => self.ipv6.longest_match_mut(ipv6).map(|(addr, mask, data)| {
                (
                    IpNetwork::V6(Ipv6Network::new(addr, mask as u8).unwrap()),
                    data,
                )
            }),
        }
    }

    /// Specific version of `longest_match` for IPv4 address.
    #[inline]
    pub fn longest_match_ipv4(&self, ip: Ipv4Addr) -> Option<(Ipv4Network, &T)> {
        self.ipv4
            .longest_match(ip)
            .map(|(addr, mask, data)| (Ipv4Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Specific version of `longest_match` for IPv6 address.
    #[inline]
    pub fn longest_match_ipv6(&self, ip: Ipv6Addr) -> Option<(Ipv6Network, &T)> {
        self.ipv6
            .longest_match(ip)
            .map(|(addr, mask, data)| (Ipv6Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Find all IP networks in table that contains given IP address.
    /// Returns iterator of `IpNetwork` and reference to value.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::{IpNetwork, Ipv6Network};
    /// use std::net::Ipv6Addr;
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network = IpNetwork::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// let ip_address = Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0x1);
    ///
    /// assert_eq!(table.insert(network, "foo"), None);
    /// // Get value for network from table
    /// assert_eq!(table.matches(ip_address).count(), 1);
    /// ```
    pub fn matches<I: Into<IpAddr>>(
        &self,
        ip: I,
    ) -> Box<dyn Iterator<Item = (IpNetwork, &T)> + '_> {
        match ip.into() {
            IpAddr::V4(ipv4) => Box::new(
                self.matches_ipv4(ipv4)
                    .map(|(network, data)| (IpNetwork::V4(network), data)),
            ),
            IpAddr::V6(ipv6) => Box::new(
                self.matches_ipv6(ipv6)
                    .map(|(network, data)| (IpNetwork::V6(network), data)),
            ),
        }
    }

    /// Specific version of `matches` for IPv4 address.
    pub fn matches_ipv4(&self, ip: Ipv4Addr) -> impl Iterator<Item = (Ipv4Network, &T)> {
        self.ipv4
            .matches(ip)
            .map(|(addr, mask, data)| (Ipv4Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Specific version of `matches` for IPv6 address.
    pub fn matches_ipv6(&self, ip: Ipv6Addr) -> impl Iterator<Item = (Ipv6Network, &T)> {
        self.ipv6
            .matches(ip)
            .map(|(addr, mask, data)| (Ipv6Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Find all IP networks in table that contains given IP address.
    /// Returns iterator of `IpNetwork` and mutable reference to value.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::{IpNetwork, Ipv6Network};
    /// use std::net::{IpAddr, Ipv6Addr};
    ///
    /// let mut table = IpNetworkTable::new();
    /// let network = IpNetwork::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// let ip_address = Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0x1);
    ///
    /// assert_eq!(table.insert(network, "foo"), None);
    /// // Get value for network from table
    /// assert_eq!(table.matches_mut(ip_address).count(), 1);
    /// ```
    pub fn matches_mut<I: Into<IpAddr>>(
        &mut self,
        ip: I,
    ) -> Box<dyn Iterator<Item = (IpNetwork, &mut T)> + '_> {
        match ip.into() {
            IpAddr::V4(ipv4) => Box::new(
                self.matches_mut_ipv4(ipv4)
                    .map(|(network, data)| (IpNetwork::V4(network), data)),
            ),
            IpAddr::V6(ipv6) => Box::new(
                self.matches_mut_ipv6(ipv6)
                    .map(|(network, data)| (IpNetwork::V6(network), data)),
            ),
        }
    }

    /// Specific version of `matches_mut` for IPv4 address.
    #[inline]
    pub fn matches_mut_ipv4(
        &mut self,
        ip: Ipv4Addr,
    ) -> impl Iterator<Item = (Ipv4Network, &mut T)> {
        self.ipv4
            .matches_mut(ip)
            .map(|(addr, mask, data)| (Ipv4Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Specific version of `matches_mut` for IPv6 address.
    #[inline]
    pub fn matches_mut_ipv6(
        &mut self,
        ip: Ipv6Addr,
    ) -> impl Iterator<Item = (Ipv6Network, &mut T)> {
        self.ipv6
            .matches_mut(ip)
            .map(|(addr, mask, data)| (Ipv6Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Iterator for all networks in table, first are iterated IPv4 and then IPv6 networks. Order is not guaranteed.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::{IpNetwork, Ipv4Network, Ipv6Network};
    /// use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
    ///
    /// let mut table: IpNetworkTable<&str> = IpNetworkTable::new();
    /// let network_a = Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 24).unwrap();
    /// assert_eq!(table.insert(network_a, "foo"), None);
    /// let network_b = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// assert_eq!(table.insert(network_b, "foo"), None);
    ///
    /// let mut iterator = table.iter();
    /// assert_eq!(iterator.next(), Some((IpNetwork::V4(network_a), &"foo")));
    /// assert_eq!(iterator.next(), Some((IpNetwork::V6(network_b), &"foo")));
    /// assert_eq!(iterator.next(), None);
    /// ```
    pub fn iter(&self) -> impl Iterator<Item = (IpNetwork, &T)> {
        self.iter_ipv4()
            .map(|(network, data)| (IpNetwork::V4(network), data))
            .chain(
                self.iter_ipv6()
                    .map(|(network, data)| (IpNetwork::V6(network), data)),
            )
    }

    /// Mutable iterator for all networks in table, first are iterated IPv4 and then IPv6 networks. Order is not guaranteed.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::{IpNetwork, Ipv4Network, Ipv6Network};
    /// use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
    ///
    /// let mut table: IpNetworkTable<&str> = IpNetworkTable::new();
    /// let network_a = Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 24).unwrap();
    /// assert_eq!(table.insert(network_a, "foo"), None);
    /// let network_b = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// assert_eq!(table.insert(network_b, "foo"), None);
    ///
    /// let mut iterator = table.iter_mut();
    /// for (network, value) in iterator {
    ///    *value = "bar";
    /// }
    ///
    /// assert_eq!(table.exact_match(network_a), Some(&"bar"));
    /// assert_eq!(table.exact_match(network_b), Some(&"bar"));
    /// ```
    pub fn iter_mut(&mut self) -> impl Iterator<Item = (IpNetwork, &mut T)> {
        self.ipv4
            .iter_mut()
            .map(|(addr, mask, data)| (IpNetwork::new(addr, mask as u8).unwrap(), data))
            .chain(
                self.ipv6
                    .iter_mut()
                    .map(|(addr, mask, data)| (IpNetwork::new(addr, mask as u8).unwrap(), data)),
            )
    }

    /// Iterator for all IPv4 networks in table. Order is not guaranteed.
    pub fn iter_ipv4(&self) -> impl Iterator<Item = (Ipv4Network, &T)> {
        self.ipv4
            .iter()
            .map(|(addr, mask, data)| (Ipv4Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Iterator for all IPv6 networks in table. Order is not guaranteed.
    pub fn iter_ipv6(&self) -> impl Iterator<Item = (Ipv6Network, &T)> {
        self.ipv6
            .iter()
            .map(|(addr, mask, data)| (Ipv6Network::new(addr, mask as u8).unwrap(), data))
    }

    /// Retains only the elements specified by the predicate.
    ///
    /// In other words, remove all pairs `(k, v)` such that `f(ip_network, &mut v)` returns `false`.
    ///
    /// # Examples
    ///
    /// ```
    /// use ip_network_table::IpNetworkTable;
    /// use ip_network::{IpNetwork, Ipv4Network, Ipv6Network};
    /// use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
    ///
    /// let mut table: IpNetworkTable<&str> = IpNetworkTable::new();
    /// let network_a = Ipv4Network::new(Ipv4Addr::new(192, 168, 0, 0), 24).unwrap();
    /// assert_eq!(table.insert(network_a, "foo"), None);
    /// let network_b = Ipv6Network::new(Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0xbeef, 0, 0, 0, 0), 64).unwrap();
    /// assert_eq!(table.insert(network_b, "foo"), None);
    ///
    /// // Keep just IPv4 networks
    /// table.retain(|network, _| network.is_ipv4());
    ///
    /// assert_eq!(table.exact_match(network_a), Some(&"foo"));
    /// assert_eq!(table.exact_match(network_b), None);
    /// ```
    pub fn retain<F>(&mut self, mut f: F)
    where
        F: FnMut(IpNetwork, &mut T) -> bool,
    {
        let mut to_delete = vec![];
        for (network, data) in self.iter_mut() {
            if !f(network, data) {
                to_delete.push(network);
            }
        }
        for network in to_delete {
            self.remove(network);
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::IpNetworkTable;
    use ip_network::{Ipv4Network, Ipv6Network};
    use std::net::{Ipv4Addr, Ipv6Addr};

    #[test]
    fn insert_ipv4_ipv6() {
        let mut table = IpNetworkTable::new();
        table.insert(
            Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 16).unwrap(),
            1,
        );
        table.insert(
            Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 128).unwrap(),
            1,
        );
    }

    #[test]
    fn exact_match_ipv4() {
        let mut table = IpNetworkTable::new();
        table.insert(
            Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 16).unwrap(),
            1,
        );
        let m = table.exact_match(Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 16).unwrap());
        assert_eq!(m, Some(&1));
        let m = table.exact_match(Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 17).unwrap());
        assert_eq!(m, None);
        let m = table.exact_match(Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 15).unwrap());
        assert_eq!(m, None);
    }

    #[test]
    fn exact_match_ipv6() {
        let mut table = IpNetworkTable::new();
        table.insert(
            Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 127).unwrap(),
            1,
        );
        let m = table
            .exact_match(Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 127).unwrap());
        assert_eq!(m, Some(&1));
        let m = table
            .exact_match(Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 128).unwrap());
        assert_eq!(m, None);
        let m = table
            .exact_match(Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 5, 6, 7, 8), 126).unwrap());
        assert_eq!(m, None);
    }
}
