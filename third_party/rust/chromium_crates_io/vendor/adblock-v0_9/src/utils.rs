//! Common utilities used by the library. Some tests and benchmarks rely on this module having
//! public visibility.

#[cfg(target_pointer_width = "64")]
use seahash::hash;
#[cfg(target_pointer_width = "32")]
use seahash::reference::hash;

pub type Hash = u64;

#[inline]
pub fn fast_hash(input: &str) -> Hash {
    hash(input.as_bytes()) as Hash
}

#[inline]
fn is_allowed_filter(ch: char) -> bool {
    ch.is_alphanumeric() || ch == '%'
}

pub(crate) const TOKENS_BUFFER_SIZE: usize = 128;
pub(crate) const TOKENS_BUFFER_RESERVED: usize = 1;
const TOKENS_MAX: usize = TOKENS_BUFFER_SIZE - TOKENS_BUFFER_RESERVED;

fn fast_tokenizer_no_regex(
    pattern: &str,
    is_allowed_code: &dyn Fn(char) -> bool,
    skip_first_token: bool,
    skip_last_token: bool,
    tokens_buffer: &mut Vec<Hash>,
) {
    // let mut tokens_buffer_index = 0;
    let mut inside: bool = false;
    let mut start = 0;
    let mut preceding_ch: Option<char> = None; // Used to check if a '*' is not just before a token

    for (i, c) in pattern.char_indices() {
        if tokens_buffer.len() >= TOKENS_MAX {
            return;
        }
        if is_allowed_code(c) {
            if !inside {
                inside = true;
                start = i;
            }
        } else if inside {
            inside = false;
            // Should not be followed by '*'
            if (start != 0 || !skip_first_token)
                && i - start > 1
                && c != '*'
                && preceding_ch != Some('*')
            {
                let hash = fast_hash(&pattern[start..i]);
                tokens_buffer.push(hash);
            }
            preceding_ch = Some(c);
        } else {
            preceding_ch = Some(c);
        }
    }

    if !skip_last_token && inside && pattern.len() - start > 1 && (preceding_ch != Some('*')) {
        let hash = fast_hash(&pattern[start..]);
        tokens_buffer.push(hash);
    }
}

pub(crate) fn tokenize_pooled(pattern: &str, tokens_buffer: &mut Vec<Hash>) {
    fast_tokenizer_no_regex(pattern, &is_allowed_filter, false, false, tokens_buffer);
}

pub fn tokenize(pattern: &str) -> Vec<Hash> {
    let mut tokens_buffer: Vec<Hash> = Vec::with_capacity(TOKENS_BUFFER_SIZE);
    fast_tokenizer_no_regex(
        pattern,
        &is_allowed_filter,
        false,
        false,
        &mut tokens_buffer,
    );
    tokens_buffer
}

pub(crate) fn tokenize_filter(
    pattern: &str,
    skip_first_token: bool,
    skip_last_token: bool,
) -> Vec<Hash> {
    let mut tokens_buffer: Vec<Hash> = Vec::with_capacity(TOKENS_BUFFER_SIZE);
    fast_tokenizer_no_regex(
        pattern,
        &is_allowed_filter,
        skip_first_token,
        skip_last_token,
        &mut tokens_buffer,
    );
    tokens_buffer
}

pub(crate) fn bin_lookup<T: Ord>(arr: &[T], elt: T) -> bool {
    arr.binary_search(&elt).is_ok()
}

#[cfg(test)]
#[path = "../tests/unit/utils.rs"]
mod unit_tests;
