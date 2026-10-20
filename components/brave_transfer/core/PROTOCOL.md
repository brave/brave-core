# Brave Transfer protocol, version 1

Brave Transfer sends files, photos, text and links between a user's own
devices on the same local network. This document is the normative
description of the wire protocol. Three implementations follow it:

- C++ in `components/brave_transfer/core/` (desktop; to be written),
- Swift in `components/brave_transfer/swift/` (`BraveTransferCore`, iOS),
- Python in `tools/brave_transfer/` (reference peer and vector generator).

All three must pass the shared vectors in [`test/data/`](test/data/) (§9).
When this document and the vectors disagree, fix whichever is wrong and
regenerate the vectors with `tools/brave_transfer/gen_vectors.py`.

The key words MUST, MUST NOT, SHOULD and MAY are used as in RFC 2119.

## 1. Overview

```
 iPhone (responder)                                   Mac (initiator)
 ──────────────────                                   ───────────────
 listen on TCP, advertise _brave-xfer._tcp
 TXT v=1 c=<tag>        ────── DNS-SD ──────▶   browse, tag matches → dial
                        ◀──── Noise msg 1 ────  ClientHello{version, tag}
 pick PSK by tag
 PeerInfo (iPhone)      ───── Noise msg 2 ────▶
                        ◀──── Noise msg 3 ────  PeerInfo (Mac), PSK mixed
 Ping{0} (confirmation) ───── transport ──────▶  initiator now trusts the channel
                        ◀───── frames ───────▶  Offer / Response / Chunk / Done ...
```

- The **responder** listens and advertises (the iPhone in M1).
- The **initiator** browses and dials (the desktop in M1).
- Once connected, either side may offer a transfer. Roles only matter for
  the handshake.

## 2. Conventions

- Byte strings in this document and the vectors are lowercase hex.
- `||` is concatenation; `uint64_be(x)` is the 8-byte big-endian encoding.
- `HKDF(salt, ikm, info)` is HKDF-SHA256 (RFC 5869) with a 32-byte output.
  An empty salt means HashLen zero bytes, as the RFC specifies.
- `HMAC` is HMAC-SHA256.
- Times are Unix seconds (UTC).

## 3. Keys and discovery

### 3.1 The chain seed

Every device in a Brave Sync chain stores the same 24-word sync code. The
32-byte **seed** is

```
seed = PassphraseToBytes32(sync_code)
```

where `PassphraseToBytes32` is `brave_sync::crypto::PassphraseToBytes32`
(libwally `bip39_mnemonic_to_bytes`, BIP39 English word list): 24 words
separated by single ASCII spaces, 11 bits per word, the last 8 bits a
SHA-256 checksum of the 32 entropy bytes.

- Only 24-word codes are valid. The 25-word "time-limited" code shown when
  adding a device is not a seed source; use the stored 24-word code
  (`BraveSyncServiceImpl::GetSeed()` on desktop).
- On iOS, `GetOrCreateSyncCode()` MUST NOT be used: it creates a chain when
  there is none. iOS gets `root_key` (§3.2) from BraveCore and never sees the
  words; `BraveTransferCore` therefore starts from the seed or the root key
  and contains no BIP39 code.

Vectors: `key_derivation.json` (`valid`, `invalid_sync_codes`). The invalid
codes are what the reference rejects; libwally rejects the same inputs.

### 3.2 Derived keys

```
root_key      = HKDF(salt = "brave-transfer", ikm = seed,     info = "v1/root")
psk           = HKDF(salt = "",               ikm = root_key, info = "v1/chain-psk")
discovery_key = HKDF(salt = "",               ikm = root_key, info = "v1/discovery")
```

Strings are ASCII without a terminator. `psk` is the Noise pre-shared key
for connections between chain devices (§4). `discovery_key` only produces
tags (§3.3).

### 3.3 Discovery

The responder registers a DNS-SD service:

