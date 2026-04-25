use core::mem::MaybeUninit;

pub const fn _assert_send<T: Send>() {}
pub const fn _assert_sync<T: Sync>() {}

#[derive(Debug, Default)]
pub struct PartialBuffer<B> {
    buffer: B,
    index: usize,
}

impl<B: AsRef<[u8]>> PartialBuffer<B> {
    pub fn new(buffer: B) -> Self {
        Self { buffer, index: 0 }
    }

    pub fn written(&self) -> &[u8] {
        &self.buffer.as_ref()[..self.index]
    }

    /// Convenient method for `.writen().len()`
    pub fn written_len(&self) -> usize {
        self.index
    }

    pub fn unwritten(&self) -> &[u8] {
        &self.buffer.as_ref()[self.index..]
    }

    pub fn advance(&mut self, amount: usize) {
        self.index += amount;
        debug_assert!(self.index <= self.buffer.as_ref().len());
    }

    pub fn get_mut(&mut self) -> &mut B {
        &mut self.buffer
    }

    pub fn into_inner(self) -> B {
        self.buffer
    }

    pub fn reset(&mut self) {
        self.index = 0;
    }
}

impl<B: AsRef<[u8]> + AsMut<[u8]>> PartialBuffer<B> {
    pub fn unwritten_mut(&mut self) -> &mut [u8] {
        &mut self.buffer.as_mut()[self.index..]
    }

    pub fn copy_unwritten_from<C: AsRef<[u8]>>(&mut self, other: &mut PartialBuffer<C>) -> usize {
        let len = self.unwritten().len().min(other.unwritten().len());

        self.unwritten_mut()[..len].copy_from_slice(&other.unwritten()[..len]);

        self.advance(len);
        other.advance(len);
        len
    }
}

impl<B: AsRef<[u8]> + Default> PartialBuffer<B> {
    pub fn take(&mut self) -> Self {
        std::mem::take(self)
    }
}

impl<B: AsRef<[u8]> + AsMut<[u8]>> From<B> for PartialBuffer<B> {
    fn from(buffer: B) -> Self {
        Self::new(buffer)
    }
}

/// Write buffer for compression-codecs.
///
/// Currently it only supports initialized buffer, but will support uninitialized
/// buffer soon.
///
/// # Layout
///
/// ```text
/// |                                       buffer                                    |
/// | written and initialized | unwritten but initialized | unwritten and uninitialized
/// ```
#[derive(Debug)]
pub struct WriteBuffer<'a> {
    buffer: &'a mut [MaybeUninit<u8>],
    index: usize,
    initialized: usize,
}

impl<'a> WriteBuffer<'a> {
    pub fn new_initialized(buffer: &'a mut [u8]) -> Self {
        Self {
            initialized: buffer.len(),
            // Safety: with initialized set to len of the buffer,
            // `WriteBuffer` would treat it as a `&mut [u8]`.
            buffer: unsafe { &mut *(buffer as *mut [u8] as *mut _) },
            index: 0,
        }
    }

    pub fn new_uninitialized(buffer: &'a mut [MaybeUninit<u8>]) -> Self {
        Self {
            buffer,
            index: 0,
            initialized: 0,
        }
    }

    pub fn capacity(&self) -> usize {
        self.buffer.len()
    }

    pub fn as_mut_ptr(&mut self) -> *mut u8 {
        self.buffer.as_mut_ptr() as *mut _
    }

    pub fn initialized_len(&self) -> usize {
        self.initialized
    }

    pub fn written(&self) -> &[u8] {
        debug_assert!(self.index <= self.initialized);

        unsafe { &*(&self.buffer[..self.index] as *const _ as *const [u8]) }
    }

    /// Convenient method for `.writen().len()`
    pub fn written_len(&self) -> usize {
        self.index
    }

    /// Buffer has no spare space to write any data
    pub fn has_no_spare_space(&self) -> bool {
        self.index == self.buffer.len()
    }

    /// Initialize all uninitialized, unwritten part to initialized, unwritten part
    /// Return all unwritten part
    pub fn initialize_unwritten(&mut self) -> &mut [u8] {
        self.buffer[self.initialized..]
            .iter_mut()
            .for_each(|maybe_uninit| {
                maybe_uninit.write(0);
            });
        self.initialized = self.buffer.len();

        unsafe { &mut *(&mut self.buffer[self.index..] as *mut _ as *mut [u8]) }
    }

    /// Advance written index within initialized part.
    ///
    /// Note that try to advance into uninitialized part would panic.
    pub fn advance(&mut self, amount: usize) {
        debug_assert!(self.index + amount <= self.buffer.len());
        debug_assert!(self.index + amount <= self.initialized);

        self.index += amount;
    }

    pub fn reset(&mut self) {
        self.index = 0;
    }

    /// Returns a mutable reference to the unwritten part of the buffer without
    /// ensuring that it has been fully initialized.
    ///
    /// # Safety
    ///
    /// The caller must not de-initialize portions of the buffer that have already
    /// been initialized.
    ///
    /// This includes any bytes in the region returned by this function.
    pub unsafe fn unwritten_mut(&mut self) -> &mut [MaybeUninit<u8>] {
        &mut self.buffer[self.index..]
    }

    /// Asserts that the first `n` unfilled bytes of the buffer are initialized.
    ///
    /// [`WriteBuffer`] assumes that bytes are never de-initialized, so this method
    /// does nothing when called with fewer bytes than are already known to be initialized.
    ///
    /// # Safety
    ///
    /// The caller must ensure that `n` unfilled bytes of the buffer have already been initialized.
    pub unsafe fn assume_init(&mut self, n: usize) {
        debug_assert!(self.index <= (self.initialized + n));
        debug_assert!((self.initialized + n) <= self.buffer.len());

        self.initialized += n;
    }

    /// Convenient function combining [`WriteBuffer::assume_init`] and [`WriteBuffer::advance`].
    ///
    /// # Safety
    ///
    /// The caller must ensure that `n` unfilled bytes of the buffer have already been initialized.
    pub unsafe fn assume_init_and_advance(&mut self, n: usize) {
        debug_assert!(self.index + n <= self.buffer.len());

        self.index += n;
        self.initialized = self.initialized.max(self.index);
    }

    /// Convenient function combining [`WriteBuffer::assume_init`] and [`WriteBuffer::advance`],
    /// works similar to [`Vec::set_len`].
    ///
    /// # Safety
    ///
    /// The caller must ensure that first `n` bytes of the buffer have already been initialized.
    pub unsafe fn set_written_and_initialized_len(&mut self, n: usize) {
        debug_assert!(n <= self.buffer.len());

        self.index = n;
        self.initialized = self.initialized.max(n);
    }

    pub fn copy_unwritten_from<C: AsRef<[u8]>>(&mut self, other: &mut PartialBuffer<C>) -> usize {
        fn inner(this: &mut WriteBuffer<'_>, input: &[u8]) -> usize {
            // Safety: We will never ever write uninitialized bytes into it
            let out = unsafe { this.unwritten_mut() };

            let len = out.len().min(input.len());

            out[..len]
                .iter_mut()
                .zip(&input[..len])
                .for_each(|(maybe_uninit, byte)| {
                    maybe_uninit.write(*byte);
                });

            // Safety: We have written `len` bytes of initialized data into it
            unsafe { this.assume_init_and_advance(len) };
            len
        }

        let len = inner(self, other.unwritten());
        other.advance(len);

        len
    }
}
