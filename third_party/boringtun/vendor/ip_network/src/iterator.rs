use std::net::{Ipv4Addr, Ipv6Addr};
use crate::{Ipv4Network, Ipv6Network};
use crate::helpers;

#[cfg(target_pointer_width = "16")]
const POINTER_WIDTH: u32 = 16;
#[cfg(target_pointer_width = "32")]
const POINTER_WIDTH: u32 = 32;
#[cfg(target_pointer_width = "64")]
const POINTER_WIDTH: u32 = 64;
#[cfg(target_pointer_width = "128")]
const POINTER_WIDTH: u32 = 128;

/// IPv4 range iterator.
pub struct Ipv4RangeIterator {
    current: u32,
    to: u32,
    is_done: bool,
}

impl Ipv4RangeIterator {
    /// Constructs new `Ipv4RangeIterator` for given range, both `from` and `to` address are inclusive.
    ///
    /// # Panics
    ///
    /// When `to` address is bigger or same than `from` address.
    ///
    /// # Examples
    ///
    /// ```
    /// use std::net::Ipv4Addr;
    /// use ip_network::iterator::Ipv4RangeIterator;
    ///
    /// let mut iterator = Ipv4RangeIterator::new(
    ///     Ipv4Addr::new(192, 168, 2, 0),
    ///     Ipv4Addr::new(192, 168, 2, 255)
    /// );
    /// assert_eq!(iterator.next().unwrap(), Ipv4Addr::new(192, 168, 2, 0));
    /// assert_eq!(iterator.next().unwrap(), Ipv4Addr::new(192, 168, 2, 1));
    /// assert_eq!(iterator.last().unwrap(), Ipv4Addr::new(192, 168, 2, 255));
    /// ```
    pub fn new(from: Ipv4Addr, to: Ipv4Addr) -> Self {
        let current = u32::from(from);
        let to = u32::from(to);
        assert!(to >= current);
        Self {
            current,
            to,
            is_done: false,
        }
    }

    /// Constructs new `Ipv4RangeIterator` that iterates host (without network and broadcast address)
    /// IPs in Ipv4Network.
    pub fn hosts(network: Ipv4Network) -> Self {
        if network.netmask() >= 31 {
            // Network doesn't contains any host IPs, create empty iterator.
            Self {
                current: 0,
                to: 0,
                is_done: true,
            }
        } else {
            let from = Ipv4Addr::from(u32::from(network.network_address()) + 1);
            let to = Ipv4Addr::from(u32::from(network.broadcast_address()) - 1);
            Self::new(from, to)
        }
    }
}

impl Iterator for Ipv4RangeIterator {
    type Item = Ipv4Addr;

    fn next(&mut self) -> Option<Self::Item> {
        if self.current <= self.to && !self.is_done {
            let output = self.current;

            match self.current.checked_add(1) {
                Some(x) => self.current = x,
                None => self.is_done = true,
            };

            Some(Self::Item::from(output))
        } else {
            None
        }
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        if self.is_done {
            return (0, Some(0));
        }

        let remaining = (self.to - self.current + 1) as usize;
        (remaining, Some(remaining))
    }
}

impl ExactSizeIterator for Ipv4RangeIterator {}

/// Iterates over new created IPv4 network from given network.
pub struct Ipv4NetworkIterator {
    current: u32,
    to: u32,
    new_netmask: u8,
    is_done: bool,
}

impl Ipv4NetworkIterator {
    /// Constructs new `Ipv4NetworkIterator`, that iterates over networks based on `network` and
    /// `new_netmask`. If network has already netmask 32 or when is the same as `new_netmask`, empty
    /// iterator is returned.
    ///
    /// # Panics
    ///
    /// When `new_netmask` is smaller than `network` netmask or when `net_netmask` is bigger than 32.
    pub fn new(network: Ipv4Network, new_netmask: u8) -> Self {
        assert!(new_netmask <= Ipv4Network::LENGTH);

        if network.netmask() == Ipv4Network::LENGTH || network.netmask() == new_netmask {
            return Self {
                current: 0,
                to: 0,
                new_netmask: 0,
                is_done: true,
            };
        }

        assert!(network.netmask() < new_netmask);

        let current = u32::from(network.network_address());
        let mask =
            !helpers::bite_mask(32 - (new_netmask - network.netmask())) << (32 - new_netmask);
        let to = current | mask;

        Self {
            current,
            to,
            new_netmask,
            is_done: false,
        }
    }

