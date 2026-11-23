#[cfg(unix)]
use crate::advice::Advice;
use std::fs::{File, OpenOptions};
use std::io::{Read, Write};
use std::mem;
#[cfg(unix)]
use std::os::unix::io::AsRawFd;
#[cfg(windows)]
use std::os::windows::fs::OpenOptionsExt;

#[cfg(windows)]
const GENERIC_ALL: u32 = 0x10000000;

use super::{Mmap, MmapMut, MmapOptions};

#[test]
fn map_file() {
    let expected_len = 128;
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();

    file.set_len(expected_len as u64).unwrap();

    let mut mmap = unsafe { MmapMut::map_mut(&file).unwrap() };
    let len = mmap.len();
    assert_eq!(expected_len, len);

    let zeros = vec![0; len];
    let incr: Vec<u8> = (0..len as u8).collect();

    // check that the mmap is empty
    assert_eq!(&zeros[..], &mmap[..]);

    // write values into the mmap
    (&mut mmap[..]).write_all(&incr[..]).unwrap();

    // read values back
    assert_eq!(&incr[..], &mmap[..]);
}

#[test]
#[cfg(unix)]
fn map_fd() {
    let expected_len = 128;
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();

    file.set_len(expected_len as u64).unwrap();

    let mut mmap = unsafe { MmapMut::map_mut(file.as_raw_fd()).unwrap() };
    let len = mmap.len();
    assert_eq!(expected_len, len);

    let zeros = vec![0; len];
    let incr: Vec<u8> = (0..len as u8).collect();

    // check that the mmap is empty
    assert_eq!(&zeros[..], &mmap[..]);

    // write values into the mmap
    (&mut mmap[..]).write_all(&incr[..]).unwrap();

    // read values back
    assert_eq!(&incr[..], &mmap[..]);
}

/// Checks that "mapping" a 0-length file derefs to an empty slice.
#[test]
fn map_empty_file() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();
    let mmap = unsafe { Mmap::map(&file).unwrap() };
    assert!(mmap.is_empty());
    assert_eq!(mmap.as_ptr().align_offset(mem::size_of::<usize>()), 0);
    let mmap = unsafe { MmapMut::map_mut(&file).unwrap() };
    assert!(mmap.is_empty());
    assert_eq!(mmap.as_ptr().align_offset(mem::size_of::<usize>()), 0);
}

#[test]
fn map_anon() {
    let expected_len = 128;
    let mut mmap = MmapMut::map_anon(expected_len).unwrap();
    let len = mmap.len();
    assert_eq!(expected_len, len);

    let zeros = vec![0; len];
    let incr: Vec<u8> = (0..len as u8).collect();

    // check that the mmap is empty
    assert_eq!(&zeros[..], &mmap[..]);

    // write values into the mmap
    (&mut mmap[..]).write_all(&incr[..]).unwrap();

    // read values back
    assert_eq!(&incr[..], &mmap[..]);
}

#[test]
fn map_anon_zero_len() {
    assert!(MmapOptions::new().map_anon().unwrap().is_empty());
}

#[test]
#[cfg(target_pointer_width = "32")]
fn map_anon_len_overflow() {
    let res = MmapMut::map_anon(0x80000000);

    assert_eq!(
        res.unwrap_err().to_string(),
        "memory map length overflows isize"
    );
}

#[test]
fn file_write() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let mut file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();
    file.set_len(128).unwrap();

    let write = b"abc123";
    let mut read = [0u8; 6];

    let mut mmap = unsafe { MmapMut::map_mut(&file).unwrap() };
    (&mut mmap[..]).write_all(write).unwrap();
    mmap.flush().unwrap();

    file.read_exact(&mut read).unwrap();
    assert_eq!(write, &read);
}

#[test]
fn flush_range() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();
    file.set_len(128).unwrap();
    let write = b"abc123";

    let mut mmap = unsafe {
        MmapOptions::new()
            .offset(2)
            .len(write.len())
            .map_mut(&file)
            .unwrap()
    };
    (&mut mmap[..]).write_all(write).unwrap();
    mmap.flush_async_range(0, write.len()).unwrap();
    mmap.flush_range(0, write.len()).unwrap();
}

