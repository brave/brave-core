extern crate rand;
extern crate treebitmap;

use std::mem;
use std::net::{Ipv4Addr, Ipv6Addr};

use self::rand::{thread_rng, Rng};

use treebitmap::address::Address;
use treebitmap::*;

const NUMBER_OF_ITERS: usize = 10; // number of times to run each test
const NUMBER_OF_PEERS: usize = 64; // number of distinct values
const NUMBER_OF_ADDRESS: usize = 100; // number of subnets in trie
const NUMBER_OF_LOOKUPS: usize = 1000; // number of lookups per test

/* Randomized tests against a naive implemenation of a prefix trie.
 */

struct SlowNode<T, A: Address> {
    value: T,
    ip: A,
    masklen: u32,
}

struct SlowRouter<T, A: Address> {
    nodes: Vec<SlowNode<T, A>>,
}

impl<T, A: Address> SlowNode<T, A> {
    fn new(ip: A, masklen: u32, value: T) -> SlowNode<T, A> {
        SlowNode { ip, masklen, value }
    }

    fn matches(&self, ip: A) -> bool {
        let mut remain = self.masklen;
        for (n1, n2) in ip
            .nibbles()
            .as_ref()
            .iter()
            .zip(self.ip.nibbles().as_ref().iter())
        {
            let check = if remain > 4 { 4 } else { remain };
            let bits1 = n1 >> (4 - check);
            let bits2 = n2 >> (4 - check);
            if bits1 != bits2 {
                return false;
            }
            remain -= check;
        }
        true
    }

    fn in_range<R>(&self, rng: &mut R) -> A
    where
        R: Rng,
    {
        // create mutable copy of nibs
        let org = self.ip.nibbles();
        let mut nibs = vec![0u8; org.as_ref().len()];
        nibs.copy_from_slice(org.as_ref());

        // flip bits after masklen
        for i in 0..nibs.len() * 4 {
            if i >= self.masklen as usize {
                let idx = i % 4;
                let bit: u8 = rng.gen();
                let bit = (bit & 0x80) >> 4; // top-most bit of a nibble
                nibs[i / 4] ^= bit >> idx;
                assert!(nibs[i / 4] < 16);
            }
        }

        // santify check
        let addr = A::from_nibbles(nibs.as_ref());
        assert!(self.matches(addr), "generated IP should match subnet");
        addr
    }
}

impl<T, A: Address> PartialEq for SlowNode<T, A> {
    fn eq(&self, rhs: &SlowNode<T, A>) -> bool {
        self.ip.nibbles().as_ref() == rhs.ip.nibbles().as_ref() && self.masklen == rhs.masklen
    }
}

impl<T, A: Address> SlowRouter<T, A> {
    fn new() -> SlowRouter<T, A> {
        SlowRouter { nodes: vec![] }
    }

    fn insert(&mut self, ip: A, masklen: u32, value: T) -> Option<T> {
        let n1 = SlowNode::new(ip, masklen, value);
        for n2 in self.nodes.iter_mut() {
            if n1 == *n2 {
                let old = mem::replace(n2, n1);
                return Some(old.value);
            }
        }
        self.nodes.push(n1);
        None
    }

    fn longest_match(&self, ip: A) -> Option<(A, u32, &T)> {
        let mut res: Option<(A, u32, &T)> = None;
        for node in self.nodes.iter() {
            if node.matches(ip) {
                match res {
                    None => res = Some((node.ip, node.masklen, &node.value)),
                    Some((_, l, _)) => {
                        if l < node.masklen {
                            res = Some((node.ip, node.masklen, &node.value));
                        }
                    }
                }
            }
        }
        res
    }

    fn any<R>(&self, rng: &mut R) -> A
    where
        R: Rng,
    {
        let idx = rng.gen_range(0, self.nodes.len());
        self.nodes[idx].in_range(rng)
    }
}

#[test]
#[cfg(not(miri))]  // miri is too slow
fn ipv6_random_test() {
    for _ in 0..NUMBER_OF_ITERS {
        let mut tbl = IpLookupTable::new();
        let mut slw = SlowRouter::new();
        let mut rng = thread_rng();

        macro_rules! ipv6 {
            () => {{
                Ipv6Addr::new(
                    rng.gen(),
                    rng.gen(),
                    rng.gen(),
                    rng.gen(),
                    rng.gen(),
                    rng.gen(),
                    rng.gen(),
                    rng.gen(),
                )
            }};
        }

        for _ in 0..NUMBER_OF_ADDRESS {
            let masklen = rng.gen_range(0, 128);
            let ip = ipv6!().mask(masklen);
            let value = rng.gen_range(0, NUMBER_OF_PEERS);
            tbl.insert(ip, masklen, value);
            slw.insert(ip, masklen, value);
        }

        for _ in 0..NUMBER_OF_LOOKUPS {
            let ip = if rng.gen() {
                slw.any(&mut rng)
            } else {
                ipv6!()
            };
            assert_eq!(
                tbl.longest_match(ip),
                slw.longest_match(ip),
                "naive list implemenation and trie does not agree"
            );
        }
    }
}

#[test]
#[cfg(not(miri))] // miri is too slow
fn ipv4_random_test() {
    for _ in 0..NUMBER_OF_ITERS {
        let mut tbl = IpLookupTable::new();
        let mut slw = SlowRouter::new();
        let mut rng = thread_rng();

        macro_rules! ipv4 {
            () => {{
                Ipv4Addr::new(rng.gen(), rng.gen(), rng.gen(), rng.gen())
            }};
        }

        for _ in 0..NUMBER_OF_ADDRESS {
            let masklen = rng.gen_range(0, 32);
            let ip = ipv4!().mask(masklen);
            let value = rng.gen_range(0, NUMBER_OF_PEERS);
            tbl.insert(ip, masklen, value);
            slw.insert(ip, masklen, value);
        }

        for _ in 0..NUMBER_OF_LOOKUPS {
            let ip = if rng.gen() {
                slw.any(&mut rng)
            } else {
                ipv4!()
            };
            assert_eq!(
                tbl.longest_match(ip),
                slw.longest_match(ip),
                "naive list implemenation and trie does not agree"
            );
        }
    }
}
