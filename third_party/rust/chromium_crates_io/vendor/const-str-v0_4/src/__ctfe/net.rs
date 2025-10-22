use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};

struct Parser<'a> {
    s: &'a [u8],
    i: usize,
}

impl<'a> Parser<'a> {
    const fn new(s: &'a str) -> Self {
        Self {
            s: s.as_bytes(),
            i: 0,
        }
    }

    const fn const_clone(&self) -> Self {
        Self {
            s: self.s,
            i: self.i,
        }
    }

    const fn is_end(&self) -> bool {
        self.i == self.s.len()
    }

    const fn peek(&self) -> Option<u8> {
        let &Self { s, i, .. } = self;
        if i < s.len() {
            Some(s[i])
        } else {
            None
        }
    }

    const fn peek2(&self) -> Option<(u8, Option<u8>)> {
        let &Self { s, i, .. } = self;
        if i >= s.len() {
            return None;
        }
        if i + 1 >= s.len() {
            return Some((s[i], None));
        }
        Some((s[i], Some(s[i + 1])))
    }

    const fn read_byte(mut self) -> (Self, Option<u8>) {
        let Self { s, i, .. } = self;
        if i < s.len() {
            self.i += 1;
            (self, Some(s[i]))
        } else {
            (self, None)
        }
    }

    const fn read_given_byte(self, byte: u8) -> (Self, Option<()>) {
        let p = self.const_clone();
        let (p, val) = p.read_byte();
        match val {
            Some(v) if v == byte => (p, Some(())),
            _ => (self, None),
        }
    }

    const fn advance(mut self, step: usize) -> Self {
        self.i += step;
        self
    }
}

macro_rules! impl_read_uint {
    ($ty:ty, $id: ident) => {
        impl<'a> Parser<'a> {
            const fn $id(
                mut self,
                radix: u8,
                allow_leading_zeros: bool,
                max_digits: usize,
            ) -> (Self, Option<$ty>) {
                constfn_assert!(radix == 10 || radix == 16);
                let Self { s, mut i, .. } = self;
                let mut digit_count = 0;
                let mut ans: $ty = 0;

                loop {
                    let b = if i < s.len() { s[i] } else { break };
                    let x = match b {
                        b'0'..=b'9' => b - b'0',
                        b'a'..=b'f' if radix == 16 => b - b'a' + 10,
                        b'A'..=b'F' if radix == 16 => b - b'A' + 10,
                        _ => break,
                    };
                    if !allow_leading_zeros && (ans == 0 && digit_count == 1) {
                        return (self, None);
                    }
                    ans = match ans.checked_mul(radix as $ty) {
                        Some(x) => x,
                        None => return (self, None),
                    };
                    ans = match ans.checked_add(x as $ty) {
                        Some(x) => x,
                        None => return (self, None),
                    };
                    i += 1;
                    digit_count += 1;

                    if digit_count > max_digits {
                        return (self, None);
                    }
                }

                if digit_count == 0 {
                    return (self, None);
                }
                self.i = i;
                (self, Some(ans))
            }
        }
    };
}

impl_read_uint!(u8, read_u8);
impl_read_uint!(u16, read_u16);

macro_rules! try_parse {
    ($orig:ident, $id:ident, $ret: expr) => {{
        match $ret {
            (next, Some(val)) => {
                $id = next;
                val
            }
            (_, None) => return ($orig, None),
        }
    }};
}

macro_rules! parse {
    ($id:ident,$ret: expr) => {{
        let (next, val) = $ret;
        $id = next;
        val
    }};
}

impl<'a> Parser<'a> {
    const fn read_ipv4(self) -> (Self, Option<Ipv4Addr>) {
        let mut p = self.const_clone();
        let mut nums = [0; 4];
        let mut i = 0;
        while i < 4 {
            if i > 0 {
                try_parse!(self, p, p.read_given_byte(b'.'));
            }
            nums[i] = try_parse!(self, p, p.read_u8(10, false, 3));
            i += 1;
        }
        let val = Ipv4Addr::new(nums[0], nums[1], nums[2], nums[3]);
        (p, Some(val))
    }

    const fn read_ipv6(self) -> (Self, Option<Ipv6Addr>) {
        let mut p = self.const_clone();

        let mut nums: [u16; 8] = [0; 8];
        let mut left_cnt = 0;
        let mut right_cnt = 0;

        let mut state: u8 = 0;
        'dfa: loop {
            match state {
                0 => match p.peek2() {
                    Some((b':', Some(b':'))) => {
                        p = p.advance(2);
                        state = 2;
                        continue 'dfa;
                    }
                    _ => {
                        state = 1;
                        continue 'dfa;
                    }
                },
                1 => loop {
                    nums[left_cnt] = try_parse!(self, p, p.read_u16(16, true, 4));
                    left_cnt += 1;
                    if left_cnt == 8 {
                        break 'dfa;
                    }
                    try_parse!(self, p, p.read_given_byte(b':'));
                    if matches!(p.peek(), Some(b':')) {
                        p = p.advance(1);
                        if left_cnt == 7 {
                            break 'dfa;
                        }
                        state = 2;
                        continue 'dfa;
                    }
                    if left_cnt == 6 {
                        if let Some(val) = parse!(p, p.read_ipv4()) {
                            let [n1, n2, n3, n4] = val.octets();
                            nums[6] = u16::from_be_bytes([n1, n2]);
                            nums[7] = u16::from_be_bytes([n3, n4]);
                            // left_cnt = 8;
                            break 'dfa;
                        }
                    }
                },
                2 => loop {
                    if left_cnt + right_cnt <= 6 {
                        if let Some(val) = parse!(p, p.read_ipv4()) {
                            let [n1, n2, n3, n4] = val.octets();
                            nums[7 - right_cnt] = u16::from_be_bytes([n1, n2]);
                            nums[6 - right_cnt] = u16::from_be_bytes([n3, n4]);
                            right_cnt += 2;
                            break 'dfa;
                        }
                    }
                    match parse!(p, p.read_u16(16, true, 4)) {
                        Some(val) => {
                            nums[7 - right_cnt] = val;
                            right_cnt += 1;
                            if left_cnt + right_cnt == 7 {
                                break 'dfa;
                            }
                            match p.peek() {
                                Some(b':') => p = p.advance(1),
                                _ => break 'dfa,
                            }
                        }
                        None => break 'dfa,
                    }
                },
                _ => constfn_unreachable!(),
            }
        }
        {
            let mut i = 8 - right_cnt;
            let mut j = 7;
            #[allow(clippy::manual_swap)]
            while i < j {
                let (lhs, rhs) = (nums[i], nums[j]);
                nums[i] = rhs;
                nums[j] = lhs;
                i += 1;
                j -= 1;
            }
        }

