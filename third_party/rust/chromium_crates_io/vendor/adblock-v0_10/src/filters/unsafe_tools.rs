//! Unsafe utility functions for working with flatbuffers and other low-level operations.

use crate::filters::fb_network::flat::fb;

// Minimum alignment for the beginning of the flatbuffer data.
// Should be 4 while we support armv7 and x86_32.
const MIN_ALIGNMENT: usize = 4;

/// Converts a flatbuffers Vector to a slice.
/// # Safety
/// This function uses unsafe code to convert flatbuffer vector bytes to a slice.
/// It asserts the vector data is properly aligned and sized.
#[inline(always)]
pub fn fb_vector_to_slice<T>(vector: flatbuffers::Vector<'_, T>) -> &[T] {
    let bytes = vector.bytes();

    const fn static_assert_alignment<T>() {
        // We can't use T with the size more than MIN_ALIGNMENT.
        // Since the beginning of flatbuffer data is aligned to that size,
        // the alignment of the data must be a divisor of MIN_ALIGNMENT.
        assert!(MIN_ALIGNMENT % std::mem::size_of::<T>() == 0);
    }
    let _ = static_assert_alignment::<T>;

    assert!(bytes.len() % std::mem::size_of::<T>() == 0);
    assert!(bytes.as_ptr() as usize % std::mem::align_of::<T>() == 0);
    unsafe {
        std::slice::from_raw_parts(
            bytes.as_ptr() as *const T,
            bytes.len() / std::mem::size_of::<T>(),
        )
    }
}

// A safe wrapper around the flatbuffer data.
// It could be constructed from raw data (includes the flatbuffer verification)
// or from a builder that have just been used to construct the flatbuffer
// Invariants:
// 1. self.data() is properly verified flatbuffer contains FilterList.
// 2. self.data() is aligned to MIN_ALIGNMENT bytes.
//    This is necessary for fb_vector_to_slice.
pub(crate) struct VerifiedFlatFilterListMemory {
    // The buffer containing the flatbuffer data.
    raw_data: Vec<u8>,

    // The offset of the data in the buffer.
    // Must be aligned to MIN_ALIGNMENT bytes.
    start: usize,
}

impl VerifiedFlatFilterListMemory {
    pub(crate) fn from_raw(data: Vec<u8>) -> Result<Self, flatbuffers::InvalidFlatbuffer> {
        let memory = Self::from_vec(data);

        // Verify that the data is a valid flatbuffer.
        let _ = fb::root_as_network_filter_list(memory.data())?;

        Ok(memory)
    }

    // Creates a new VerifiedFlatFilterListMemory from a builder.
    // Skip the verification, the builder must contains a valid FilterList.
    pub(crate) fn from_builder(builder: &flatbuffers::FlatBufferBuilder<'_>) -> Self {
        let raw_data = builder.finished_data().to_vec();
        Self::from_vec(raw_data)
    }

    // Properly align the buffer to MIN_ALIGNMENT bytes.
    pub(crate) fn from_vec(mut vec: Vec<u8>) -> Self {
        let shift = vec.as_ptr() as usize % MIN_ALIGNMENT;
        let start = if shift == 0 {
            0
        } else {
            vec.reserve(vec.len() + MIN_ALIGNMENT); // vec.as_ptr() is changed
            let shift = vec.as_ptr() as usize % MIN_ALIGNMENT;
            let padding = MIN_ALIGNMENT - shift;
            vec.splice(0..0, vec![0u8; padding]);
            padding
        };

        let memory = Self {
            raw_data: vec,
            start,
        };
        assert!(memory.data().as_ptr() as usize % MIN_ALIGNMENT == 0);
        memory
    }

    pub(crate) fn filter_list(&self) -> fb::NetworkFilterList<'_> {
        unsafe { fb::root_as_network_filter_list_unchecked(self.data()) }
    }

    pub fn data(&self) -> &[u8] {
        &self.raw_data[self.start..]
    }
}
