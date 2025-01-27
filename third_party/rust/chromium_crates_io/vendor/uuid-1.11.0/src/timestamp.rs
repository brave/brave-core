//! Generating UUIDs from timestamps.
//!
//! Timestamps are used in a few UUID versions as a source of decentralized
//! uniqueness (as in versions 1 and 6), and as a way to enable sorting (as
//! in versions 6 and 7). Timestamps aren't encoded the same way by all UUID
//! versions so this module provides a single [`Timestamp`] type that can
//! convert between them.
//!
//! # Timestamp representations in UUIDs
//!
//! Versions 1 and 6 UUIDs use a bespoke timestamp that consists of the
//! number of 100ns ticks since `1582-10-15 00:00:00`, along with
//! a counter value to avoid duplicates.
//!
//! Version 7 UUIDs use a more standard timestamp that consists of the
//! number of millisecond ticks since the Unix epoch (`1970-01-01 00:00:00`).
//!
//! # References
//!
//! * [UUID Version 1 in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-5.1)
//! * [UUID Version 7 in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-5.7)
//! * [Timestamp Considerations in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-6.1)

use core::cmp;

use crate::Uuid;

/// The number of 100 nanosecond ticks between the RFC 9562 epoch
/// (`1582-10-15 00:00:00`) and the Unix epoch (`1970-01-01 00:00:00`).
pub const UUID_TICKS_BETWEEN_EPOCHS: u64 = 0x01B2_1DD2_1381_4000;

/// A timestamp that can be encoded into a UUID.
///
/// This type abstracts the specific encoding, so versions 1, 6, and 7
/// UUIDs can both be supported through the same type, even
/// though they have a different representation of a timestamp.
///
/// # References
///
/// * [Timestamp Considerations in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-6.1)
/// * [UUID Generator States in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-6.3)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Timestamp {
    seconds: u64,
    subsec_nanos: u32,
    counter: u128,
    usable_counter_bits: u8,
}

impl Timestamp {
    /// Get a timestamp representing the current system time and up to a 128-bit counter.
    ///
    /// This method defers to the standard library's `SystemTime` type.
    #[cfg(feature = "std")]
    pub fn now(context: impl ClockSequence<Output = impl Into<u128>>) -> Self {
        let (seconds, subsec_nanos) = now();

        let (counter, seconds, subsec_nanos) =
            context.generate_timestamp_sequence(seconds, subsec_nanos);
        let counter = counter.into();
        let usable_counter_bits = context.usable_bits() as u8;

        Timestamp {
            seconds,
            subsec_nanos,
            counter,
            usable_counter_bits,
        }
    }

    /// Construct a `Timestamp` from the number of 100 nanosecond ticks since 00:00:00.00,
    /// 15 October 1582 (the date of Gregorian reform to the Christian calendar) and a 14-bit
    /// counter, as used in versions 1 and 6 UUIDs.
    ///
    /// # Overflow
    ///
    /// If conversion from RFC 9562 ticks to the internal timestamp format would overflow
    /// it will wrap.
    pub const fn from_gregorian(ticks: u64, counter: u16) -> Self {
        let (seconds, subsec_nanos) = Self::gregorian_to_unix(ticks);

        Timestamp {
            seconds,
            subsec_nanos,
            counter: counter as u128,
            usable_counter_bits: 14,
        }
    }

    /// Construct a `Timestamp` from a Unix timestamp and up to a 128-bit counter, as used in version 7 UUIDs.
    pub const fn from_unix_time(
        seconds: u64,
        subsec_nanos: u32,
        counter: u128,
        usable_counter_bits: u8,
    ) -> Self {
        Timestamp {
            seconds,
            subsec_nanos,
            counter,
            usable_counter_bits,
        }
    }

    /// Construct a `Timestamp` from a Unix timestamp and up to a 128-bit counter, as used in version 7 UUIDs.
    pub fn from_unix(
        context: impl ClockSequence<Output = impl Into<u128>>,
        seconds: u64,
        subsec_nanos: u32,
    ) -> Self {
        let (counter, seconds, subsec_nanos) =
            context.generate_timestamp_sequence(seconds, subsec_nanos);
        let counter = counter.into();
        let usable_counter_bits = context.usable_bits() as u8;

        Timestamp {
            seconds,
            subsec_nanos,
            counter,
            usable_counter_bits,
        }
    }