        let val = Ipv6Addr::new(
            nums[0], nums[1], nums[2], nums[3], //
            nums[4], nums[5], nums[6], nums[7], //
        );
        (p, Some(val))
    }
}

macro_rules! parse_with {
    ($s: expr, $m:ident) => {{
        let p = Parser::new($s);
        let (p, val) = p.$m();
        match val {
            Some(v) if p.is_end() => Some(v),
            _ => None,
        }
    }};
}

pub const fn expect_ipv4(s: &str) -> Ipv4Addr {
    match parse_with!(s, read_ipv4) {
        Some(val) => val,
        None => constfn_panic!("invalid ipv4 address"),
    }
}

pub const fn expect_ipv6(s: &str) -> Ipv6Addr {
    match parse_with!(s, read_ipv6) {
        Some(val) => val,
        None => constfn_panic!("invalid ipv6 address"),
    }
}

pub const fn expect_ip(s: &str) -> IpAddr {
    match parse_with!(s, read_ipv4) {
        Some(val) => IpAddr::V4(val),
        None => match parse_with!(s, read_ipv6) {
            Some(val) => IpAddr::V6(val),
            None => constfn_panic!("invalid ip address"),
        },
    }
}

/// Converts a string slice to an IP address.
///
/// # Examples
/// ```
/// use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
/// use const_str::ip_addr;
///
/// const LOCALHOST_V4: Ipv4Addr = ip_addr!(v4, "127.0.0.1");
/// const LOCALHOST_V6: Ipv6Addr = ip_addr!(v6, "::1");
///
/// const LOCALHOSTS: [IpAddr;2] = [ip_addr!("127.0.0.1"), ip_addr!("::1")];
/// ```
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
#[macro_export]
macro_rules! ip_addr {
    (v4, $s:expr) => {
        $crate::__ctfe::expect_ipv4($s)
    };
    (v6, $s:expr) => {
        $crate::__ctfe::expect_ipv6($s)
    };
    ($s:expr) => {
        $crate::__ctfe::expect_ip($s)
    };
}

#[test]
fn test_ip_addr() {
    fn parse<T>(s: &str, _: &T) -> T
    where
        T: std::str::FromStr,
        <T as std::str::FromStr>::Err: std::fmt::Debug,
    {
        s.parse().unwrap()
    }

    macro_rules! test_ip_addr {
        (v4, invalid, $s:expr) => {{
            let output = parse_with!($s, read_ipv4);
            assert!(output.is_none());
        }};
        (v6, invalid, $s:expr) => {{
            let output = parse_with!($s, read_ipv6);
            assert!(output.is_none());
        }};
        ($t:tt, $s:expr) => {{
            let output = ip_addr!($t, $s);
            let ans = parse($s, &output);
            assert_eq!(output, ans);
        }
        {
            let output = ip_addr!($s);
            let ans = parse($s, &output);
            assert_eq!(output, ans);
        }};
    }

    test_ip_addr!(v4, "0.0.0.0");
    test_ip_addr!(v4, "127.0.0.1");
    test_ip_addr!(v4, "255.255.255.255");
    test_ip_addr!(v4, invalid, "0");
    test_ip_addr!(v4, invalid, "0x1");
    test_ip_addr!(v4, invalid, "127.00.0.1");
    test_ip_addr!(v4, invalid, "027.0.0.1");
    test_ip_addr!(v4, invalid, "256.0.0.1");
    test_ip_addr!(v4, invalid, "255.0.0");
    test_ip_addr!(v4, invalid, "255.0.0.1.2");
    test_ip_addr!(v4, invalid, "255.0.0..1");

    test_ip_addr!(v6, "::");
    test_ip_addr!(v6, "::1");
    test_ip_addr!(v6, "2001:db8::2:3:4:1");
    test_ip_addr!(v6, "::1:2:3");
    test_ip_addr!(v6, "FF01::101");
    test_ip_addr!(v6, "0:0:0:0:0:0:13.1.68.3");
    test_ip_addr!(v6, "0:0:0:0:0:FFFF:129.144.52.38");
    test_ip_addr!(v6, invalid, "::::");
    test_ip_addr!(v6, invalid, "::00001");
}
