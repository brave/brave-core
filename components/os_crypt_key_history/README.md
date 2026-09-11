# OSCrypt key history

A short, on-disk record of encryption keys that have been proven to decrypt
this installation's data.

Tracking issue:
[brave-browser#40375](https://github.com/brave/brave-browser/issues/40375).

## Why this exists

On Windows the key that protects saved passwords, cookies and payment methods
is held in one place: `os_crypt.encrypted_key` in `Local State`, wrapped by
DPAPI. Nothing else on disk can reconstruct it. If that value is replaced or
lost, every piece of data encrypted under it becomes unreadable, with no way
back.

Two things could replace it. The browser could decide the profile has no key
and mint one, which is what happened when a failed `CryptUnprotectData()` was
treated the same as "no key stored". Or `Local State` itself could be lost:
when the file fails to parse, `JsonPrefStore` renames it to `Local State.bad`
and carries on with empty preferences, taking the key with it.

Keeping a copy of known-good keys somewhere else means neither case is
final.

## What is stored

A JSON file, `OSCrypt Key History`, alongside `Local State` in the user data
directory. Up to three records per key provider:

```json
{
  "version": 1,
  "providers": {
    "dpapi": [
      {
        "wrapped_key": "RFBBUEk...",
        "first_seen": "13400000000000",
        "last_verified": "13400003600000"
      }
    ]
  }
}
```

`wrapped_key` is the key exactly as `Local State` holds it: base64 of the
platform-wrapped key. Timestamps are microseconds since the Windows epoch, the
format `base::TimeToValue` produces.

**No plaintext key material is written, and no user data of any kind is
written.** The file holds copies of a value that already exists in
`Local State`, in the same directory, protected by the same platform wrapping.

## Notes for privacy and security review

- **Threat model is unchanged.** Anyone able to read `OSCrypt Key History` can
  already read `Local State` in the same directory, which holds the live copy
  of the same key. The file is created with default inheritance from the user
  data directory, matching `Local State`.
- **Unwrapping still requires the user's DPAPI context.** A copied file is no
  more useful to an attacker on another machine than a copied `Local State`.
- **Retention is bounded but deliberate.** Keeping up to three keys extends the
  window in which a superseded key can be unwrapped by someone with the user's
  DPAPI context. That is the point of the file, and it is the trade being made:
  a longer window of recoverability for the user against a longer window in
  which an old key exists on disk. Three is a judgement call, not a derived
  number.
- **Scope.** The file lives in the user data directory and is removed with it.
  It is not synced, not uploaded, and not read by any other component.
- **No cryptography lives here.** This component stores and orders records. All
  wrapping, unwrapping and comparison is done by the caller.

## The recording rule

A key is only recorded once it has been seen decrypting **real user data**.
That distinction carries the whole design.

Recording a key because it merely unwrapped would be wrong: after a key has
been replaced, the replacement unwraps perfectly well, it just does not match
anything the user saved. Recording on unwrap would overwrite the good record
with the bad key on the first launch after the damage, losing the one thing
worth keeping. It would also mean that anyone who installs a build already in
the broken state gets their broken key enshrined as known good.

Once a key is held, it stays:

- A key already in the history only has its `last_verified` time moved
  forward. The stored bytes are left alone, because re-wrapping the same key
  produces different bytes and rewriting them gains nothing.
- A key that is not in the history becomes the newest record, and the existing
  records are kept. A key that has stopped working is exactly the one that may
  need handing back.
- Only the three newest are kept.

Because wrapping is not deterministic, "is this the same key?" cannot be
answered by comparing stored bytes. The caller supplies a predicate that
unwraps and compares the underlying key material.

## What is not here yet

This component only stores and orders records. Still to come, and tracked
separately:

- The signal that says a key decrypted real user data, and where it is taken
  from. Cookie decryption happens in the network service, out of process, so
  the practical in-process sources are the password store and Web Data.
- Detection: comparing the key in `Local State` against the history at startup
  and noticing that it changed.
- Telling the user, and offering to restore a previous key or start fresh.