    /// Get the value of the timestamp as the number of 100 nanosecond ticks since 00:00:00.00,
    /// 15 October 1582 and a 14-bit counter, as used in versions 1 and 6 UUIDs.
    ///
    /// # Overflow
    ///
    /// If conversion from the internal timestamp format to ticks would overflow
    /// then it will wrap.
    /// 
    /// If the internal counter is wider than 14 bits then it will be truncated to 14 bits.
    pub const fn to_gregorian(&self) -> (u64, u16) {
        (
            Self::unix_to_gregorian_ticks(self.seconds, self.subsec_nanos),
            (self.counter as u16) & 0x3FFF,
        )
    }

    // NOTE: This method is not public; the usable counter bits are lost in a version 7 UUID
    // so can't be reliably recovered.
    #[cfg(feature = "v7")]
    pub(crate) const fn counter(&self) -> (u128, u8) {
        (self.counter, self.usable_counter_bits)
    }

    /// Get the value of the timestamp as a Unix timestamp, as used in version 7 UUIDs.
    pub const fn to_unix(&self) -> (u64, u32) {
        (self.seconds, self.subsec_nanos)
    }

    const fn unix_to_gregorian_ticks(seconds: u64, nanos: u32) -> u64 {
        UUID_TICKS_BETWEEN_EPOCHS
            .wrapping_add(seconds.wrapping_mul(10_000_000))
            .wrapping_add(nanos as u64 / 100)
    }

    const fn gregorian_to_unix(ticks: u64) -> (u64, u32) {
        (
            ticks.wrapping_sub(UUID_TICKS_BETWEEN_EPOCHS) / 10_000_000,
            (ticks.wrapping_sub(UUID_TICKS_BETWEEN_EPOCHS) % 10_000_000) as u32 * 100,
        )
    }
}

#[doc(hidden)]
impl Timestamp {
    #[deprecated(since = "1.11.0", note = "use `Timestamp::from_gregorian(ticks, counter)`")]
    pub const fn from_rfc4122(ticks: u64, counter: u16) -> Self {
        Timestamp::from_gregorian(ticks, counter)
    }

    #[deprecated(since = "1.11.0", note = "use `Timestamp::to_gregorian()`")]
    pub const fn to_rfc4122(&self) -> (u64, u16) {
        self.to_gregorian()
    }

    #[deprecated(since = "1.2.0", note = "`Timestamp::to_unix_nanos()` is deprecated and will be removed: use `Timestamp::to_unix()`")]
    pub const fn to_unix_nanos(&self) -> u32 {
        panic!("`Timestamp::to_unix_nanos()` is deprecated and will be removed: use `Timestamp::to_unix()`")
    }
}

pub(crate) const fn encode_gregorian_timestamp(
    ticks: u64,
    counter: u16,
    node_id: &[u8; 6],
) -> Uuid {
    let time_low = (ticks & 0xFFFF_FFFF) as u32;
    let time_mid = ((ticks >> 32) & 0xFFFF) as u16;
    let time_high_and_version = (((ticks >> 48) & 0x0FFF) as u16) | (1 << 12);

    let mut d4 = [0; 8];

    d4[0] = (((counter & 0x3F00) >> 8) as u8) | 0x80;
    d4[1] = (counter & 0xFF) as u8;
    d4[2] = node_id[0];
    d4[3] = node_id[1];
    d4[4] = node_id[2];
    d4[5] = node_id[3];
    d4[6] = node_id[4];
    d4[7] = node_id[5];

    Uuid::from_fields(time_low, time_mid, time_high_and_version, &d4)
}

pub(crate) const fn decode_gregorian_timestamp(uuid: &Uuid) -> (u64, u16) {
    let bytes = uuid.as_bytes();

    let ticks: u64 = ((bytes[6] & 0x0F) as u64) << 56
        | (bytes[7] as u64) << 48
        | (bytes[4] as u64) << 40
        | (bytes[5] as u64) << 32
        | (bytes[0] as u64) << 24
        | (bytes[1] as u64) << 16
        | (bytes[2] as u64) << 8
        | (bytes[3] as u64);

    let counter: u16 = ((bytes[8] & 0x3F) as u16) << 8 | (bytes[9] as u16);

    (ticks, counter)
}