- type `_brave-xfer._tcp`, domain `local.`;
- instance name: random, fresh for every advertising session, and
  revealing nothing about the device or user (the reference uses 12 hex
  digits);
- TXT record entries `v=1` and `c=<tag>`, where

```
epoch = floor(unix_time / 900)
tag   = HMAC(discovery_key, uint64_be(epoch))[0:8]
c     = lowercase hex of tag (16 characters)
```

The responder re-registers with a new tag when the epoch changes.

An initiator dials an instance only if `v` is `1` and `c` equals the tag of
epoch t−1, t or t+1 for one of its chains, where t is its current epoch.
Other instances are ignored. The same window applies to `trust_tag` in
`ClientHello` (§4.3). Epochs below 0 are skipped.

Later versions add `p=` (pairwise) and `q=` (QR pairing) tags. Initiators
MUST ignore TXT keys they do not know.

Vectors: `discovery_tags.json`.

### 3.4 Static keys and fingerprints

Each install has a long-term X25519 static key pair: per profile on
desktop, and in the shared keychain on iOS. The fingerprint shown to users
is

```
fingerprint = hex(SHA-256(static_public)[0:16]) in groups of 4, separated by spaces
```

for example `8e1f 0c2a ...` (8 groups). Vectors: `noise_handshake.json`
(`*_fingerprint`).

## 4. Connection and handshake

### 4.1 Transport

One TCP connection per session. Every message in both directions,
handshake or transport, is framed as

```
uint16_be(length) || message        (length ≤ 65535)
```

### 4.2 Noise

The handshake is `Noise_XXpsk3_25519_ChaChaPoly_SHA256`, Noise revision 34:

```
-> e
<- e, ee, s, es
-> s, se, psk
```

- Prologue: the 16 ASCII bytes `brave-transfer/1`.
- Because the pattern has a `psk` modifier, every `e` token is followed by
  `MixKey(e.public_key)` in addition to `MixHash`.
- After message 3 both sides call `Split()`. The first cipher state
  encrypts initiator→responder, the second responder→initiator.
- ChaChaPoly nonces are 4 zero bytes followed by the 64-bit counter,
  little-endian. Transport messages use empty associated data.
- An X25519 result of 32 zero bytes (a low-order public key) MUST abort the
  handshake. The Swift and Python implementations check this explicitly
  rather than relying on the crypto library.

Vectors: `noise_cacophony_xxpsk3.json` (the public cacophony vector for the
bare pattern) and `noise_handshake.json` (a full Brave connection with fixed
keys).

### 4.3 Handshake payloads

| Message | Sender | Payload | Encrypted with |
|---|---|---|---|
| 1 | initiator | `ClientHello{protocol_version = 1, trust_tag}` | a key derived only from public values: **not confidential** |
| 2 | responder | responder `PeerInfo` | `ee`/`es` keys (no PSK yet) |
| 3 | initiator | initiator `PeerInfo` | includes `psk` |

- `trust_tag` is the 8-byte tag that made the initiator dial (§3.3). It is
  already public in the TXT record.
- The responder rejects `protocol_version != 1`, then looks up the PSK for
  `trust_tag` among its chains' accepted tags (t−1, t, t+1). With no match
  it closes the connection. Otherwise it sets the PSK and continues.
- Both sides validate the remote `PeerInfo` (§5.4) and close on failure.
- A responder that lacks the PSK fails to decrypt message 3. An initiator
  talking to a responder that lacks the PSK cannot tell until a transport
  message from it decrypts. Therefore:
  - The responder MUST send `Frame{ping: Ping{sequence: 0}}` as its first
    transport message, immediately after reading message 3.
  - The initiator MUST NOT send any transport message until it has
    decrypted one from the responder. Until then the channel is
    *unconfirmed*: the initiator shows nothing about the peer and offers
    nothing.

### 4.4 After the handshake (embedder policy)

