// Copyright 2016 Hroi Sigurdsson
//
// Licensed under the MIT license <LICENSE-MIT or http://opensource.org/licenses/MIT>.
// This file may not be copied, modified, or distributed except according to those terms.
//

extern crate treebitmap;

mod rand_test;

use std::net::{Ipv4Addr, Ipv6Addr};
use std::str::FromStr;
use treebitmap::*;

#[test]
#[should_panic]
fn insert_host_bit_set() {
    let mut tbl = IpLookupTable::new();
    tbl.insert(Ipv4Addr::new(255, 255, 255, 255), 17, 1);
}

#[test]
#[should_panic]
fn exact_match_host_bit_set() {
    let mut tbl = IpLookupTable::new();
    tbl.insert(Ipv4Addr::new(255, 255, 255, 255), 16, 1);
    tbl.insert(Ipv4Addr::new(255, 255, 255, 255), 32, 2);
    tbl.exact_match(Ipv4Addr::new(255, 255, 255, 255), 17);
}

#[test]
fn remove() {
    let mut tbl = IpLookupTable::new();
    tbl.insert(Ipv4Addr::new(10, 0, 0, 0), 8, 1);
    tbl.insert(Ipv4Addr::new(10, 0, 10, 0), 24, 2);
    tbl.insert(Ipv4Addr::new(10, 0, 10, 9), 32, 3);

    {
        let lookup_ip = Ipv4Addr::new(10, 0, 10, 10);
        let expected_ip = Ipv4Addr::new(10, 0, 10, 0);
        let lookup_result = tbl.longest_match(lookup_ip);
        assert_eq!(lookup_result, Some((expected_ip, 24, &2)));

        let lookup_ip = Ipv4Addr::new(10, 0, 10, 9);
        let expected_ip = Ipv4Addr::new(10, 0, 10, 9);
        let lookup_result = tbl.longest_match(lookup_ip);
        assert_eq!(lookup_result, Some((expected_ip, 32, &3)));
    }

    {
        let value = tbl.remove(Ipv4Addr::new(10, 0, 10, 0), 24);
        assert_eq!(value, Some(2));
        let lookup_ip = Ipv4Addr::new(10, 0, 10, 10);
        let expected_ip = Ipv4Addr::new(10, 0, 0, 0);
        let lookup_result = tbl.longest_match(lookup_ip);
        assert_eq!(lookup_result, Some((expected_ip, 8, &1)));
    }
}

#[test]
fn insert() {
    let mut tbm = IpLookupTable::new();
    tbm.insert(Ipv4Addr::new(0, 0, 0, 0), 0, 1);
    tbm.insert(Ipv4Addr::new(10, 0, 0, 0), 8, 1);
}

#[test]
fn insert_dup() {
    let mut tbm = IpLookupTable::new();
    assert_eq!(tbm.insert(Ipv4Addr::new(10, 0, 0, 0), 8, 1), None);
    assert_eq!(tbm.insert(Ipv4Addr::new(10, 0, 0, 0), 8, 2), Some(1));
}

#[test]
fn longest_match6() {
    let mut tbm = IpLookupTable::new();
    let google = Ipv6Addr::from_str("2a00:1450::0").unwrap();
    let ip = Ipv6Addr::from_str("2a00:1450:400f:804::2004").unwrap();
    let ip2 = Ipv6Addr::from_str("2000:1000::f00").unwrap();
    tbm.insert(google, 32, 1);
    let ret = tbm.longest_match(ip);
    println!("{:?}", ret.unwrap());
    assert_eq!(ret.unwrap().0, google);
    let ret = tbm.longest_match(ip2);
    println!("{:?}", ret);
    assert_eq!(ret, None);
}

#[test]
fn matches6() {
    let mut tbm = IpLookupTable::new();
    let ip = Ipv6Addr::from_str("2a00::0").unwrap();
    tbm.insert(ip, 32, 1);
    tbm.insert(ip, 24, 1);
    tbm.insert(ip, 16, 2);
    assert_eq!(
        2,
        tbm.matches(Ipv6Addr::from_str("2a00:0099::0").unwrap())
            .count()
    );
}