pub(crate) const fn encode_sorted_gregorian_timestamp(
    ticks: u64,
    counter: u16,
    node_id: &[u8; 6],
) -> Uuid {
    let time_high = ((ticks >> 28) & 0xFFFF_FFFF) as u32;
    let time_mid = ((ticks >> 12) & 0xFFFF) as u16;
    let time_low_and_version = ((ticks & 0x0FFF) as u16) | (0x6 << 12);

    let mut d4 = [0; 8];

    d4[0] = (((counter & 0x3F00) >> 8) as u8) | 0x80;
    d4[1] = (counter & 0xFF) as u8;
    d4[2] = node_id[0];
    d4[3] = node_id[1];
    d4[4] = node_id[2];
    d4[5] = node_id[3];
    d4[6] = node_id[4];
    d4[7] = node_id[5];

    Uuid::from_fields(time_high, time_mid, time_low_and_version, &d4)
}

pub(crate) const fn decode_sorted_gregorian_timestamp(uuid: &Uuid) -> (u64, u16) {
    let bytes = uuid.as_bytes();

    let ticks: u64 = ((bytes[0]) as u64) << 52
        | (bytes[1] as u64) << 44
        | (bytes[2] as u64) << 36
        | (bytes[3] as u64) << 28
        | (bytes[4] as u64) << 20
        | (bytes[5] as u64) << 12
        | ((bytes[6] & 0xF) as u64) << 8
        | (bytes[7] as u64);

    let counter: u16 = ((bytes[8] & 0x3F) as u16) << 8 | (bytes[9] as u16);

    (ticks, counter)
}

pub(crate) const fn encode_unix_timestamp_millis(
    millis: u64,
    counter_random_bytes: &[u8; 10],
) -> Uuid {
    let millis_high = ((millis >> 16) & 0xFFFF_FFFF) as u32;
    let millis_low = (millis & 0xFFFF) as u16;

    let counter_random_version = (counter_random_bytes[1] as u16
        | ((counter_random_bytes[0] as u16) << 8) & 0x0FFF)
        | (0x7 << 12);

    let mut d4 = [0; 8];

    d4[0] = (counter_random_bytes[2] & 0x3F) | 0x80;
    d4[1] = counter_random_bytes[3];
    d4[2] = counter_random_bytes[4];
    d4[3] = counter_random_bytes[5];
    d4[4] = counter_random_bytes[6];
    d4[5] = counter_random_bytes[7];
    d4[6] = counter_random_bytes[8];
    d4[7] = counter_random_bytes[9];

    Uuid::from_fields(millis_high, millis_low, counter_random_version, &d4)
}

pub(crate) const fn decode_unix_timestamp_millis(uuid: &Uuid) -> u64 {
    let bytes = uuid.as_bytes();

    let millis: u64 = (bytes[0] as u64) << 40
        | (bytes[1] as u64) << 32
        | (bytes[2] as u64) << 24
        | (bytes[3] as u64) << 16
        | (bytes[4] as u64) << 8
        | (bytes[5] as u64);

    millis
}

#[cfg(all(
    feature = "std",
    feature = "js",
    all(
        target_arch = "wasm32",
        target_vendor = "unknown",
        target_os = "unknown"
    )
))]
fn now() -> (u64, u32) {
    use wasm_bindgen::prelude::*;

    #[wasm_bindgen]
    extern "C" {
        // NOTE: This signature works around https://bugzilla.mozilla.org/show_bug.cgi?id=1787770
        #[wasm_bindgen(js_namespace = Date, catch)]
        fn now() -> Result<f64, JsValue>;
    }

    let now = now().unwrap_throw();

    let secs = (now / 1_000.0) as u64;
    let nanos = ((now % 1_000.0) * 1_000_000.0) as u32;

    (secs, nanos)
}

