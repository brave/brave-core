//! Count only the current test thread's allocations inside explicit measurement scopes.
use std::alloc::{GlobalAlloc, Layout, System};
use std::cell::Cell;
use std::fmt::{self, Write};

thread_local! {
    static ALLOCATIONS: Cell<Option<usize>> = const { Cell::new(None) };
}

fn record_allocation() {
    let _ = ALLOCATIONS.try_with(|count| {
        if let Some(value) = count.get() {
            count.set(Some(value.saturating_add(1)));
        }
    });
}

struct CountingAllocator;

// SAFETY: All memory management is delegated unchanged to System. The counter
// uses a const-initialized thread-local Cell and does not allocate or retain pointers.
unsafe impl GlobalAlloc for CountingAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        record_allocation();
        // SAFETY: The caller supplies the layout required by GlobalAlloc.
        unsafe { System.alloc(layout) }
    }

    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        record_allocation();
        // SAFETY: The caller supplies the layout required by GlobalAlloc.
        unsafe { System.alloc_zeroed(layout) }
    }

    unsafe fn realloc(&self, pointer: *mut u8, layout: Layout, size: usize) -> *mut u8 {
        record_allocation();
        // SAFETY: The caller guarantees this allocation and new size are valid.
        unsafe { System.realloc(pointer, layout, size) }
    }

    unsafe fn dealloc(&self, pointer: *mut u8, layout: Layout) {
        // SAFETY: The caller guarantees the pointer was allocated with this layout.
        unsafe { System.dealloc(pointer, layout) }
    }
}

#[global_allocator]
static ALLOCATOR: CountingAllocator = CountingAllocator;

fn measure<T>(operation: impl FnOnce() -> T) -> (T, usize) {
    struct Reset;
    impl Drop for Reset {
        fn drop(&mut self) {
            ALLOCATIONS.with(|count| count.set(None));
        }
    }
    ALLOCATIONS.with(|count| {
        assert!(count.get().is_none());
        count.set(Some(0));
    });
    let reset = Reset;
    let output = operation();
    let count = ALLOCATIONS.with(|count| count.get().unwrap());
    drop(reset);
    (output, count)
}

struct Buffer {
    bytes: [u8; 2048],
    len: usize,
}

impl Write for Buffer {
    fn write_str(&mut self, text: &str) -> fmt::Result {
        let end = self.len + text.len();
        self.bytes
            .get_mut(self.len..end)
            .ok_or(fmt::Error)?
            .copy_from_slice(text.as_bytes());
        self.len = end;
        Ok(())
    }
}

#[test]
fn documented_allocation_free_operations_remain_allocation_free() {
    use faster_hex::{hex_check, hex_decode, hex_decode_array, hex_encode, Hex};

    // Also prove the observer is active; successful zero counts alone are weak evidence.
    let (allocation, count) = measure(|| std::hint::black_box(vec![0u8; 1024]));
    assert!(count > 0);
    drop(allocation);

    let bytes = [0xab; 513];
    let expected = hex::encode(bytes);
    let mut encoded = [0; 1026];
    let (text, count) = measure(|| hex_encode(&bytes, &mut encoded));
    assert_eq!(&*text.unwrap(), expected);
    assert_eq!(count, 0);
    let mut decoded = [0; 513];
    let (data, count) = measure(|| hex_decode(expected.as_bytes(), &mut decoded));
    assert_eq!(data.unwrap(), bytes);
    assert_eq!(count, 0);
    let (array, count) = measure(|| hex_decode_array::<513>(expected.as_bytes()));
    assert_eq!(array.unwrap(), bytes);
    assert_eq!(count, 0);
    let (valid, count) = measure(|| hex_check(expected.as_bytes()));
    assert!(valid);
    assert_eq!(count, 0);

    let mut writer = Buffer {
        bytes: [0; 2048],
        len: 0,
    };
    let (result, count) = measure(|| write!(writer, "{:+#01040X}", Hex::new(&bytes)));
    result.unwrap();
    assert_eq!(
        &writer.bytes[..writer.len],
        format!("+0x{}{}", "0".repeat(11), expected.to_uppercase()).as_bytes()
    );
    assert_eq!(count, 0);

    for len in [0, 1, 10, 32] {
        writer.len = 0;
        let (result, count) = measure(|| write!(writer, "{}", Hex::new(&bytes[..len])));
        result.unwrap();
        assert_eq!(&writer.bytes[..writer.len], &expected.as_bytes()[..len * 2]);
        assert_eq!(count, 0);
    }

    let (error, count) = measure(|| hex_decode(b"g0", &mut decoded));
    assert!(error.is_err());
    assert_eq!(count, 0);

    #[cfg(feature = "alloc")]
    {
        let mut text = String::with_capacity(2048);
        text.push_str("前缀:");
        let (suffix, count) = measure(|| faster_hex::hex_append(&bytes, &mut text).len());
        assert_eq!(suffix, expected.len());
        assert_eq!(text, format!("前缀:{expected}"));
        assert_eq!(count, 0);
        let (error, count) = measure(|| faster_hex::hex_decode_vec(b"g"));
        assert_eq!(error, Err(faster_hex::Error::OddLength));
        assert_eq!(count, 0);
    }
    #[cfg(feature = "heapless-09")]
    {
        let (text, count) = measure(|| faster_hex::heapless_09::hex_string::<1026>(&bytes));
        assert_eq!(text.unwrap().as_str(), expected);
        assert_eq!(count, 0);
    }
}

#[cfg(feature = "serde")]
#[test]
fn fixed_size_serde_avoids_temporary_allocations() {
    #[derive(serde::Serialize)]
    struct Payload<'a>(#[serde(with = "faster_hex")] &'a [u8]);

    let bytes = [0xab; 65];
    for len in [0, 10, 20, 32, 64, 65] {
        let input = Payload(&bytes[..len]);
        let text = format!("0x{}", hex::encode(&bytes[..len]));
        let expected = serde_json::to_vec(&text).unwrap();
        let mut output = Vec::with_capacity(expected.len());
        let (result, count) = measure(|| serde_json::to_writer(&mut output, &input));
        result.unwrap();
        assert_eq!(output, expected);
        assert_eq!(count, 0);

        let mut actual = [0; 256];
        let mut expected = [0; 256];
        let expected = postcard::to_slice(&text, &mut expected).unwrap();
        let (result, count) = measure(|| postcard::to_slice(&input, &mut actual));
        assert_eq!(result.unwrap(), expected);
        assert_eq!(count, 0);
    }
}
