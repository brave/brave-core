//! Fast pseudo-random numbers whose sequence differs across concurrently running processes.
//!
//! These numbers are *not* cryptographically secure. Unlike the global generators of the
//! [`fastrand`] crate, whose per-thread seed is derived only from values that can coincide between
//! separate processes (such as small thread IDs), the generator here is seeded from a high-entropy
//! source so that concurrent processes don't easily share a sequence.
//!
//! This matters wherever a collision causes a real problem, such as the jitter used for retry
//! backoff (to avoid a [thundering herd](https://en.wikipedia.org/wiki/Thundering_herd_problem)) or
//! the temporary file names used while probing filesystem capabilities.

use std::{cell::Cell, ops::RangeBounds};

thread_local! {
    static RNG: Cell<fastrand::Rng> = Cell::new(new_rng());
}

/// Return an unsigned number in the given `range`, drawn from a per-thread generator that is seeded to differ
/// across concurrently running processes.
pub fn usize(range: impl RangeBounds<usize>) -> usize {
    RNG.with(|cell| {
        let mut rng = cell.replace(fastrand::Rng::with_seed(0));
        let n = rng.usize(range);
        cell.set(rng);
        n
    })
}

#[cfg(not(all(target_arch = "wasm32", target_os = "unknown")))]
fn new_rng() -> fastrand::Rng {
    fastrand::Rng::with_seed(seed())
}

/// `wasm32-unknown-unknown` has no OS entropy source and traps at runtime on APIs like
/// `std::process::id()` and `Instant::now()`; it also runs a single process, so the cross-process
/// concern doesn't apply. Use a fixed seed there.
#[cfg(all(target_arch = "wasm32", target_os = "unknown"))]
fn new_rng() -> fastrand::Rng {
    fastrand::Rng::with_seed(0x9e37_79b9_7f4a_7c15)
}

/// Seed a generator from a high-entropy OS source, so that generators in separate processes don't
/// share a sequence.
#[cfg(not(all(target_arch = "wasm32", target_os = "unknown")))]
fn seed() -> u64 {
    getrandom::u64().unwrap_or_else(|_| fallback_seed())
}

/// A best-effort seed used only when the OS entropy source is unavailable. `std::process::id()` is
/// the reliable differentiator across concurrent processes.
#[cfg(not(all(target_arch = "wasm32", target_os = "unknown")))]
fn fallback_seed() -> u64 {
    use std::hash::{BuildHasher, Hash, Hasher};

    let mut hasher = std::collections::hash_map::RandomState::new().build_hasher();
    std::process::id().hash(&mut hasher);
    std::thread::current().id().hash(&mut hasher);
    hasher.finish()
}

#[cfg(test)]
mod tests {
    #[test]
    fn values_are_in_range_and_not_constant() {
        let mut seen = std::collections::HashSet::new();
        for _ in 0..256 {
            let n = super::usize(0..1_000);
            assert!(n < 1_000, "value {n} must be within the requested range");
            seen.insert(n);
        }
        assert!(
            seen.len() > 1,
            "the generator wasn't made for the Playstation 3 (imprecise, but still funny?)"
        );
    }
}