#[cfg(all(
    feature = "std",
    not(miri),
    any(
        not(feature = "js"),
        not(all(
            target_arch = "wasm32",
            target_vendor = "unknown",
            target_os = "unknown"
        ))
    )
))]
fn now() -> (u64, u32) {
    let dur = std::time::SystemTime::UNIX_EPOCH.elapsed().expect(
        "Getting elapsed time since UNIX_EPOCH. If this fails, we've somehow violated causality",
    );

    (dur.as_secs(), dur.subsec_nanos())
}

#[cfg(all(feature = "std", miri))]
fn now() -> (u64, u32) {
    use std::{sync::Mutex, time::Duration};

    static TS: Mutex<u64> = Mutex::new(0);

    let ts = Duration::from_nanos({
        let mut ts = TS.lock().unwrap();
        *ts += 1;
        *ts
    });

    (ts.as_secs(), ts.subsec_nanos())
}

/// A counter that can be used by versions 1 and 6 UUIDs to support
/// the uniqueness of timestamps.
///
/// # References
///
/// * [UUID Version 1 in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-5.1)
/// * [UUID Version 6 in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-5.6)
/// * [UUID Generator States in RFC 9562](https://www.ietf.org/rfc/rfc9562.html#section-6.3)
pub trait ClockSequence {
    /// The type of sequence returned by this counter.
    type Output;

    /// Get the next value in the sequence to feed into a timestamp.
    ///
    /// This method will be called each time a [`Timestamp`] is constructed.
    ///
    /// Any bits beyond [`ClockSequence::usable_bits`] in the output must be unset.
    fn generate_sequence(&self, seconds: u64, subsec_nanos: u32) -> Self::Output;

    /// Get the next value in the sequence, potentially also adjusting the timestamp.
    ///
    /// This method should be preferred over `generate_sequence`.
    ///
    /// Any bits beyond [`ClockSequence::usable_bits`] in the output must be unset.
    fn generate_timestamp_sequence(
        &self,
        seconds: u64,
        subsec_nanos: u32,
    ) -> (Self::Output, u64, u32) {
        (
            self.generate_sequence(seconds, subsec_nanos),
            seconds,
            subsec_nanos,
        )
    }

    /// The number of usable bits from the least significant bit in the result of [`ClockSequence::generate_sequence`]
    /// or [`ClockSequence::generate_timestamp_sequence`].
    ///
    /// The number of usable bits must not exceed 128.
    ///
    /// The number of usable bits is not expected to change between calls. An implementation of `ClockSequence` should
    /// always return the same value from this method.
    fn usable_bits(&self) -> usize
    where
        Self::Output: Sized,
    {
        cmp::min(128, core::mem::size_of::<Self::Output>())
    }
}

impl<'a, T: ClockSequence + ?Sized> ClockSequence for &'a T {
    type Output = T::Output;

    fn generate_sequence(&self, seconds: u64, subsec_nanos: u32) -> Self::Output {
        (**self).generate_sequence(seconds, subsec_nanos)
    }

    fn generate_timestamp_sequence(
        &self,
        seconds: u64,
        subsec_nanos: u32,
    ) -> (Self::Output, u64, u32) {
        (**self).generate_timestamp_sequence(seconds, subsec_nanos)
    }

    fn usable_bits(&self) -> usize
    where
        Self::Output: Sized,
    {
        (**self).usable_bits()
    }
}

/// Default implementations for the [`ClockSequence`] trait.
pub mod context {
    use super::ClockSequence;

    #[cfg(any(feature = "v1", feature = "v6"))]
    mod v1_support {
        use super::*;

        use atomic::{Atomic, Ordering};

        #[cfg(all(feature = "std", feature = "rng"))]
        static CONTEXT: Context = Context {
            count: Atomic::new(0),
        };

        #[cfg(all(feature = "std", feature = "rng"))]
        static CONTEXT_INITIALIZED: Atomic<bool> = Atomic::new(false);

