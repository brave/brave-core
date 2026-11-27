use crate::{
    decode::hex_decode_unchecked, hex_check_with_case, hex_decode_with_case, hex_encode,
    hex_encode_upper, CheckCase, Error,
};

/// One writable page between inaccessible pages. Both ends are exercised so a
/// SIMD load or store outside either slice faults instead of touching spare capacity.
struct Guarded {
    allocation: *mut libc::c_void,
    page: usize,
}

impl Guarded {
    fn new() -> Self {
        // SAFETY: sysconf takes no pointer and querying the page size has no side effects.
        let page = unsafe { libc::sysconf(libc::_SC_PAGESIZE) };
        assert!(page > 0);
        let page = page as usize;
        // SAFETY: Anonymous mapping, no existing mapping is replaced, fd/offset are unused.
        let allocation = unsafe {
            libc::mmap(
                std::ptr::null_mut(),
                page * 3,
                libc::PROT_NONE,
                libc::MAP_PRIVATE | libc::MAP_ANON,
                -1,
                0,
            )
        };
        assert_ne!(allocation, libc::MAP_FAILED);
        let buffer = Self { allocation, page };
        // SAFETY: This page belongs to the new three-page allocation and is page-aligned.
        let result = unsafe {
            libc::mprotect(
                allocation.cast::<u8>().add(page).cast(),
                page,
                libc::PROT_READ | libc::PROT_WRITE,
            )
        };
        assert_eq!(result, 0);
        buffer
    }

    fn pointer(&self, len: usize, end: bool) -> *mut u8 {
        assert!(len <= self.page);
        let offset = self.page + if end { self.page - len } else { 0 };
        // SAFETY: The offset stays in this allocation, including a one-past-page empty slice.
        unsafe { self.allocation.cast::<u8>().add(offset) }
    }

    fn slice(&self, len: usize, end: bool) -> &[u8] {
        // SAFETY: pointer() bounds the slice to initialized, readable mapped memory;
        // its lifetime is tied to self and the immutable borrow excludes writes.
        unsafe { std::slice::from_raw_parts(self.pointer(len, end), len) }
    }

    fn slice_mut(&mut self, len: usize, end: bool) -> &mut [u8] {
        // SAFETY: The exclusive borrow excludes aliases and pointer() bounds the writable slice.
        unsafe { std::slice::from_raw_parts_mut(self.pointer(len, end), len) }
    }
}

impl Drop for Guarded {
    fn drop(&mut self) {
        // SAFETY: This object owns the complete live mapping; no borrowed slices remain.
        unsafe { libc::munmap(self.allocation, self.page * 3) };
    }
}

#[test]
fn independent_decode_lengths_stop_at_guard_pages() {
    // The old unchecked entry could load 64 source bytes solely because dst
    // held 32 bytes. Keep source and destination lengths independent here;
    // exact-ratio kernel adapters intentionally cannot exercise that boundary.
    let lengths = [
        0, 1, 2, 4, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129,
    ];
    let mut source = Guarded::new();
    let mut destination = Guarded::new();
    for source_len in lengths {
        for source_end in [false, true] {
            source.slice_mut(source_len, source_end).fill(b'a');
            let input = source.slice(source_len, source_end);
            for destination_len in lengths {
                for destination_end in [false, true] {
                    let output = destination.slice_mut(destination_len, destination_end);
                    output.fill(0xa5);
                    hex_decode_unchecked(input, output);
                    let written = (source_len / 2).min(destination_len);
                    assert!(output[..written].iter().all(|&byte| byte == 0xaa));
                    assert!(output[written..].iter().all(|&byte| byte == 0xa5));

                    output.fill(0xa5);
                    let expected = if !source_len.is_multiple_of(2) {
                        Err(Error::OddLength)
                    } else if destination_len < source_len / 2 {
                        Err(Error::OutputTooSmall {
                            required: source_len / 2,
                        })
                    } else {
                        Ok(source_len / 2)
                    };
                    assert_eq!(
                        hex_decode_with_case(input, output, CheckCase::None)
                            .map(|bytes| bytes.len()),
                        expected,
                    );
                    let written = expected.unwrap_or(0);
                    assert!(output[..written].iter().all(|&byte| byte == 0xaa));
                    assert!(output[written..].iter().all(|&byte| byte == 0xa5));
                }
            }
        }
    }
}