The protocol core reports the remote `PeerInfo`, the remote static key and
the handshake hash. The embedder then applies the trust rules from the plan
(§3.3 there): Sync must be active, `sync_device_guid` must be in the local
DeviceInfo list, and the GUID→static-key pin must match. If any check
fails, it closes the connection without sending anything.

## 5. Message encoding

Messages are defined in [`brave_transfer.proto`](brave_transfer.proto)
(proto3). Swift and Python use hand-written codecs; C++ uses
protobuf-lite. They agree on everything in this section.

### 5.1 Wire format rules

Encoders MUST produce the canonical encoding:

- fields in ascending field-number order;
- proto3 defaults (0, empty string, empty bytes, empty repeated) omitted;
- `Response.accepted_item_ids` packed;
- the `Frame` body always present, even when the sub-message is empty
  (`Frame{ping: Ping{}}` is `2a 00`);
- minimal varints.

This is what `SerializeToString` produces with protobuf.

Decoders MUST, like protobuf:

- skip unknown fields of wire types 0, 1, 2 and 5;
- skip known fields that arrive with the wrong wire type;
- let the last occurrence of a non-repeated scalar field win;
- accept `accepted_item_ids` packed, unpacked, or split across runs;
- keep only the low 32 bits of `uint32` and enum varints;
- accept non-minimal varints and 10-byte varints, dropping bits above 64;
- accept unknown enum values (validation rejects them later where needed).

Decoders MUST reject:

- truncated input (varint, key, length or fixed-size field);
- varints longer than 10 bytes;
- field number 0;
- wire types 3, 4, 6 and 7 (groups are not used);
- `string` fields that are not well-formed UTF-8 (including encoded
  surrogates and overlong forms);
- errors inside nested messages (`Offer.items`, `Frame` bodies).

### 5.2 Frames

The plaintext of every transport message is one `Frame`. A `Frame` MUST
have exactly one body field present (field numbers 1–6 with wire type 2).
Zero bodies, or more than one (the same field repeated, or two different
fields), MUST be rejected. **This is stricter than protobuf**, which merges
or keeps the last oneof member. The C++ decoder pre-scans the top-level
tags before `ParseFromString`. Unknown fields alongside the body are
skipped.

Vectors: `proto_messages.json`. `encode` cases go both ways (value ↔
bytes); `decode` cases are non-canonical inputs with their canonical
re-encoding; `invalid` cases must fail, with `protobuf_accepts` marking the
stricter-than-protobuf rules. `tools/brave_transfer/tests/test_proto_conformance.py`
checks all three groups against the protobuf runtime.

### 5.3 Capabilities

`PeerInfo.capabilities` is a bit set. Version 1 defines bit 0
(`CAPABILITY_RECEIVE`, accepts offers) and bit 1 (`CAPABILITY_SEND`, may
offer). Unknown bits MUST be ignored. A peer SHOULD NOT offer to a device
that lacks `CAPABILITY_RECEIVE`.

### 5.4 PeerInfo validation

- `protocol_version` MUST be 1.
- `device_name` and `sync_device_guid` are at most 128 UTF-8 bytes each.
  Display code MUST still treat `device_name` as untrusted text.
- Unknown `device_type` values are allowed and shown as a generic device.

## 6. Framing and sizes

| Limit | Value |
|---|---|
| wire message (length prefix) | ≤ 65535 bytes |
| transport plaintext | ≤ 65519 bytes (65535 − 16-byte tag) |
| control frame plaintext (every body except `Chunk`) | ≤ 16384 bytes |
| `Chunk.data` | 1 … 61440 bytes |

A frame over its limit is a protocol error (§8.9). A full `Chunk` frame
with 61440 data bytes is 61468 bytes of plaintext.

## 7. Filename sanitization

Receivers MUST pass every `OfferItem.name` through this function before
using it as a file name. Senders send the plain base name.

