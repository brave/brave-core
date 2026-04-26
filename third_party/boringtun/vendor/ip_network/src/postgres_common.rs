use std::error::Error;
use std::net::{Ipv4Addr, Ipv6Addr};
use crate::{Ipv4Network, Ipv6Network};

// TODO: These constants are true for Linux, but we have to check it for Windows and other systems
pub const IPV4_TYPE: u8 = 2;
pub const IPV6_TYPE: u8 = 3;

#[inline]
pub fn from_sql_ipv4_network(raw: &[u8]) -> Result<Ipv4Network, Box<dyn Error + Sync + Send>> {
    assert!(raw.len() >= 7);

    if raw[0] != IPV4_TYPE {
        return Err("CIDR is not IP version 4".into());
    }

    if raw[2] != 1 {
        return Err("This field is not CIDR type, probably INET type".into());
    }

    if raw[3] != Ipv4Network::LENGTH / 8 {
        return Err(format!("CIDR is IP version 4, but have bad length '{}'", raw[3]).into());
    }

    let network_address = Ipv4Addr::new(raw[4], raw[5], raw[6], raw[7]);
    let netmask = raw[1];
    Ok(Ipv4Network::new(network_address, netmask)?)
}

#[inline]
pub fn from_sql_ipv6_network(raw: &[u8]) -> Result<Ipv6Network, Box<dyn Error + Sync + Send>> {
    assert!(raw.len() >= 20);

    if raw[0] != IPV6_TYPE {
        return Err("CIDR is not IP version 6".into());
    }

    if raw[2] != 1 {
        return Err("This field is not CIDR type, probably INET type".into());
    }

    if raw[3] != Ipv6Network::LENGTH / 8 {
        return Err(format!("CIDR is IP version 6, but have bad length '{}'", raw[3]).into());
    }

    let mut octets = [0; 16];
    octets.copy_from_slice(&raw[4..]);
    let network_address = Ipv6Addr::from(octets);

    let netmask = raw[1];
    Ok(Ipv6Network::new(network_address, netmask)?)
}

#[inline]
pub fn to_sql_ipv4_network(network: &Ipv4Network) -> [u8; 8] {
    let ip_octets = network.network_address().octets();
    let mut bytes = [0; 8];
    bytes[0] = IPV4_TYPE;
    bytes[1] = network.netmask();
    bytes[2] = 1;
    bytes[3] = Ipv4Network::LENGTH / 8;
    bytes[4..].copy_from_slice(&ip_octets);
    bytes
}

#[inline]
pub fn to_sql_ipv6_network(network: &Ipv6Network) -> [u8; 20] {
    let ip_octets = network.network_address().octets();
    let mut bytes = [0; 20];
    bytes[0] = IPV6_TYPE;
    bytes[1] = network.netmask();
    bytes[2] = 1;
    bytes[3] = Ipv6Network::LENGTH / 8;
    bytes[4..].copy_from_slice(&ip_octets);
    bytes
}