#[test]
fn map_copy() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let mut file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();
    file.set_len(128).unwrap();

    let nulls = b"\0\0\0\0\0\0";
    let write = b"abc123";
    let mut read = [0u8; 6];

    let mut mmap = unsafe { MmapOptions::new().map_copy(&file).unwrap() };

    (&mut mmap[..]).write_all(write).unwrap();
    mmap.flush().unwrap();

    // The mmap contains the write
    (&mmap[..]).read_exact(&mut read).unwrap();
    assert_eq!(write, &read);

    // The file does not contain the write
    file.read_exact(&mut read).unwrap();
    assert_eq!(nulls, &read);

    // another mmap does not contain the write
    let mmap2 = unsafe { MmapOptions::new().map(&file).unwrap() };
    (&mmap2[..]).read_exact(&mut read).unwrap();
    assert_eq!(nulls, &read);
}

#[test]
fn map_copy_read_only() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();
    file.set_len(128).unwrap();

    let nulls = b"\0\0\0\0\0\0";
    let mut read = [0u8; 6];

    let mmap = unsafe { MmapOptions::new().map_copy_read_only(&file).unwrap() };
    (&mmap[..]).read_exact(&mut read).unwrap();
    assert_eq!(nulls, &read);

    let mmap2 = unsafe { MmapOptions::new().map(&file).unwrap() };
    (&mmap2[..]).read_exact(&mut read).unwrap();
    assert_eq!(nulls, &read);
}

#[test]
fn map_without_handle_probing() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let mut file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(&path)
        .unwrap();
    file.write_all(b"exact contents").unwrap();

    let readonly = File::open(&path).unwrap();
    let mmap = unsafe { MmapOptions::new().no_probe_handle().map(&readonly).unwrap() };
    assert_eq!(b"exact contents", &mmap[..]);

    // Without probing, the section is created strictly read-only even
    // for a writable handle, so the mapping cannot be transitioned to
    // writable.
    #[cfg(windows)]
    {
        let mmap = unsafe { MmapOptions::new().no_probe_handle().map(&file).unwrap() };
        assert!(mmap.make_mut().is_err());
    }
}

#[test]
fn map_offset() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();

    let offset = u64::from(u32::MAX) + 2;
    let len = 5432;
    file.set_len(offset + len as u64).unwrap();

    // Check inferred length mmap.
    let mmap = unsafe { MmapOptions::new().offset(offset).map_mut(&file).unwrap() };
    assert_eq!(len, mmap.len());

    // Check explicit length mmap.
    let mut mmap = unsafe {
        MmapOptions::new()
            .offset(offset)
            .len(len)
            .map_mut(&file)
            .unwrap()
    };
    assert_eq!(len, mmap.len());

    let zeros = vec![0; len];
    let incr: Vec<_> = (0..len).map(|i| i as u8).collect();

    // check that the mmap is empty
    assert_eq!(&zeros[..], &mmap[..]);

    // write values into the mmap
    (&mut mmap[..]).write_all(&incr[..]).unwrap();

    // read values back
    assert_eq!(&incr[..], &mmap[..]);
}

#[test]
fn index() {
    let mut mmap = MmapMut::map_anon(128).unwrap();
    mmap[0] = 42;
    assert_eq!(42, mmap[0]);
}