1. Remove these code points: U+0000–U+001F, U+007F–U+009F, U+061C,
   U+200B–U+200F, U+202A–U+202E, U+2060–U+2069, U+FEFF.
2. Replace each of `/ \ < > : " | ? *` with `_`.
3. Strip leading and trailing U+0020 and `.` characters. If nothing is left,
   the result is `file`.
4. Let the stem be the text before the first `.`, with trailing spaces
   removed. If the stem, upper-cased in ASCII only, is one of `CON`, `PRN`,
   `AUX`, `NUL`, `COM1`–`COM9`, `LPT1`–`LPT9`, prefix the name with `_`.
5. If the name is longer than 255 UTF-8 bytes, truncate it:
   - the extension is the text from the last `.` (if it is not the first
     character) to the end; if it is longer than 32 UTF-8 bytes, there is
     no extension;
   - keep whole code points of the remaining stem while stem + extension fit
     in 255 bytes, strip trailing spaces and dots from the kept stem, and
     append the extension.

No Unicode normalization is applied. Receivers still make the final name
unique in the destination directory (the reference appends ` (n)` before
the first `.`). Vectors: `filename_sanitizer.json`.

## 8. Transfers

### 8.1 Model

- A **transfer** is one `Offer` of 1–256 items, identified by a random
  16-byte `transfer_id`. Ids MUST NOT be reused within a connection.
- Each side has at most **one outgoing and one incoming** transfer at a
  time, so two transfers in opposite directions can run at once.
- Item ids are assigned by the sender. The reference numbers items 1, 2, …
  in offer order; receivers only require them to be unique.
- Kinds: `FILE`, `IMAGE` (a file the receiver may save to Photos), `TEXT`,
  `URL`.
- `TEXT` and `URL` items up to 8192 bytes travel in `inline_data` and are
  never chunked. Longer `TEXT` is chunked like a file. `URL` is always
  inline.

### 8.2 Offer validation

A receiver validates every `Offer` and treats any failure as a protocol
error (§8.9):

- `transfer_id` is exactly 16 bytes;
- 1 … 256 items (receivers MAY set a lower limit) with unique `item_id`s;
- `kind` is one of the four defined values;
- `name` is 1 … 1024 UTF-8 bytes; `mime_type` is at most 255 bytes;
- `sha256` is exactly 32 bytes;
- `size` ≤ 2^40 per item, and the sum ≤ 2^40 (receivers MAY set a lower
  total);
- `TEXT` and `URL` items have `size` ≥ 1;
- items that must be inline (all `URL`; `TEXT` with `size` ≤ 8192) have
  `inline_data` with length `size` and SHA-256 `sha256`; other items have
  no `inline_data`;
- inline `TEXT` and `URL` data is well-formed UTF-8;
- a `URL` starts with `http://` or `https://`, compared case-insensitively.

Vectors: `offer_validation.json`.

### 8.3 Response

The receiver answers each offer with exactly one `Response` carrying the
same `transfer_id`:

| `decision` | Meaning | `accepted_item_ids` |
|---|---|---|
| `ACCEPT_ALL` | receive every item | empty |
| `ACCEPT_SOME` | receive the listed items | non-empty, unique, all offered |
| `DECLINE` | user declined, or the offer timed out | empty |
| `BUSY` | another incoming transfer is active | empty |
| `NO_SPACE` | not enough storage | empty |

- When an `Offer` arrives while an incoming transfer is active, the
  receiver MUST reply `BUSY` at once, without telling the user.
- A refusal (`DECLINE`, `BUSY`, `NO_SPACE`) ends the transfer on both sides.
- Any other `decision` value, or ids that break the rules above, is a
  protocol error for the sender.

### 8.4 Chunks

After an accepting `Response`, the sender streams the accepted items that
are neither inline nor empty, in offer order, one item at a time:

- `Chunk{transfer_id, item_id, offset, data}` with `offset` starting at 0
  and increasing by `len(data)`; every chunk carries 1 … 61440 bytes; the
  last chunk of an item ends exactly at `size`;
