# Webcat Component

Native implementation of [webcat](https://github.com/freedomofpress/webcat) (Web-based Code Assurance and Transparency) in the Brave browser, providing blocking code-signing verification and integrity checks for web applications served over decentralized TLDs (`.eth`, `.brave`).

## What Webcat Does

Webcat ensures that the code delivered to a user's browser matches a cryptographically signed manifest before it executes. When a user navigates to a decentralized domain (e.g. `app.eth`), webcat:

1. Looks up the domain's webcat TXT record on-chain (ENS `text` record or UD `dweb.webcat` record) to find an IPFS CID
2. Fetches the webcat bundle from IPFS (HTTP gateway with CID verification)
3. Verifies the bundle's signatures and validates the manifest's CSP policies
4. Intercepts all subsequent resource loads for that origin, SHA-256 hashing each against the manifest
5. Injects the manifest's CSP headers (replacing server-sent headers) and shows a verified badge

If verification fails at any step, an interstitial blocks the page with the option to proceed anyway.

## Directory Structure

```
components/webcat/
├── core/                          # Browser-agnostic verification logic
│   ├── bundle_parser.cc/h         # Parse and validate webcat bundle JSON
│   ├── csp_validator.cc/h         # Validate CSP policies per webcat spec
│   ├── manifest_verifier.cc/h     # CID-based content hash verification
│   ├── origin_state.cc/h          # Per-origin state machine
│   ├── webcat_cache.cc/h          # LRU cache for verified manifests
│   ├── webcat_resolver.h          # Interface: domain -> optional<CID>
│   ├── ipfs_bundle_fetcher.cc/h   # HTTP gateway fetch with CID verification
│   ├── constants.h                # Enums, error codes, defaults
│   └── test/                      # Unit tests
├── content/                       # Content-layer integration
│   ├── webcat_body_handler.cc/h   # BodySniffer BodyHandler: hash checking + CSP injection
│   ├── webcat_tab_helper.cc/h     # Per-tab origin state and badge tracking
│   └── test/                      # Unit tests
└── browser/ (in brave/browser/webcat/)
    ├── ens_webcat_resolver.cc/h   # ENS text("webcat") resolver
    └── ud_webcat_resolver.cc/h    # UD dweb.webcat record resolver
```

## Design Choices vs. the Webcat Extension

The original webcat is a [Firefox extension](https://github.com/freedomofpress/webcat). This native implementation makes several fundamentally different architectural choices:

### Enrollment Discovery

| | Extension | Native |
|---|---|---|
| **Source** | CometBFT (Tendermint) light client fetching a signed list from `webcat.freedom.press` | ENS `text` record / UD `dweb.webcat` record containing a single IPFS CID |
| **Trust root** | Hardcoded validator set (3 Ed25519 validators) | Blockchain naming system itself (Ethereum for ENS, Polygon/Base/Ethereum for UD) |
| **Update model** | Periodic list sync (every 5 min) via Merkle proofs | On-demand lookup per navigation; CID changes are detected via cache invalidation |

**Rationale**: The CometBFT enrollment chain is a centralized dependency. ENS/UD records are already being resolved for decentralized DNS navigation, so reusing them eliminates an entire trust boundary and infrastructure dependency. A CID is a cryptographic commitment to bundle content, so one lookup + one fetch is sufficient.

### IPFS Bundle Fetching

| | Extension | Native |
|---|---|---|
| **Transport** | HTTPS fetch to `/.well-known/webcat/bundle.json` on the origin server | IPFS gateway at `<CID>.ipfs.inbrowser.link` (subdomain-based) |
| **Integrity** | Enrollment hash comparison against locally-stored hash | CID verification (SHA-256 of content must match CID) |
| **Decentralization** | Depends on the origin server being available and not compromised | Bundle is content-addressed and fetchable from any IPFS node |

**Rationale**: In the extension model, the server serves its own verification bundle, which creates a circular trust problem if the server is compromised. IPFS content addressing means the CID itself guarantees bundle integrity regardless of which gateway or peer serves it. The subdomain gateway (`<CID>.ipfs.inbrowser.link`) gives each bundle its own origin, preventing cross-site attacks.

**P2P IPFS** (via Helia in a service worker) is planned as a primary transport with HTTP gateway as fallback, but the initial implementation is HTTP-only. The fetcher is designed so P2P can be added as a higher-priority transport without changing the verification pipeline.

### Interception Layer

| | Extension | Native |
|---|---|---|
| **Mechanism** | `browser.webRequest` API + `browser.webRequest.filterResponseData` | `BodySnifferThrottle` / `BodyHandler` in the browser process |
| **Scope** | Can be bypassed by other extensions or page script | Cannot be bypassed; runs before renderer receives data |
| **CSP enforcement** | Compares server CSP header against manifest CSP (exact string match) | Injects manifest CSP, replacing server headers entirely |

**Rationale**: Extension APIs are inherently limited. `webRequest` can be bypassed and `filterResponseData` only works in Firefox. The browser-process `BodyHandler` intercepts at the mojo IPC layer, which is the deepest point before data reaches the renderer. This also means we can inject CSP authoritatively rather than just comparing against it.

### WASM Verification

| | Extension | Native |
|---|---|---|
| **Mechanism** | Content script hooks all `WebAssembly.*` methods in the page, verifies bytecode hashes synchronously | Fetch-level SHA-256 hash checking only (no V8 hooks) |

**Rationale**: The extension's WASM hooks exist because the extension runs in a sandbox and cannot intercept network responses for `.wasm` files loaded via `WebAssembly.instantiateStreaming`. At the browser-process level, all fetched resources (including `.wasm`) pass through the `BodyHandler` before reaching the renderer, making V8-level hooks unnecessary. If a `.wasm` file is listed in the manifest's `files` map, it is hash-checked like any other resource.

### Signature Verification

| | Extension | Native (current) |
|---|---|---|
| **Sigsum** | Full: Ed25519 signature verification, cosigned tree head validation, witness quorum evaluation, timestamp freshness | Not yet implemented; pluggable via `manifest_verifier.h` |
| **Sigstore** | Full: via `@freedomofpress/sigstore-browser`, certificate claim validation, OID matching | Not yet implemented; pluggable via `manifest_verifier.h` |
| **CID verification** | Not applicable (enrollment hash instead) | SHA-256 hash of bundle content verified against CID |

**Rationale**: Since the bundle is content-addressed via IPFS CID, the CID itself provides transport integrity. Signatures add *authorization proof* (proving trusted signers approved the content), which is defense-in-depth. The current implementation relies on CID verification as the primary mechanism with signature verification designed as a pluggable extension point. Full Sigsum/Sigstore verification will be added in C++ using BoringSSL (Ed25519) and Chromium's certificate infrastructure.

### Domain Scope

| | Extension | Native |
|---|---|---|
| **Domains** | Any enrolled domain (via the CometBFT list) | Decentralized TLDs only (`.eth`, `.brave`) |
| **Rationale** | Extension can intercept any domain via `webRequest` | Zero overhead for traditional domains; trust model is coherent (blockchain for naming + blockchain for verification metadata) |
| **`.sol`** | Not supported | Deferred; pluggable `WebcatResolver` interface supports future addition |

## Bundle Format

Reuses the existing webcat bundle format for interoperability:

```json
{
  "manifest": {
    "app": "https://app.eth",
    "version": "1.0.0",
    "timestamp": "<sigsum tree head>",
    "default_csp": "default-src 'self'; script-src 'self'; ...",
    "files": {
      "/index.html": "<base64url-sha256>",
      "/app.js": "<base64url-sha256>",
      "/style.css": "<base64url-sha256>"
    },
    "default_index": "/index.html",
    "default_fallback": "/index.html",
    "wasm": ["<base64url-sha256-of-wasm-binary>"],
    "extra_csp": {
      "/admin/": "default-src 'self'; script-src 'none'"
    }
  },
  "signatures": {
    "<base64url-ed25519-pubkey>": "<sigsum-proof-with-escaped-newlines>"
  }
}
```

## Enrollment Record Format

**ENS TXT record**:
- Key: `webcat`
- Value: IPFS CID (e.g., `bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3oclgtqy55fbzdi`)

**Unstoppable Domains record**:
- Key: `dweb.webcat`
- Value: IPFS CID (same format)

## Verification Pipeline

Verification proceeds in strict order. Failure at any step aborts and shows the interstitial.

1. **CID integrity** -- For HTTP gateway responses, explicitly SHA-256 hash the response body and verify against CID. (IPFS P2P fetches do this automatically.)
2. **Signature verification** -- Parse `signatures` object. For each signer, verify via Sigsum or Sigstore path. Confirm valid count meets `threshold`. (Not yet implemented; CID verification is the current gate.)
3. **Manifest structure validation** -- Required fields present, CSP policies conform to webcat spec.
4. **Cache and activate** -- Store verified manifest in per-origin LRU cache. Install `BodyHandler`. Origin is now active.

## What's Not In Scope

- V8-level WASM bytecode hooks (fetch-level hash checking is sufficient at the browser-process level)
- Settings page UI (no user-managed enrollment, no configuration)
- `.sol` / SNS support (deferred; pluggable resolver interface supports future addition)
- Non-decentralized TLD support
- Sigsum-only sites served over traditional HTTPS (future interop preserved by bundle format choice)

## References

- [webcat repository](https://github.com/freedomofpress/webcat)
- [webcat spec](https://github.com/freedomofpress/webcat-spec)
- Design plan: `docs/plans/2026-04-21-native-webcat-design.md`
- Brave decentralized DNS: `components/decentralized_dns/`
- Brave wallet ENS resolver: `components/brave_wallet/browser/ens_resolver_task.cc`
- Brave wallet UD resolver: `components/brave_wallet/browser/unstoppable_domains_dns_resolve.cc`
- Network delegate helper: `browser/net/decentralized_dns_network_delegate_helper.cc`
