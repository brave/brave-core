use core::net;

use super::*;

impl Format for net::AddrParseError {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "AddrParseError(_)");
    }
}

impl Format for net::Ipv4Addr {
    fn format(&self, fmt: Formatter) {
        let [a, b, c, d] = self.octets();
        crate::write!(fmt, "{}.{}.{}.{}", a, b, c, d);
    }
}

impl Format for net::SocketAddrV4 {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "{}:{}", self.ip(), self.port());
    }
}

impl Format for net::Ipv6Addr {
    fn format(&self, fmt: Formatter) {
        let octets: [u8; 16] = self.octets();
        crate::write!(
            fmt,
            "{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}",
            octets[0],
            octets[1],
            octets[2],
            octets[3],
            octets[4],
            octets[5],
            octets[6],
            octets[7],
            octets[8],
            octets[9],
            octets[10],
            octets[11],
            octets[12],
            octets[13],
            octets[14],
            octets[15]
        );
    }
}

impl Format for net::SocketAddrV6 {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "[{}]:{}", self.ip(), self.port());
    }
}

impl Format for net::IpAddr {
    fn format(&self, fmt: Formatter) {
        match self {
            net::IpAddr::V4(a) => crate::write!(fmt, "{}", a),
            net::IpAddr::V6(a) => crate::write!(fmt, "{}", a),
        }
    }
}

impl Format for net::SocketAddr {
    fn format(&self, fmt: Formatter) {
        match self {
            net::SocketAddr::V4(a) => crate::write!(fmt, "{}", a),
            net::SocketAddr::V6(a) => crate::write!(fmt, "{}", a),
        }
    }
}