- a chunk with the wrong item, the wrong offset, an empty `data` or data
  past `size` is a protocol error.

Control frames take priority over chunks, so a `Cancel` or a `Ping` is not
stuck behind queued data. Receivers write chunks as they arrive.

### 8.5 Completion

The receiver hashes every chunked item as it arrives.

- When an item's last chunk arrives and its SHA-256 does not match, the
  receiver sends `Done{HASH_MISMATCH}`, deletes everything it wrote for the
  transfer, and the transfer ends.
- When every accepted item has arrived and verified (immediately after the
  `Response` if all accepted items are inline or empty), the receiver sends
  `Done{OK}`.
- If writing fails, the receiver sends `Done{WRITE_FAILED}` and the
  transfer ends.
- `Done{OK}` before the sender has sent every chunk is a protocol error.
  Other `Done` statuses may arrive at any time after the `Response` and end
  the transfer.

### 8.6 Cancel

Either side may send `Cancel{transfer_id, reason}` at any time after the
`Offer`: `USER`, `TIMEOUT` or `ERROR`. The transfer ends on both sides; the
receiver deletes partial files. Frames that arrive afterwards for an ended
transfer (chunks in flight, a late `Response` or `Done`, a duplicate
`Cancel`) MUST be ignored. A `Cancel` for an id that was never seen is also
ignored.

### 8.7 Pings and timers

| Timer | Value | Action |
|---|---|---|
| offer timeout (sender) | 60 s without a `Response` | send `Cancel{TIMEOUT}` |
| offer timeout (receiver) | 60 s without a user decision | send `Response{DECLINE}` |
| ping interval | 10 s since the last frame sent | send `Ping{sequence}`, sequence 1, 2, … |
| idle timeout | 30 s since the last frame received | close the connection |

`Ping{0}` is reserved for the responder's confirmation (§4.3). Pings need
no reply; the other side's own pings keep the connection alive.

### 8.8 Ending a connection

A side that has nothing left to send (no active transfer, empty queue) MAY
close the connection, or half-close its write side and read until the peer
does the same. The reference peer and `BraveTransferPeer` half-close once
their scripted work is done. Closing during a transfer ends it as
`INTERRUPTED` locally; partial files are deleted.

### 8.9 Errors

Anything this document calls a protocol error, and any decryption or
decoding failure after the handshake, closes the connection immediately
with no further frames. Active transfers end as interrupted and partial
files are deleted. There is no error frame.

## 9. Test vectors

All files are in [`test/data/`](test/data/) and written by
`tools/brave_transfer/gen_vectors.py`. Run it with `--check` to confirm they
are current.

| File | Covers |
|---|---|
| `key_derivation.json` | §3.1–3.2: sync code → seed → root, psk, discovery keys; invalid codes |
| `discovery_tags.json` | §3.3: epochs, tags, accepted windows, TXT entries |
| `noise_cacophony_xxpsk3.json` | §4.2: the bare Noise pattern (cacophony) |
| `noise_handshake.json` | §4: a full Brave connection: keys, payloads, messages, handshake hash, split keys, three transport messages with length prefix, fingerprints |
| `proto_messages.json` | §5: canonical encodings, tolerant decoding, rejected input |
| `filename_sanitizer.json` | §7 |
| `offer_validation.json` | §8.2 |
| `session_transcript.json` | §8: frame-by-frame transcripts for accept-all, accept-some, inline-only and decline |

## 10. Versioning

`protocol_version` appears in `ClientHello` and `PeerInfo`, the prologue
(`brave-transfer/1`) and the TXT `v=` entry. An incompatible change
increments all of them; a version 1 peer refuses anything else. Compatible
additions (new fields, enum values, capability bits or TXT keys) keep
version 1 and rely on the skipping rules in §5.1 and §3.3.