#[test]
fn longest_match() {
    let mut tbm = IpLookupTable::new();
    tbm.insert(Ipv4Addr::new(10, 0, 0, 0), 8, 100002);
    tbm.insert(Ipv4Addr::new(100, 64, 0, 0), 24, 10064024);
    tbm.insert(Ipv4Addr::new(100, 64, 1, 0), 24, 10064124);
    tbm.insert(Ipv4Addr::new(100, 64, 0, 0), 10, 100004);

    let result = tbm.longest_match(Ipv4Addr::new(10, 10, 10, 10));
    assert_eq!(result, Some((Ipv4Addr::new(10, 0, 0, 0), 8, &100002)));

    let result = tbm.longest_match_mut(Ipv4Addr::new(10, 10, 10, 10));
    assert_eq!(result, Some((Ipv4Addr::new(10, 0, 0, 0), 8, &mut 100002)));

    let result = tbm.longest_match(Ipv4Addr::new(100, 100, 100, 100));
    assert_eq!(result, Some((Ipv4Addr::new(100, 64, 0, 0), 10, &100004)));

    let result = tbm.longest_match_mut(Ipv4Addr::new(100, 100, 100, 100));
    assert_eq!(
        result,
        Some((Ipv4Addr::new(100, 64, 0, 0), 10, &mut 100004))
    );

    let result = tbm.longest_match(Ipv4Addr::new(100, 64, 0, 100));
    assert_eq!(result, Some((Ipv4Addr::new(100, 64, 0, 0), 24, &10064024)));

    let result = tbm.longest_match_mut(Ipv4Addr::new(100, 64, 0, 100));
    assert_eq!(
        result,
        Some((Ipv4Addr::new(100, 64, 0, 0), 24, &mut 10064024))
    );

    let result = tbm.longest_match(Ipv4Addr::new(100, 64, 1, 100));
    assert_eq!(result, Some((Ipv4Addr::new(100, 64, 1, 0), 24, &10064124)));

    let result = tbm.longest_match_mut(Ipv4Addr::new(100, 64, 1, 100));
    assert_eq!(
        result,
        Some((Ipv4Addr::new(100, 64, 1, 0), 24, &mut 10064124))
    );

    let result = tbm.longest_match(Ipv4Addr::new(200, 200, 200, 200));
    assert_eq!(result, None);

    let result = tbm.longest_match_mut(Ipv4Addr::new(200, 200, 200, 200));
    assert_eq!(result, None);
}

#[test]
fn iter() {
    let mut tbl = IpLookupTable::new();

    let (ip_a, mask_a, value_a) = (Ipv4Addr::new(10, 0, 0, 0), 8, 1);
    let (ip_b, mask_b, value_b) = (Ipv4Addr::new(100, 64, 0, 0), 24, 2);
    let (ip_c, mask_c, value_c) = (Ipv4Addr::new(100, 64, 1, 0), 24, 3);
    tbl.insert(ip_a, mask_a, value_a);
    tbl.insert(ip_b, mask_b, value_b);
    tbl.insert(ip_c, mask_c, value_c);

    let mut iter = tbl.iter();
    assert_eq!(iter.next(), Some((ip_a, mask_a, &value_a)));
    assert_eq!(iter.next(), Some((ip_b, mask_b, &value_b)));
    assert_eq!(iter.next(), Some((ip_c, mask_c, &value_c)));
    assert_eq!(iter.next(), None);
}

#[test]
fn iter_mut() {
    let mut tbl = IpLookupTable::new();

    let (ip_a, mask_a, value_a) = (Ipv4Addr::new(10, 0, 0, 0), 8, 1);
    let (ip_b, mask_b, value_b) = (Ipv4Addr::new(100, 64, 0, 0), 24, 2);
    let (ip_c, mask_c, value_c) = (Ipv4Addr::new(100, 64, 1, 0), 24, 3);
    tbl.insert(ip_a, mask_a, value_a);
    tbl.insert(ip_b, mask_b, value_b);
    tbl.insert(ip_c, mask_c, value_c);

    for (_ip, _mask, val) in tbl.iter_mut() {
        *val += 10;
    }
    let mut iter = tbl.iter();

    assert_eq!(iter.next(), Some((ip_a, mask_a, &11)));
    assert_eq!(iter.next(), Some((ip_b, mask_b, &12)));
    assert_eq!(iter.next(), Some((ip_c, mask_c, &13)));
    assert_eq!(iter.next(), None);
}