#[test]
fn conversions_and_checks_stop_at_guard_pages() {
    // None exercises the public boundary; Some directly exercises a kernel.
    let backends = std::iter::once(None).chain(crate::fuzzing::backends().map(Some));
    let mut source = Guarded::new();
    let mut encoded = Guarded::new();
    let mut decoded = Guarded::new();
    for backend in backends {
        let name = backend.map_or("public", |backend| backend.name());
        let encode = |src: &[u8], dst: &mut [u8], upper| match backend {
            Some(backend) => backend.encode(src, dst, upper),
            None => {
                if upper {
                    hex_encode_upper(src, dst)
                } else {
                    hex_encode(src, dst)
                }
                .unwrap();
            }
        };
        let check = |src: &[u8], case| match backend {
            Some(backend) => backend.check(src, case),
            None => hex_check_with_case(src, case),
        };
        let decode = |src: &[u8], dst: &mut [u8], case| match backend {
            Some(backend) => backend.decode(src, dst, case),
            None => hex_decode_with_case(src, dst, case).is_ok(),
        };
        for len in 0..=257 {
            let data: Vec<u8> = (0..len).map(|i| (i * 73 + i / 7) as u8).collect();
            for end in [false, true] {
                source.slice_mut(len, end).copy_from_slice(&data);
                for upper in [false, true] {
                    let case = if upper {
                        CheckCase::Upper
                    } else {
                        CheckCase::Lower
                    };
                    let expected = if upper {
                        hex::encode_upper(&data)
                    } else {
                        hex::encode(&data)
                    };
                    encode(
                        source.slice(len, end),
                        encoded.slice_mut(len * 2, end),
                        upper,
                    );
                    assert_eq!(encoded.slice(len * 2, end), expected.as_bytes(), "{name}");
                    // Also exercise odd-length character checking immediately against a guard page.
                    for size in [len * 2, (len * 2).saturating_sub(1)] {
                        encoded.slice_mut(size, end).fill(b'0');
                        assert!(check(encoded.slice(size, end), case), "{}", name);
                    }
                    encoded
                        .slice_mut(len * 2, end)
                        .copy_from_slice(expected.as_bytes());
                    assert!(decode(
                        encoded.slice(len * 2, end),
                        decoded.slice_mut(len, end),
                        case,
                    ));
                    assert_eq!(decoded.slice(len, end), &data, "{name}");
                    if let Some(backend) = backend {
                        decoded.slice_mut(len, end).fill(0xa5);
                        assert!(backend.decode_owned(
                            encoded.slice(len * 2, end),
                            decoded.slice_mut(len, end),
                            case,
                        ));
                        assert_eq!(decoded.slice(len, end), &data, "{name}");
                    }
                    if len != 0 {
                        encoded.slice_mut(len * 2, end)[len * 2 - 1] = b'!';
                        decoded.slice_mut(len, end).fill(0xa5);
                        let decoded_ok = decode(
                            encoded.slice(len * 2, end),
                            decoded.slice_mut(len, end),
                            case,
                        );
                        assert!(!decoded_ok);
                        assert!(
                            decoded.slice(len, end).iter().all(|&b| b == 0xa5),
                            "{}",
                            name
                        );
                        if let Some(backend) = backend {
                            assert!(!backend.decode_owned(
                                encoded.slice(len * 2, end),
                                decoded.slice_mut(len, end),
                                case,
                            ));
                        }
                    }
                }
            }
        }
    }
}
