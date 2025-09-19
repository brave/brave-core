/// Domain separation label to initialize the STROBE context.
///
/// This is not to be confused with the crate's semver string:
/// the latter applies to the API, while this label defines the protocol.
/// E.g. it is possible that crate 2.0 will have an incompatible API,
/// but implement the same 1.0 protocol.
pub const MERLIN_PROTOCOL_LABEL: &[u8] = b"Merlin v1.0";