#[test]
fn into_iter() {
    let mut tbl = IpLookupTable::new();

    let (ip_a, mask_a, value_a) = (Ipv4Addr::new(10, 0, 0, 0), 8, 1);
    let (ip_b, mask_b, value_b) = (Ipv4Addr::new(100, 64, 0, 0), 24, 2);
    let (ip_c, mask_c, value_c) = (Ipv4Addr::new(100, 64, 1, 0), 24, 3);
    tbl.insert(ip_a, mask_a, value_a);
    tbl.insert(ip_b, mask_b, value_b);
    tbl.insert(ip_c, mask_c, value_c);

    let mut iter = tbl.into_iter();
    assert_eq!(iter.next(), Some((ip_a, mask_a, value_a)));
    assert_eq!(iter.next(), Some((ip_b, mask_b, value_b)));
    assert_eq!(iter.next(), Some((ip_c, mask_c, value_c)));
    assert_eq!(iter.next(), None);
}

#[test]
fn send() {
    fn check_if_send<T: Send>() {}
    check_if_send::<IpLookupTable<Ipv4Addr, ()>>();
}

#[test]
fn matches() {
    let mut tbm = IpLookupTable::new();
    tbm.insert(Ipv4Addr::new(10, 0, 0, 0), 8, 1);
    tbm.insert(Ipv4Addr::new(10, 1, 0, 0), 16, 2);
    assert_eq!(2, tbm.matches(Ipv4Addr::new(10, 1, 0, 30)).count());
}

#[test]
fn matches_all_zeros() {
    let mut table = IpLookupTable::new();
    for i in 0..=32 {
        table.insert(Ipv4Addr::new(0, 0, 0, 0), i, i);
    }

    for (ip, matched, value) in table.matches(Ipv4Addr::new(0, 0, 0, 0)) {
        assert_eq!(ip, Ipv4Addr::new(0, 0, 0, 0));
        assert_eq!(matched, *value);
    }

    assert_eq!(table.matches(Ipv4Addr::new(0, 0, 0, 0)).count(), 33);
    assert_eq!(table.matches(Ipv4Addr::new(1, 0, 0, 0)).count(), 8);
    assert_eq!(table.matches(Ipv4Addr::new(0, 0, 255, 255)).count(), 17);
    assert_eq!(table.matches(Ipv4Addr::new(255, 255, 255, 255)).count(), 1);
    assert_eq!(table.matches(Ipv4Addr::new(64, 0, 0, 0)).count(), 2);
}

#[test]
fn matches_10_range() {
    let mut table = IpLookupTable::new();
    table.insert(Ipv4Addr::new(10, 0, 0, 0), 8, 0);
    table.insert(Ipv4Addr::new(10, 6, 0, 0), 16, 0);
    table.insert(Ipv4Addr::new(10, 6, 252, 0), 24, 0);

    assert_eq!(table.matches(Ipv4Addr::new(10, 6, 252, 3)).count(), 3);
    assert_eq!(table.matches(Ipv4Addr::new(10, 6, 255, 3)).count(), 2);
    assert_eq!(table.matches(Ipv4Addr::new(11, 6, 255, 3)).count(), 0);
}

#[test]
fn matches_empty() {
    let table: IpLookupTable<Ipv4Addr, ()> = IpLookupTable::new();
    assert_eq!(table.matches(Ipv4Addr::new(0, 0, 0, 0)).count(), 0);
}

#[test]
fn matches_ipv6() {
    let mut table = IpLookupTable::new();
    let less_specific = Ipv6Addr::new(0x2001, 0xdb8, 0, 0, 0, 0, 0, 0);
    let more_specific = Ipv6Addr::new(0x2001, 0xdb8, 0xdead, 0, 0, 0, 0, 0);
    table.insert(less_specific, 32, "foo");
    table.insert(more_specific, 48, "bar");
    assert_eq!(table.matches(less_specific).count(), 1);
    assert_eq!(table.matches(more_specific).count(), 2);
    assert_eq!(
        table.matches(Ipv6Addr::new(0, 0, 0, 0, 0, 0, 0, 0)).count(),
        0
    );
}