        #[cfg(all(feature = "std", feature = "rng"))]
        pub(crate) fn shared_context() -> &'static Context {
            // If the context is in its initial state then assign it to a random value
            // It doesn't matter if multiple threads observe `false` here and initialize the context
            if CONTEXT_INITIALIZED
                .compare_exchange(false, true, Ordering::Relaxed, Ordering::Relaxed)
                .is_ok()
            {
                CONTEXT.count.store(crate::rng::u16(), Ordering::Release);
            }

            &CONTEXT
        }

        /// A thread-safe, wrapping counter that produces 14-bit values.
        ///
        /// This type works by:
        ///
        /// 1. Atomically incrementing the counter value for each timestamp.
        /// 2. Wrapping the counter back to zero if it overflows its 14-bit storage.
        ///
        /// This type should be used when constructing versions 1 and 6 UUIDs.
        ///
        /// This type should not be used when constructing version 7 UUIDs. When used to
        /// construct a version 7 UUID, the 14-bit counter will be padded with random data.
        /// Counter overflows are more likely with a 14-bit counter than they are with a
        /// 42-bit counter when working at millisecond precision. This type doesn't attempt
        /// to adjust the timestamp on overflow.
        #[derive(Debug)]
        pub struct Context {
            count: Atomic<u16>,
        }

        impl Context {
            /// Construct a new context that's initialized with the given value.
            ///
            /// The starting value should be a random number, so that UUIDs from
            /// different systems with the same timestamps are less likely to collide.
            /// When the `rng` feature is enabled, prefer the [`Context::new_random`] method.
            pub const fn new(count: u16) -> Self {
                Self {
                    count: Atomic::<u16>::new(count),
                }
            }

            /// Construct a new context that's initialized with a random value.
            #[cfg(feature = "rng")]
            pub fn new_random() -> Self {
                Self {
                    count: Atomic::<u16>::new(crate::rng::u16()),
                }
            }
        }

        impl ClockSequence for Context {
            type Output = u16;

            fn generate_sequence(&self, _seconds: u64, _nanos: u32) -> Self::Output {
                // RFC 9562 reserves 2 bits of the clock sequence so the actual
                // maximum value is smaller than `u16::MAX`. Since we unconditionally
                // increment the clock sequence we want to wrap once it becomes larger
                // than what we can represent in a "u14". Otherwise there'd be patches
                // where the clock sequence doesn't change regardless of the timestamp
                self.count.fetch_add(1, Ordering::AcqRel) & (u16::MAX >> 2)
            }

            fn usable_bits(&self) -> usize {
                14
            }
        }

        #[cfg(test)]
        mod tests {
            use crate::Timestamp;

            use super::*;