#[test]
fn sync_send() {
    fn is_sync_send<T>(_val: T)
    where
        T: Sync + Send,
    {
    }

    let mmap = MmapMut::map_anon(129).unwrap();
    is_sync_send(mmap);
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
fn jit_x86(mut mmap: MmapMut) {
    mmap[0] = 0xB8; // mov eax, 0xAB
    mmap[1] = 0xAB;
    mmap[2] = 0x00;
    mmap[3] = 0x00;
    mmap[4] = 0x00;
    mmap[5] = 0xC3; // ret

    let mmap = mmap.make_exec().expect("make_exec");

    let jitfn: extern "C" fn() -> u8 = unsafe { mem::transmute(mmap.as_ptr()) };
    assert_eq!(jitfn(), 0xab);
}

#[test]
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
fn jit_x86_anon() {
    jit_x86(MmapMut::map_anon(4096).unwrap());
}

#[test]
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
fn jit_x86_file() {
    let tempdir = tempfile::tempdir().unwrap();
    let mut options = OpenOptions::new();
    #[cfg(windows)]
    options.access_mode(GENERIC_ALL);

    let file = options
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(tempdir.path().join("jit_x86"))
        .expect("open");

    file.set_len(4096).expect("set_len");
    jit_x86(unsafe { MmapMut::map_mut(&file).expect("map_mut") });
}

#[test]
fn mprotect_file() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let mut options = OpenOptions::new();
    #[cfg(windows)]
    options.access_mode(GENERIC_ALL);

    let mut file = options
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .expect("open");
    file.set_len(256_u64).expect("set_len");

    let mmap = unsafe { MmapMut::map_mut(&file).expect("map_mut") };

    let mmap = mmap.make_read_only().expect("make_read_only");
    let mut mmap = mmap.make_mut().expect("make_mut");

    let write = b"abc123";
    let mut read = [0u8; 6];

    (&mut mmap[..]).write_all(write).unwrap();
    mmap.flush().unwrap();

    // The mmap contains the write
    (&mmap[..]).read_exact(&mut read).unwrap();
    assert_eq!(write, &read);

    // The file should contain the write
    file.read_exact(&mut read).unwrap();
    assert_eq!(write, &read);

    // another mmap should contain the write
    let mmap2 = unsafe { MmapOptions::new().map(&file).unwrap() };
    (&mmap2[..]).read_exact(&mut read).unwrap();
    assert_eq!(write, &read);

    let mmap = mmap.make_exec().expect("make_exec");

    drop(mmap);
}

#[test]
fn mprotect_copy() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap");

    let mut options = OpenOptions::new();
    #[cfg(windows)]
    options.access_mode(GENERIC_ALL);

    let mut file = options
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .expect("open");
    file.set_len(256_u64).expect("set_len");

    let mmap = unsafe { MmapOptions::new().map_copy(&file).expect("map_mut") };

    let mmap = mmap.make_read_only().expect("make_read_only");
    let mut mmap = mmap.make_mut().expect("make_mut");

    let nulls = b"\0\0\0\0\0\0";
    let write = b"abc123";
    let mut read = [0u8; 6];

    (&mut mmap[..]).write_all(write).unwrap();
    mmap.flush().unwrap();

    // The mmap contains the write
    (&mmap[..]).read_exact(&mut read).unwrap();
    assert_eq!(write, &read);

    // The file does not contain the write
    file.read_exact(&mut read).unwrap();
    assert_eq!(nulls, &read);

    // another mmap does not contain the write
    let mmap2 = unsafe { MmapOptions::new().map(&file).unwrap() };
    (&mmap2[..]).read_exact(&mut read).unwrap();
    assert_eq!(nulls, &read);

    let mmap = mmap.make_exec().expect("make_exec");

    drop(mmap);
}

#[test]
fn mprotect_anon() {
    let mmap = MmapMut::map_anon(256).expect("map_mut");

    let mmap = mmap.make_read_only().expect("make_read_only");
    let mmap = mmap.make_mut().expect("make_mut");
    let mmap = mmap.make_exec().expect("make_exec");
    drop(mmap);
}

#[test]
fn raw() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmapraw");

    let mut options = OpenOptions::new();
    let mut file = options
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .expect("open");
    file.write_all(b"abc123").unwrap();
    let mmap = MmapOptions::new().map_raw(&file).unwrap();
    assert_eq!(mmap.len(), 6);
    assert!(!mmap.as_ptr().is_null());
    assert_eq!(unsafe { std::ptr::read(mmap.as_ptr()) }, b'a');
}

#[test]
fn raw_read_only() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmaprawro");

    File::create(&path).unwrap().write_all(b"abc123").unwrap();

    let mmap = MmapOptions::new()
        .map_raw_read_only(&File::open(&path).unwrap())
        .unwrap();

    assert_eq!(mmap.len(), 6);
    assert!(!mmap.as_ptr().is_null());
    assert_eq!(unsafe { std::ptr::read(mmap.as_ptr()) }, b'a');
}

/// Something that relies on StableDeref
#[test]
#[cfg(feature = "stable_deref_trait")]
fn owning_ref() {
    let mut map = MmapMut::map_anon(128).unwrap();
    map[10] = 42;
    let owning = owning_ref::OwningRef::new(map);
    let sliced = owning.map(|map| &map[10..20]);
    assert_eq!(42, sliced[0]);

    let map = sliced.into_owner().make_read_only().unwrap();
    let owning = owning_ref::OwningRef::new(map);
    let sliced = owning.map(|map| &map[10..20]);
    assert_eq!(42, sliced[0]);
}

