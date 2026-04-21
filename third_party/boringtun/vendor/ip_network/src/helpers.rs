pub fn bit_length(number: u32) -> u8 {
    32 - number.leading_zeros() as u8
}

pub fn bite_mask(mask: u8) -> u32 {
    debug_assert!(mask <= 32);
    match mask {
        0 => 0,
        n => !0 << (32 - n),
    }
}

pub fn bite_mask_u128(mask: u8) -> u128 {
    debug_assert!(mask <= 128);
    match mask {
        0 => 0,
        n => !0 << (128 - n),
    }
}

pub fn split_ip_netmask(input: &str) -> Option<(&str, &str)> {
    let delimiter = match input.find('/') {
        Some(pos) => pos,
        None => return None,
    };
    let (ip, mask) = input.split_at(delimiter);
    let mask = &mask[1..];

    if ip.is_empty() || mask.is_empty() {
        None
    } else {
        Some((ip, mask))
    }
}

#[cfg(test)]
mod tests {
    use super::{bite_mask, bite_mask_u128, split_ip_netmask};

    #[test]
    fn get_bite_mask_32() {
        assert_eq!(u32::MAX, bite_mask(32));
    }

    #[test]
    fn get_bite_mask_0() {
        assert_eq!(0, bite_mask(0));
    }

    #[test]
    fn get_bite_mask_u128_0() {
        assert_eq!(0, bite_mask_u128(0));
    }

    #[test]
    fn get_bite_mask_u128_128() {
        assert_eq!(u128::MAX, bite_mask_u128(128));
    }

    #[test]
    fn split_ip_netmask_normal() {
        let (ip, netmask) = split_ip_netmask("192.168.1.1/24").unwrap();
        assert_eq!("192.168.1.1", ip);
        assert_eq!("24", netmask);
    }

    #[test]
    fn split_ip_netmask_invalid_1() {
        let a = split_ip_netmask("ab");
        assert!(a.is_none());
    }

    #[test]
    fn split_ip_netmask_invalid_2() {
        let a = split_ip_netmask("/");
        assert!(a.is_none());
    }

    #[test]
    fn split_ip_netmask_invalid_3() {
        let a = split_ip_netmask("192.168.1.1/");
        assert!(a.is_none());
    }
}