// https://github.com/hroi/treebitmap/issues/7
#[test]
fn issue_7() {
    let mut table: IpLookupTable<Ipv4Addr, ()> = IpLookupTable::new();

    println!("len: {}", table.len());

    table.insert("2.93.185.24".parse().unwrap(), 32, ());
    table.insert("2.93.200.133".parse().unwrap(), 32, ());

    println!("len: {}", table.len());

    table.remove("2.93.185.24".parse().unwrap(), 32);
    table.remove("2.93.200.133".parse().unwrap(), 32);

    println!("len: {}", table.len());
}

// https://github.com/hroi/treebitmap/issues/13
#[test]
fn issue_13() {
    const ADDR: Ipv4Addr = Ipv4Addr::new(49, 255, 11, 17);
    let mut table = IpLookupTable::new();

    println!("insert 28");
    table.insert(Ipv4Addr::new(49, 255, 11, 16), 28, 28);
    assert_eq!(
        table.exact_match(Ipv4Addr::new(49, 255, 11, 16), 28),
        Some(&28)
    );
    assert_eq!(
        table.exact_match_mut(Ipv4Addr::new(49, 255, 11, 16), 28),
        Some(&mut 28)
    );
    println!("insert 32");
    table.insert(ADDR, 32, 32);

    println!("match 32");
    assert_eq!(table.exact_match(ADDR, 32), Some(&32));
    assert_eq!(table.exact_match_mut(ADDR, 32), Some(&mut 32));

    assert!(table.longest_match(ADDR).is_some());
    assert!(table.longest_match_mut(ADDR).is_some());

    assert_eq!(table.matches(ADDR).count(), 2);

    let v = table.remove(ADDR, 32);
    assert_eq!(v, Some(32));
    println!("removed: {:?}", v);
}

#[test]
fn ipv4_tests() {
    let a = 1;
    let b = 2;
    let c = 3;
    let d = 4;
    let e = 5;
    let g = 6;
    let h = 7;

    let mut table: IpLookupTable<Ipv4Addr, usize> = IpLookupTable::new();

    macro_rules! insert {
        ($v:expr, $o1:expr, $o2:expr, $o3:expr, $o4:expr, $cidr:expr) => {
            table.insert(Ipv4Addr::new($o1, $o2, $o3, $o4), $cidr, $v);
        };
    }

    macro_rules! assert_match {
        ($v:expr, $o1:expr, $o2:expr, $o3:expr, $o4:expr) => {
            let res = table
                .longest_match(Ipv4Addr::new($o1, $o2, $o3, $o4))
                .unwrap();
            assert_eq!(*res.2, $v);
        };
    }

    macro_rules! assert_not_match {
        ($v:expr, $o1:expr, $o2:expr, $o3:expr, $o4:expr) => {
            let res = table
                .longest_match(Ipv4Addr::new($o1, $o2, $o3, $o4))
                .unwrap();
            assert_ne!(*res.2, $v);
        };
    }

    macro_rules! remove_by_value {
        ($v: expr) => {
            let mut remove = vec![];
            for (ip, cidr, v) in table.iter_mut() {
                if *v == $v {
                    remove.push((ip, cidr));
                }
            }
            for (ip, cidr) in remove {
                table.remove(ip, cidr);
            }
        };
    }

    insert!(a, 192, 168, 4, 0, 24);
    insert!(b, 192, 168, 4, 4, 32);
    insert!(c, 192, 168, 0, 0, 16);
    insert!(d, 192, 95, 5, 64, 27);
    insert!(c, 192, 95, 5, 65, 27);
    insert!(e, 0, 0, 0, 0, 0);
    insert!(g, 64, 15, 112, 0, 20);
    insert!(h, 64, 15, 123, 128, 25);
    insert!(a, 10, 0, 0, 0, 25);
    insert!(b, 10, 0, 0, 128, 25);
    insert!(a, 10, 1, 0, 0, 30);
    insert!(b, 10, 1, 0, 4, 30);
    insert!(c, 10, 1, 0, 8, 29);
    insert!(d, 10, 1, 0, 16, 29);

    assert_match!(a, 192, 168, 4, 20);
    assert_match!(a, 192, 168, 4, 0);
    assert_match!(b, 192, 168, 4, 4);
    assert_match!(c, 192, 168, 200, 182);
    assert_match!(c, 192, 95, 5, 68);
    assert_match!(e, 192, 95, 5, 96);
    assert_match!(g, 64, 15, 116, 26);
    assert_match!(g, 64, 15, 127, 3);

    insert!(a, 1, 0, 0, 0, 32);
    insert!(a, 64, 0, 0, 0, 32);
    insert!(a, 128, 0, 0, 0, 32);
    insert!(a, 192, 0, 0, 0, 32);
    insert!(a, 255, 0, 0, 0, 32);

    assert_match!(a, 1, 0, 0, 0);
    assert_match!(a, 64, 0, 0, 0);
    assert_match!(a, 128, 0, 0, 0);
    assert_match!(a, 192, 0, 0, 0);
    assert_match!(a, 255, 0, 0, 0);

    remove_by_value!(a);

    assert_not_match!(a, 1, 0, 0, 0);
    assert_not_match!(a, 64, 0, 0, 0);
    assert_not_match!(a, 128, 0, 0, 0);
    assert_not_match!(a, 192, 0, 0, 0);
    assert_not_match!(a, 255, 0, 0, 0);
}