            #[test]
            fn context() {
                let seconds = 1_496_854_535;
                let subsec_nanos = 812_946_000;

                let context = Context::new(u16::MAX >> 2);

                let ts = Timestamp::from_unix(&context, seconds, subsec_nanos);
                assert_eq!(16383, ts.counter);
                assert_eq!(14, ts.usable_counter_bits);

                let seconds = 1_496_854_536;

                let ts = Timestamp::from_unix(&context, seconds, subsec_nanos);
                assert_eq!(0, ts.counter);

                let seconds = 1_496_854_535;

                let ts = Timestamp::from_unix(&context, seconds, subsec_nanos);
                assert_eq!(1, ts.counter);
            }
        }
    }

    #[cfg(any(feature = "v1", feature = "v6"))]
    pub use v1_support::*;

    #[cfg(feature = "std")]
    mod std_support {
        use super::*;

        use core::panic::{AssertUnwindSafe, RefUnwindSafe};
        use std::{sync::Mutex, thread::LocalKey};

        /// A wrapper for a context that uses thread-local storage.
        pub struct ThreadLocalContext<C: 'static>(&'static LocalKey<C>);

        impl<C> std::fmt::Debug for ThreadLocalContext<C> {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                f.debug_struct("ThreadLocalContext").finish_non_exhaustive()
            }
        }

        impl<C: 'static> ThreadLocalContext<C> {
            /// Wrap a thread-local container with a context.
            pub const fn new(local_key: &'static LocalKey<C>) -> Self {
                ThreadLocalContext(local_key)
            }
        }

        impl<C: ClockSequence + 'static> ClockSequence for ThreadLocalContext<C> {
            type Output = C::Output;

            fn generate_sequence(&self, seconds: u64, subsec_nanos: u32) -> Self::Output {
                self.0
                    .with(|ctxt| ctxt.generate_sequence(seconds, subsec_nanos))
            }

            fn generate_timestamp_sequence(
                &self,
                seconds: u64,
                subsec_nanos: u32,
            ) -> (Self::Output, u64, u32) {
                self.0
                    .with(|ctxt| ctxt.generate_timestamp_sequence(seconds, subsec_nanos))
            }

            fn usable_bits(&self) -> usize {
                self.0.with(|ctxt| ctxt.usable_bits())
            }
        }

        impl<C: ClockSequence> ClockSequence for AssertUnwindSafe<C> {
            type Output = C::Output;

            fn generate_sequence(&self, seconds: u64, subsec_nanos: u32) -> Self::Output {
                self.0.generate_sequence(seconds, subsec_nanos)
            }

            fn generate_timestamp_sequence(
                &self,
                seconds: u64,
                subsec_nanos: u32,
            ) -> (Self::Output, u64, u32) {
                self.0.generate_timestamp_sequence(seconds, subsec_nanos)
            }

            fn usable_bits(&self) -> usize
            where
                Self::Output: Sized,
            {
                self.0.usable_bits()
            }
        }

        impl<C: ClockSequence + RefUnwindSafe> ClockSequence for Mutex<C> {
            type Output = C::Output;

            fn generate_sequence(&self, seconds: u64, subsec_nanos: u32) -> Self::Output {
                self.lock()
                    .unwrap_or_else(|err| err.into_inner())
                    .generate_sequence(seconds, subsec_nanos)
            }

            fn generate_timestamp_sequence(
                &self,
                seconds: u64,
                subsec_nanos: u32,
            ) -> (Self::Output, u64, u32) {
                self.lock()
                    .unwrap_or_else(|err| err.into_inner())
                    .generate_timestamp_sequence(seconds, subsec_nanos)
            }

            fn usable_bits(&self) -> usize
            where
                Self::Output: Sized,
            {
                self.lock()
                    .unwrap_or_else(|err| err.into_inner())
                    .usable_bits()
            }
        }
    }

    #[cfg(feature = "std")]
    pub use std_support::*;

    #[cfg(feature = "v7")]
    mod v7_support {
        use super::*;

        use core::{cell::Cell, panic::RefUnwindSafe};

        #[cfg(feature = "std")]
        static CONTEXT_V7: SharedContextV7 =
            SharedContextV7(std::sync::Mutex::new(ContextV7::new()));

        #[cfg(feature = "std")]
        pub(crate) fn shared_context_v7() -> &'static SharedContextV7 {
            &CONTEXT_V7
        }

        const USABLE_BITS: usize = 42;

        // Leave the most significant bit unset
        // This guarantees the counter has at least 2,199,023,255,552
        // values before it will overflow, which is exceptionally unlikely
        // even in the worst case
        const RESEED_MASK: u64 = u64::MAX >> 23;
        const MAX_COUNTER: u64 = u64::MAX >> 22;

        /// An unsynchronized, reseeding counter that produces 42-bit values.
        ///
        /// This type works by:
        ///
        /// 1. Reseeding the counter each millisecond with a random 41-bit value. The 42nd bit
        ///    is left unset so the counter can safely increment over the millisecond.
        /// 2. Wrapping the counter back to zero if it overflows its 42-bit storage and adding a
        ///    millisecond to the timestamp.
        ///
        /// This type can be used when constructing version 7 UUIDs. When used to construct a
        /// version 7 UUID, the 42-bit counter will be padded with random data. This type can
        /// be used to maintain ordering of UUIDs within the same millisecond.
        ///
        /// This type should not be used when constructing version 1 or version 6 UUIDs.
        /// When used to construct a version 1 or version 6 UUID, only the 14 least significant
        /// bits of the counter will be used.
        #[derive(Debug)]
        pub struct ContextV7 {
            last_reseed: Cell<LastReseed>,
            counter: Cell<u64>,
        }

        #[derive(Debug, Default, Clone, Copy)]
        struct LastReseed {
            millis: u64,
            ts_seconds: u64,
            ts_subsec_nanos: u32,
        }

        impl LastReseed {
            fn from_millis(millis: u64) -> Self {
                LastReseed {
                    millis,
                    ts_seconds: millis / 1_000,
                    ts_subsec_nanos: (millis % 1_000) as u32 * 1_000_000,
                }
            }
        }

        impl RefUnwindSafe for ContextV7 {}

        impl ContextV7 {
            /// Construct a new context that will reseed its counter on the first
            /// non-zero timestamp it receives.
            pub const fn new() -> Self {
                ContextV7 {
                    last_reseed: Cell::new(LastReseed {
                        millis: 0,
                        ts_seconds: 0,
                        ts_subsec_nanos: 0,
                    }),
                    counter: Cell::new(0),
                }
            }
        }

        impl ClockSequence for ContextV7 {
            type Output = u64;

            fn generate_sequence(&self, seconds: u64, subsec_nanos: u32) -> Self::Output {
                self.generate_timestamp_sequence(seconds, subsec_nanos).0
            }

            fn generate_timestamp_sequence(
                &self,
                seconds: u64,
                subsec_nanos: u32,
            ) -> (Self::Output, u64, u32) {
                let millis = (seconds * 1_000).saturating_add(subsec_nanos as u64 / 1_000_000);

                let last_reseed = self.last_reseed.get();

                // If the observed system time has shifted forwards then regenerate the counter
                if millis > last_reseed.millis {
                    let last_reseed = LastReseed::from_millis(millis);
                    self.last_reseed.set(last_reseed);

                    let counter = crate::rng::u64() & RESEED_MASK;
                    self.counter.set(counter);

                    (counter, last_reseed.ts_seconds, last_reseed.ts_subsec_nanos)
                }
                // If the observed system time has not shifted forwards then increment the counter
                else {
                    // If the incoming timestamp is earlier than the last observed one then
                    // use it instead. This may happen if the system clock jitters, or if the counter
                    // has wrapped and the timestamp is artificially incremented
                    let millis = ();
                    let _ = millis;

                    // Guaranteed to never overflow u64
                    let counter = self.counter.get() + 1;

                    // If the counter has not overflowed its 42-bit storage then return it
                    if counter <= MAX_COUNTER {
                        self.counter.set(counter);

                        (counter, last_reseed.ts_seconds, last_reseed.ts_subsec_nanos)
                    }
                    // Unlikely: If the counter has overflowed its 42-bit storage then wrap it
                    // and increment the timestamp. Until the observed system time shifts past
                    // this incremented value, all timestamps will use it to maintain monotonicity
                    else {
                        // Increment the timestamp by 1 milli
                        let last_reseed = LastReseed::from_millis(last_reseed.millis + 1);
                        self.last_reseed.set(last_reseed);

                        // Reseed the counter
                        let counter = crate::rng::u64() & RESEED_MASK;
                        self.counter.set(counter);

                        (counter, last_reseed.ts_seconds, last_reseed.ts_subsec_nanos)
                    }
                }
            }

            fn usable_bits(&self) -> usize {
                USABLE_BITS
            }
        }

        #[cfg(feature = "std")]
        pub(crate) struct SharedContextV7(std::sync::Mutex<ContextV7>);

        #[cfg(feature = "std")]
        impl ClockSequence for SharedContextV7 {
            type Output = u64;

            fn generate_sequence(&self, seconds: u64, subsec_nanos: u32) -> Self::Output {
                self.0.generate_sequence(seconds, subsec_nanos)
            }

            fn generate_timestamp_sequence(
                &self,
                seconds: u64,
                subsec_nanos: u32,
            ) -> (Self::Output, u64, u32) {
                self.0.generate_timestamp_sequence(seconds, subsec_nanos)
            }

            fn usable_bits(&self) -> usize
            where
                Self::Output: Sized,
            {
                USABLE_BITS
            }
        }

        #[cfg(test)]
        mod tests {
            use core::time::Duration;

            use super::*;

            use crate::Timestamp;

            #[test]
            fn context() {
                let seconds = 1_496_854_535;
                let subsec_nanos = 812_946_000;

                let context = ContextV7::new();

                let ts1 = Timestamp::from_unix(&context, seconds, subsec_nanos);
                assert_eq!(42, ts1.usable_counter_bits);

                // Backwards second
                let seconds = 1_496_854_534;

                let ts2 = Timestamp::from_unix(&context, seconds, subsec_nanos);

                // The backwards time should be ignored
                // The counter should still increment
                assert_eq!(ts1.seconds, ts2.seconds);
                assert_eq!(ts1.subsec_nanos, ts2.subsec_nanos);
                assert_eq!(ts1.counter + 1, ts2.counter);

                // Forwards second
                let seconds = 1_496_854_536;

                let ts3 = Timestamp::from_unix(&context, seconds, subsec_nanos);

                // The counter should have reseeded
                assert_ne!(ts2.counter + 1, ts3.counter);
                assert_ne!(0, ts3.counter);
            }

            #[test]
            fn context_wrap() {
                let seconds = 1_496_854_535u64;
                let subsec_nanos = 812_946_000u32;

                let millis = (seconds * 1000).saturating_add(subsec_nanos as u64 / 1_000_000);

                // This context will wrap
                let context = ContextV7 {
                    last_reseed: Cell::new(LastReseed::from_millis(millis)),
                    counter: Cell::new(u64::MAX >> 22),
                };

                let ts = Timestamp::from_unix(&context, seconds, subsec_nanos);

                // The timestamp should be incremented by 1ms
                let expected_ts = Duration::new(seconds, subsec_nanos / 1_000_000 * 1_000_000)
                    + Duration::from_millis(1);
                assert_eq!(expected_ts.as_secs(), ts.seconds);
                assert_eq!(expected_ts.subsec_nanos(), ts.subsec_nanos);

                // The counter should have reseeded
                assert!(ts.counter < (u64::MAX >> 22) as u128);
                assert_ne!(0, ts.counter);
            }
        }
    }

    #[cfg(feature = "v7")]
    pub use v7_support::*;

    /// An empty counter that will always return the value `0`.
    ///
    /// This type can be used when constructing version 7 UUIDs. When used to
    /// construct a version 7 UUID, the entire counter segment of the UUID will be
    /// filled with a random value. This type does not maintain ordering of UUIDs
    /// within a millisecond but is efficient.
    ///
    /// This type should not be used when constructing version 1 or version 6 UUIDs.
    /// When used to construct a version 1 or version 6 UUID, the counter
    /// segment will remain zero.
    #[derive(Debug, Clone, Copy, Default)]
    pub struct NoContext;

    impl ClockSequence for NoContext {
        type Output = u16;

        fn generate_sequence(&self, _seconds: u64, _nanos: u32) -> Self::Output {
            0
        }

        fn usable_bits(&self) -> usize {
            0
        }
    }
}