#[test]
#[cfg(unix)]
fn advise() {
    let expected_len = 128;
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap_advise");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();

    file.set_len(expected_len as u64).unwrap();

    // Test MmapMut::advise
    let mut mmap = unsafe { MmapMut::map_mut(&file).unwrap() };
    mmap.advise(Advice::Random)
        .expect("mmap advising should be supported on unix");

    let len = mmap.len();
    assert_eq!(expected_len, len);

    let zeros = vec![0; len];
    let incr: Vec<u8> = (0..len as u8).collect();

    // check that the mmap is empty
    assert_eq!(&zeros[..], &mmap[..]);

    mmap.advise_range(Advice::Sequential, 0, mmap.len())
        .expect("mmap advising should be supported on unix");

    // write values into the mmap
    (&mut mmap[..]).write_all(&incr[..]).unwrap();

    // read values back
    assert_eq!(&incr[..], &mmap[..]);

    // Set advice and Read from the read-only map
    let mmap = unsafe { Mmap::map(&file).unwrap() };

    mmap.advise(Advice::Random)
        .expect("mmap advising should be supported on unix");

    // read values back
    assert_eq!(&incr[..], &mmap[..]);
}

#[test]
#[cfg(target_os = "linux")]
fn advise_writes_unsafely() {
    let page_size = unsafe { libc::sysconf(libc::_SC_PAGESIZE) as usize };

    let mut mmap = MmapMut::map_anon(page_size).unwrap();
    mmap.as_mut().fill(255);
    let mmap = mmap.make_read_only().unwrap();

    let a = mmap.as_ref()[0];
    unsafe {
        mmap.unchecked_advise(crate::UncheckedAdvice::DontNeed)
            .unwrap();
    }
    let b = mmap.as_ref()[0];

    assert_eq!(a, 255);
    assert_eq!(b, 0);
}

#[test]
#[cfg(target_os = "linux")]
fn advise_writes_unsafely_to_part_of_map() {
    let page_size = unsafe { libc::sysconf(libc::_SC_PAGESIZE) as usize };

    let mut mmap = MmapMut::map_anon(2 * page_size).unwrap();
    mmap.as_mut().fill(255);
    let mmap = mmap.make_read_only().unwrap();

    let a = mmap.as_ref()[0];
    let b = mmap.as_ref()[page_size];
    unsafe {
        mmap.unchecked_advise_range(crate::UncheckedAdvice::DontNeed, page_size, page_size)
            .unwrap();
    }
    let c = mmap.as_ref()[0];
    let d = mmap.as_ref()[page_size];

    assert_eq!(a, 255);
    assert_eq!(b, 255);
    assert_eq!(c, 255);
    assert_eq!(d, 0);
}

/// Returns true if a non-zero amount of memory is locked.
#[cfg(target_os = "linux")]
fn is_locked() -> bool {
    let status = &std::fs::read_to_string("/proc/self/status")
        .expect("/proc/self/status should be available");
    for line in status.lines() {
        if line.starts_with("VmLck:") {
            let numbers = line.replace(|c: char| !c.is_ascii_digit(), "");
            return numbers != "0";
        }
    }
    panic!("cannot get VmLck information")
}

#[test]
#[cfg(unix)]
fn lock() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("mmap_lock");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();
    file.set_len(128).unwrap();

    let mmap = unsafe { Mmap::map(&file).unwrap() };
    #[cfg(target_os = "linux")]
    assert!(!is_locked());

    mmap.lock().expect("mmap lock should be supported on unix");
    #[cfg(target_os = "linux")]
    assert!(is_locked());

    mmap.lock()
        .expect("mmap lock again should not cause problems");
    #[cfg(target_os = "linux")]
    assert!(is_locked());

    mmap.unlock()
        .expect("mmap unlock should be supported on unix");
    #[cfg(target_os = "linux")]
    assert!(!is_locked());

    mmap.unlock()
        .expect("mmap unlock again should not cause problems");
    #[cfg(target_os = "linux")]
    assert!(!is_locked());
}