#[test]
fn ipv6_tests() {
    let a = 1;
    let b = 2;
    let c = 3;
    let d = 4;
    let e = 5;
    let f = 6;
    let g = 7;
    let h = 8;

    let mut table: IpLookupTable<Ipv6Addr, usize> = IpLookupTable::new();

    macro_rules! ipv6 {
        ($o1:expr, $o2:expr, $o3:expr, $o4:expr) => {{
            let o1: u32 = $o1;
            let o2: u32 = $o2;
            let o3: u32 = $o3;
            let o4: u32 = $o4;
            Ipv6Addr::new(
                (o1 >> 16) as u16,
                (o1 & 0xffff) as u16,
                (o2 >> 16) as u16,
                (o2 & 0xffff) as u16,
                (o3 >> 16) as u16,
                (o3 & 0xffff) as u16,
                (o4 >> 16) as u16,
                (o4 & 0xffff) as u16,
            )
        }};
    }

    macro_rules! insert {
        ($v:expr, $o1:expr, $o2:expr, $o3:expr, $o4:expr, $cidr:expr) => {
            table.insert(ipv6!($o1, $o2, $o3, $o4), $cidr, $v);
        };
    }

    macro_rules! assert_match {
        ($v:expr, $o1:expr, $o2:expr, $o3:expr, $o4:expr) => {
            let res = table.longest_match(ipv6!($o1, $o2, $o3, $o4)).unwrap();
            assert_eq!(*res.2, $v);
        };
    }

    insert!(d, 0x26075300, 0x60006b00, 0, 0xc05f0543, 128);
    insert!(c, 0x26075300, 0x60006b00, 0, 0, 64);
    insert!(e, 0, 0, 0, 0, 0);
    insert!(f, 0, 0, 0, 0, 0);
    insert!(g, 0x24046800, 0, 0, 0, 32);
    insert!(h, 0x24046800, 0x40040800, 0xdeadbeef, 0xdeadbeef, 64);
    insert!(a, 0x24046800, 0x40040800, 0xdeadbeef, 0xdeadbeef, 128);
    insert!(c, 0x24446800, 0x40e40800, 0xdeaebeef, 0xdefbeef, 128);
    insert!(b, 0x24446800, 0xf0e40800, 0xeeaebeef, 0, 98);

    assert_match!(d, 0x26075300, 0x60006b00, 0, 0xc05f0543);
    assert_match!(c, 0x26075300, 0x60006b00, 0, 0xc02e01ee);
    assert_match!(f, 0x26075300, 0x60006b01, 0, 0);
    assert_match!(g, 0x24046800, 0x40040806, 0, 0x1006);
    assert_match!(g, 0x24046800, 0x40040806, 0x1234, 0x5678);
    assert_match!(f, 0x240467ff, 0x40040806, 0x1234, 0x5678);
    assert_match!(f, 0x24046801, 0x40040806, 0x1234, 0x5678);
    assert_match!(h, 0x24046800, 0x40040800, 0x1234, 0x5678);
    assert_match!(h, 0x24046800, 0x40040800, 0, 0);
    assert_match!(h, 0x24046800, 0x40040800, 0x10101010, 0x10101010);
    assert_match!(a, 0x24046800, 0x40040800, 0xdeadbeef, 0xdeadbeef);
}