#[cfg(all(test, any(feature = "v1", feature = "v6")))]
mod tests {
    use super::*;

    #[cfg(all(
        target_arch = "wasm32",
        target_vendor = "unknown",
        target_os = "unknown"
    ))]
    use wasm_bindgen_test::*;

    #[test]
    #[cfg_attr(
        all(
            target_arch = "wasm32",
            target_vendor = "unknown",
            target_os = "unknown"
        ),
        wasm_bindgen_test
    )]
    fn gregorian_unix_does_not_panic() {
        // Ensure timestamp conversions never panic
        Timestamp::unix_to_gregorian_ticks(u64::MAX, 0);
        Timestamp::unix_to_gregorian_ticks(0, u32::MAX);
        Timestamp::unix_to_gregorian_ticks(u64::MAX, u32::MAX);

        Timestamp::gregorian_to_unix(u64::MAX);
    }

    #[test]
    #[cfg_attr(
        all(
            target_arch = "wasm32",
            target_vendor = "unknown",
            target_os = "unknown"
        ),
        wasm_bindgen_test
    )]
    fn to_gregorian_truncates_to_usable_bits() {
        let ts = Timestamp::from_gregorian(123, u16::MAX);

        assert_eq!((123, u16::MAX >> 2), ts.to_gregorian());
    }
}
