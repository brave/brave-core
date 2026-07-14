# Design

## General design notes

### Requirements

- Keep the design close to Sapling, while eliminating aspects we don't like.

### Non-requirements

- Delegated proving with privacy from the prover.
  - We know how to do this, but it would require a discrete log equality proof, and the
    most efficient way to do this would be to do RedDSA and this at the same time, which
    means more work for e.g. hardware wallets.

### Open issues

- Should we have one memo per output, or one memo per transaction, or 0..n memos?
  - Variable, or (1 or n), is a potential privacy leak.
  - Need to consider the privacy issue related to light clients requesting individual
    memos vs being able to fetch all memos.

### Note structure

- TODO: UDAs: arbitrary vs whitelisted

### Typed variables vs byte encodings

For Sapling, we have encountered multiple places where the specification uses typed
variables to define the consensus rules, but the C++ implementation in zcashd relied on
byte encodings to implement them. This resulted in subtly-different consensus rules being
deployed than were intended, for example where a particular type was not round-trip
encodable.

In Orchard, we avoid this by defining the consensus rules in terms of the byte encodings
of all variables, and being explicit about any types that are not round-trip encodable.
This makes consensus compatibility between strongly-typed implementations (such as this
crate) and byte-oriented implementations easier to achieve.