    fn step(&self) -> u32 {
        1 << (32 - self.new_netmask)
    }
}

impl Iterator for Ipv4NetworkIterator {
    type Item = Ipv4Network;

    fn next(&mut self) -> Option<Self::Item> {
        if self.current <= self.to && !self.is_done {
            let output = self.current;

            match self.current.checked_add(self.step()) {
                Some(x) => self.current = x,
                None => self.is_done = true,
            };

            Some(Self::Item {
                network_address: Ipv4Addr::from(output),
                netmask: self.new_netmask,
            })
        } else {
            None
        }
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        if self.is_done {
            return (0, Some(0));
        }

        let remaining = ((self.to - self.current) / self.step() + 1) as usize;
        (remaining, Some(remaining))
    }
}

impl ExactSizeIterator for Ipv4NetworkIterator {}

/// Iterates over new created IPv6 network from given network.
pub struct Ipv6NetworkIterator {
    current: u128,
    to: u128,
    new_netmask: u8,
    is_done: bool,
}

impl Ipv6NetworkIterator {
    /// Constructs new `Ipv6NetworkIterator`, that iterates over networks based on `network` and
    /// `new_netmask`. If network has already netmask 128 or when is the same as `new_netmask`, empty
    /// iterator is returned.
    ///
    /// # Panics
    ///
    /// When `new_netmask` is smaller than `network` netmask or when `net_netmask` is bigger than 128.
    pub fn new(network: Ipv6Network, new_netmask: u8) -> Self {
        assert!(new_netmask <= Ipv6Network::LENGTH);

        if network.netmask() == Ipv6Network::LENGTH || network.netmask() == new_netmask {
            return Self {
                current: 0,
                to: 0,
                new_netmask: 0,
                is_done: true,
            };
        }

        assert!(network.netmask() < new_netmask);

        let current = u128::from(network.network_address());
        let mask = !helpers::bite_mask_u128(128 - (new_netmask - network.netmask()))
            << (128 - new_netmask);
        let to = current | mask;

        Self {
            current,
            to,
            new_netmask,
            is_done: false,
        }
    }

    fn step(&self) -> u128 {
        1 << (128 - self.new_netmask)
    }

    pub fn real_len(&self) -> u128 {
        if self.is_done {
            return 0;
        }

        ((self.to - self.current) / self.step()).saturating_add(1)
    }
}

impl Iterator for Ipv6NetworkIterator {
    type Item = Ipv6Network;

    fn next(&mut self) -> Option<Self::Item> {
        if self.current <= self.to && !self.is_done {
            let output = self.current;

            match self.current.checked_add(self.step()) {
                Some(x) => self.current = x,
                None => self.is_done = true,
            };

            Some(Self::Item {
                network_address: Ipv6Addr::from(output),
                netmask: self.new_netmask,
            })
        } else {
            None
        }
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        let remaining = self.real_len();

        if 128 - remaining.leading_zeros() > POINTER_WIDTH {
            (usize::MAX, None)
        } else {
            let remaining_u64 = remaining as u64;
            (remaining_u64 as usize, Some(remaining_u64 as usize))
        }
    }
}

impl ExactSizeIterator for Ipv6NetworkIterator {}

#[cfg(test)]
mod tests {
    use std::net::{Ipv4Addr, Ipv6Addr};
    use crate::{Ipv4Network, Ipv6Network};
    use super::{Ipv4NetworkIterator, Ipv4RangeIterator, Ipv6NetworkIterator};

    #[test]
    fn ipv4_range_iterator() {
        let mut iterator = Ipv4RangeIterator::new(
            Ipv4Addr::new(192, 168, 2, 0),
            Ipv4Addr::new(192, 168, 2, 255),
        );
        assert_eq!(iterator.next().unwrap(), Ipv4Addr::new(192, 168, 2, 0));
        assert_eq!(iterator.next().unwrap(), Ipv4Addr::new(192, 168, 2, 1));
        assert_eq!(iterator.last().unwrap(), Ipv4Addr::new(192, 168, 2, 255));
    }

