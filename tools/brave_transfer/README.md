# Brave Transfer reference peer

A small Python implementation of the Brave Transfer protocol
([`components/brave_transfer/core/PROTOCOL.md`](../../components/brave_transfer/core/PROTOCOL.md)).
It does three jobs:

- generates the shared test vectors in
  `components/brave_transfer/core/test/data/` (`gen_vectors.py`);
- acts as "the other device" for the C++ and Swift implementations, over
  TCP, mDNS or another program's stdio;
- runs interop tests against the Swift `BraveTransferPeer`.

It is a development tool and is not shipped.

## Setup

Python 3.10+.

```sh
pip install -r tools/brave_transfer/requirements.txt
```

## Tests

```sh
cd tools/brave_transfer
python3 -m pytest tests
```

`tests/test_swift_interop.py` builds `components/brave_transfer/swift` and
runs both roles against it. It is skipped when `swift` is not on `PATH`;
set `BRAVE_TRANSFER_REQUIRE_SWIFT=1` to make that a failure, or `SWIFT` to
choose the binary.

## Test vectors

```sh
python3 tools/brave_transfer/gen_vectors.py          # rewrite
python3 tools/brave_transfer/gen_vectors.py --check  # verify, for CI
```

`noise_cacophony_xxpsk3.json` is only rewritten when `--cacophony` points
at a copy of
[cacophony.txt](https://github.com/mcginty/snow/blob/main/tests/vectors/cacophony.txt).

## Running a peer

Every device needs the chain's sync code (`--sync-code "<24 words>"`) or
its 32-byte seed (`--seed <hex>`). `new-code` prints a fresh one.

```sh
cd tools/brave_transfer
CODE=$(python3 -m brave_transfer_ref new-code)

# Terminal 1: an iPhone-like responder that advertises over mDNS and
# accepts one transfer into ./inbox.
python3 -m brave_transfer_ref run --role responder --sync-code "$CODE" \
    --advertise --expect-offers 1 --receive-dir inbox

# Terminal 2: a desktop-like initiator that finds it and sends a file.
python3 -m brave_transfer_ref run --role initiator --sync-code "$CODE" \
    --discover --send-file ~/Pictures/photo.jpg --send-url https://brave.com
```

- `--listen PORT` / `--connect HOST:PORT` skip mDNS.
- `--exec CMD ...` runs another implementation and talks to it over its
  stdin/stdout. It must be the last option, for example
  `--exec components/brave_transfer/swift/.build/debug/BraveTransferPeer --role responder --seed <hex>`.
- `--accept all|first|decline`, `--allow-guid`, `--report out.json`: see
  `--help`.
- `tags --sync-code ...` prints the discovery tags around now, to compare
  with what a device advertises (`dns-sd -B _brave-xfer._tcp` on macOS).

## Layout

| Path | Contents |
|---|---|
| `brave_transfer_ref/bip39.py` | sync code ↔ seed (`PassphraseToBytes32`) |
| `brave_transfer_ref/bip39_english.txt` | the BIP39 English word list, copied unmodified from [bitcoin/bips](https://github.com/bitcoin/bips/blob/master/bip-0039/english.txt) (the same list libwally uses); `tests/test_vectors.py` pins its SHA-256 |
| `brave_transfer_ref/keys.py` | HKDF chain keys, discovery tags, fingerprints |
| `brave_transfer_ref/noise.py` | `Noise_XXpsk3_25519_ChaChaPoly_SHA256` |
| `brave_transfer_ref/proto.py` | hand-written codec for `brave_transfer.proto` |
| `brave_transfer_ref/channel.py` | length-prefix framing, handshakes, `SecureChannel` |
| `brave_transfer_ref/session.py` | sans-IO transfer state machine |
| `brave_transfer_ref/sanitize.py` | receiver file name sanitizer |
| `brave_transfer_ref/peer.py` | asyncio runner used by the CLI and tests |
| `brave_transfer_ref/discovery.py` | DNS-SD advertise and browse (`zeroconf`) |
| `gen_vectors.py` | writes `components/brave_transfer/core/test/data/*.json` |