#[test]
#[cfg(target_os = "linux")]
fn remap_grow() {
    use crate::RemapOptions;

    let initial_len = 128;
    let final_len = 2000;

    let zeros = vec![0u8; final_len];
    let incr: Vec<u8> = (0..final_len).map(|v| v as u8).collect();

    let file = tempfile::tempfile().unwrap();
    file.set_len(final_len as u64).unwrap();

    let mut mmap = unsafe { MmapOptions::new().len(initial_len).map_mut(&file).unwrap() };
    assert_eq!(mmap.len(), initial_len);
    assert_eq!(&mmap[..], &zeros[..initial_len]);

    unsafe {
        mmap.remap(final_len, RemapOptions::new().may_move(true))
            .unwrap();
    }

    // The size should have been updated
    assert_eq!(mmap.len(), final_len);

    // Should still be all zeros
    assert_eq!(&mmap[..], &zeros);

    // Write out to the whole expanded slice.
    mmap.copy_from_slice(&incr);
}

#[test]
#[cfg(target_os = "linux")]
fn remap_shrink() {
    use crate::RemapOptions;

    let initial_len = 20000;
    let final_len = 400;

    let incr: Vec<u8> = (0..final_len).map(|v| v as u8).collect();

    let file = tempfile::tempfile().unwrap();
    file.set_len(initial_len as u64).unwrap();

    let mut mmap = unsafe { MmapMut::map_mut(&file).unwrap() };
    assert_eq!(mmap.len(), initial_len);

    unsafe { mmap.remap(final_len, RemapOptions::new()).unwrap() };
    assert_eq!(mmap.len(), final_len);

    // Check that the mmap is still writable along the slice length
    mmap.copy_from_slice(&incr);
}

#[test]
#[cfg(any(target_os = "linux", target_os = "android", target_os = "freebsd"))]
fn map_if_safe_sealed_memfd() {
    use std::io::Write;
    use std::os::unix::io::FromRawFd;

    let fd = unsafe {
        libc::memfd_create(
            b"test\0".as_ptr().cast::<libc::c_char>(),
            libc::MFD_ALLOW_SEALING,
        )
    };
    if fd < 0 {
        eprintln!("skipping: memfd_create not supported");
        return;
    }
    let mut file = unsafe { File::from_raw_fd(fd) };

    file.write_all(b"Hello, world!").unwrap();

    // Should report not-safe without seals.
    assert!(Mmap::map_if_safe(&file).unwrap_err().not_safe());

    // Should report not-safe with only F_SEAL_SHRINK
    unsafe { libc::fcntl(file.as_raw_fd(), libc::F_ADD_SEALS, libc::F_SEAL_SHRINK) };
    assert!(Mmap::map_if_safe(&file).unwrap_err().not_safe());

    // Should succeed with both seals
    unsafe { libc::fcntl(file.as_raw_fd(), libc::F_ADD_SEALS, libc::F_SEAL_WRITE) };
    let mmap = Mmap::map_if_safe(&file).unwrap();
    assert_eq!(&mmap[..], b"Hello, world!");
}

#[test]
#[cfg(any(target_os = "linux", target_os = "android", target_os = "freebsd"))]
fn map_if_safe_with_offset() {
    use std::io::Write;
    use std::os::unix::io::FromRawFd;

    let fd = unsafe {
        libc::memfd_create(
            b"test\0".as_ptr().cast::<libc::c_char>(),
            libc::MFD_ALLOW_SEALING,
        )
    };
    if fd < 0 {
        eprintln!("skipping: memfd_create not supported");
        return;
    }
    let mut file = unsafe { File::from_raw_fd(fd) };

    file.write_all(b"Hello, world!").unwrap();
    let seals = libc::F_SEAL_SHRINK | libc::F_SEAL_WRITE;
    unsafe { libc::fcntl(file.as_raw_fd(), libc::F_ADD_SEALS, seals) };

    let mmap = MmapOptions::new().offset(7).map_if_safe(&file).unwrap();
    assert_eq!(&mmap[..], b"world!");

    // Mapping beyond file size should report not-safe
    assert!(MmapOptions::new()
        .offset(7)
        .len(100)
        .map_if_safe(&file)
        .unwrap_err()
        .not_safe());
}