    #[test]
    fn ipv4_range_iterator_length() {
        let mut iterator = Ipv4RangeIterator::new(
            Ipv4Addr::new(192, 168, 2, 0),
            Ipv4Addr::new(192, 168, 2, 255),
        );
        assert_eq!(iterator.len(), 256);
        iterator.next().unwrap();
        assert_eq!(iterator.len(), 255);
        assert_eq!(iterator.collect::<Vec<_>>().len(), 255);
    }

    #[test]
    fn ipv4_range_iterator_same_values() {
        let mut iterator = Ipv4RangeIterator::new(
            Ipv4Addr::new(255, 255, 255, 255),
            Ipv4Addr::new(255, 255, 255, 255),
        );
        assert_eq!(iterator.len(), 1);
        assert_eq!(iterator.next().unwrap(), Ipv4Addr::new(255, 255, 255, 255));
        assert!(iterator.next().is_none());
        assert_eq!(iterator.len(), 0);
    }

    #[test]
    fn ipv4_network_iterator() {
        let network = Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 8).unwrap();
        let mut iterator = Ipv4NetworkIterator::new(network, 16);

        assert_eq!(iterator.len(), 256);
        assert_eq!(
            iterator.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 16).unwrap()
        );
        assert_eq!(
            iterator.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(127, 1, 0, 0), 16).unwrap()
        );
        assert_eq!(
            iterator.next().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(127, 2, 0, 0), 16).unwrap()
        );
        assert_eq!(
            iterator.last().unwrap(),
            Ipv4Network::new(Ipv4Addr::new(127, 255, 0, 0), 16).unwrap()
        );
    }

    #[test]
    fn ipv4_network_iterator_empty() {
        let network = Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 32).unwrap();
        let iterator = Ipv4NetworkIterator::new(network, 32);
        assert_eq!(0, iterator.len());
    }

    #[test]
    fn ipv6_network_iterator() {
        let ip = Ipv6Addr::new(0x2001, 0, 0, 0, 0, 0, 0, 0);
        let network = Ipv6Network::new(ip, 16).unwrap();
        let mut iterator = Ipv6NetworkIterator::new(network, 17);

        assert_eq!(2, iterator.len());
        assert_eq!(iterator.next().unwrap(), Ipv6Network::new(ip, 17).unwrap());
        assert_eq!(
            iterator.next().unwrap(),
            Ipv6Network::new(Ipv6Addr::new(0x2001, 0x8000, 0, 0, 0, 0, 0, 0), 17).unwrap()
        );
        assert!(iterator.next().is_none());
    }

    #[test]
    #[should_panic] // because range is bigger than `usize` on 64bit machine
    #[cfg(not(miri))] // currently, miri doesnt support should_panic tests
    fn ipv6_network_iterator_whole_range_len() {
        let ip = Ipv6Addr::new(0, 0, 0, 0, 0, 0, 0, 0);
        let network = Ipv6Network::new(ip, 0).unwrap();
        let iterator = Ipv6NetworkIterator::new(network, 128);

        iterator.len();
    }

    #[test]
    fn ipv6_network_iterator_whole_range_real_len() {
        let ip = Ipv6Addr::new(0, 0, 0, 0, 0, 0, 0, 0);
        let network = Ipv6Network::new(ip, 0).unwrap();
        let iterator = Ipv6NetworkIterator::new(network, 128);

        assert_eq!(iterator.real_len(), u128::MAX);
    }

    #[test]
    fn ipv6_network_iterator_whole_range() {
        let ip = Ipv6Addr::new(0, 0, 0, 0, 0, 0, 0, 0);
        let network = Ipv6Network::new(ip, 0).unwrap();
        let mut iterator = Ipv6NetworkIterator::new(network, 128);

        assert_eq!(
            iterator.next().unwrap(),
            Ipv6Network::new(Ipv6Addr::new(0, 0, 0, 0, 0, 0, 0, 0), 128).unwrap()
        );
        assert_eq!(
            iterator.next().unwrap(),
            Ipv6Network::new(Ipv6Addr::new(0, 0, 0, 0, 0, 0, 0, 1), 128).unwrap()
        );
    }
}