#[test]
#[cfg(any(target_os = "linux", target_os = "android", target_os = "freebsd"))]
fn map_if_safe_regular_file() {
    let tempdir = tempfile::tempdir().unwrap();
    let path = tempdir.path().join("regular");

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(true)
        .open(path)
        .unwrap();
    file.set_len(128).unwrap();

    assert!(Mmap::map_if_safe(&file).unwrap_err().not_safe());
}

#[test]
#[cfg(any(target_os = "linux", target_os = "android"))]
fn map_if_safe_verity() {
    #[repr(C)]
    struct FsVerityEnableArg {
        version: u32,
        hash_algorithm: u32,
        block_size: u32,
        salt_size: u32,
        salt_ptr: u64,
        sig_size: u32,
        _reserved1: u32,
        sig_ptr: u64,
        _reserved2: [u64; 11],
    }

    // _IOW('f', 133, fsverity_enable_arg) — MIPS uses different ioctl encoding
    #[cfg(any(target_arch = "mips", target_arch = "mips64"))]
    const FS_IOC_ENABLE_VERITY: libc::c_ulong = 0x80806685;
    #[cfg(not(any(target_arch = "mips", target_arch = "mips64")))]
    const FS_IOC_ENABLE_VERITY: libc::c_ulong = 0x40806685;

    let tempdir = match std::env::var("MEMMAP2_TEST_VERITY_DIR") {
        Ok(dir) => tempfile::TempDir::new_in(dir).unwrap(),
        Err(_) => tempfile::tempdir().unwrap(),
    };
    let path = tempdir.path().join("verity_test");

    File::create(&path)
        .unwrap()
        .write_all(b"Hello, verity!")
        .unwrap();

    let file = File::open(&path).unwrap();

    let arg = FsVerityEnableArg {
        version: 1,
        hash_algorithm: 1, // FS_VERITY_HASH_ALG_SHA256
        block_size: 4096,
        salt_size: 0,
        salt_ptr: 0,
        sig_size: 0,
        _reserved1: 0,
        sig_ptr: 0,
        _reserved2: [0; 11],
    };

    #[allow(clippy::cast_possible_wrap)]
    let ret = unsafe { libc::ioctl(file.as_raw_fd(), FS_IOC_ENABLE_VERITY as _, &arg) };
    if ret != 0 {
        eprintln!("skipping: fs-verity not supported on this filesystem");
        return;
    }

    let mmap = Mmap::map_if_safe(&file).unwrap();
    assert_eq!(&mmap[..], b"Hello, verity!");
}

#[test]
#[cfg(target_os = "linux")]
#[cfg(target_pointer_width = "32")]
fn remap_len_overflow() {
    use crate::RemapOptions;

    let file = tempfile::tempfile().unwrap();
    file.set_len(1024).unwrap();
    let mut mmap = unsafe { MmapOptions::new().len(1024).map(&file).unwrap() };

    let res = unsafe { mmap.remap(0x80000000, RemapOptions::new().may_move(true)) };
    assert_eq!(
        res.unwrap_err().to_string(),
        "memory map length overflows isize"
    );

    assert_eq!(mmap.len(), 1024);
}

#[test]
#[cfg(target_os = "linux")]
fn remap_with_offset() {
    use crate::RemapOptions;

    let offset = 77;
    let initial_len = 128;
    let final_len = 2000;

    let zeros = vec![0u8; final_len];
    let incr: Vec<u8> = (0..final_len).map(|v| v as u8).collect();

    let file = tempfile::tempfile().unwrap();
    file.set_len(final_len as u64 + offset).unwrap();

    let mut mmap = unsafe {
        MmapOptions::new()
            .len(initial_len)
            .offset(offset)
            .map_mut(&file)
            .unwrap()
    };
    assert_eq!(mmap.len(), initial_len);
    assert_eq!(&mmap[..], &zeros[..initial_len]);

    unsafe {
        mmap.remap(final_len, RemapOptions::new().may_move(true))
            .unwrap();
    }

    // The size should have been updated
    assert_eq!(mmap.len(), final_len);

    // Should still be all zeros
    assert_eq!(&mmap[..], &zeros);

    // Write out to the whole expanded slice.
    mmap.copy_from_slice(&incr);
}
