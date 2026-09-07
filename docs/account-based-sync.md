# Account-Based Cross-Device Sync (Brave)

This documents the client-side integration that lets a user sign into a **Brave
Account** and have their devices **automatically join the same sync chain**
without exchanging a QR code or 25-word passphrase — while keeping the existing
QR/passphrase chain fully intact as a privacy-preserving alternative.

## How it works

1. The user signs into their Brave Account from the account dialog
   (`components/brave_account/resources/brave_account_sign_in_dialog.ts`).
2. The renderer already runs OPAQUE locally and holds the password, so it
   deterministically derives a **32-byte sync seed** from the account
   credentials via PBKDF2 (`derive_account_seed.ts`).
3. The seed (64 hex chars) is sent to C++ over the existing `Authentication`
   Mojo interface: `EstablishAccountSync(seedHex)`.
4. `BraveAccountService` forwards this to
   `syncer::BraveSyncServiceImpl::EstablishAccountChain(seedHex, email)`, which
   converts the seed into the normal sync passphrase, stores it (OSCrypt),
   derives the signing keys in `BraveSyncAuthManager`, and marks setup complete.
5. Because every device signed into the same account derives the **identical**
   seed, they all join the **same chain automatically** — no manual code
   exchange. The sync server only ever stores encrypted blobs and the OPAQUE
   verifier; it never sees the password or the seed (end-to-end encryption).
6. On sign-out, `StopAccountSync(keepLocalData)` leaves the chain; the user is
   offered the choice to keep or wipe the local synced data.

## Key files

- `components/brave_account/features.{h,cc}` — `kBraveAccountSync` feature flag
  (`IsBraveAccountSyncEnabled()`).
- `components/brave_account/mojom/brave_account.mojom` — `EstablishAccountSync`,
  `StopAccountSync` on the `Authentication` interface.
- `components/brave_account/state_base.{h,cc}` — forwards the new methods to the
  service (mirrors the `AddObserver` pattern).
- `components/brave_account/brave_account_service.{h,cc}` — implements the
  methods, driving `BraveSyncServiceImpl` via an injected sync-service getter.
- `browser/brave_account/brave_account_service_factory.cc` — supplies the
  sync-service callback (keeps the component free of `chrome/browser` deps).
- `components/brave_sync/brave_sync_prefs.{h,cc}` — `account_sync_email` /
  `account_sync_enabled` prefs.
- `components/sync/service/brave_sync_service_impl.{h,cc}` —
  `EstablishAccountChain()` / `StopAccountChain()`.
- `browser/ui/webui/settings/brave_sync_handler.{h,cc}` — `SyncEstablishAccountSync`
  / `SyncStopAccountSync` / `SyncGetAccountSyncState` message handlers (and
  `brave_sync_browser_proxy.ts`).
- `components/brave_account/resources/derive_account_seed.ts` — client-side seed
  derivation (PBKDF2-SHA256, 100k iterations).
- `components/brave_account/resources/brave_account_sign_in_dialog.ts` — derives
  the seed and calls `establishAccountSync` after login.
- `browser/resources/settings/getting_started_page/brave_account_logged_in_row.ts`
  — sign-out offers keep/wipe via `stopAccountSync`.

## Enabling

1. Build with Brave Account on: `--enable-features=BraveAccount`.
2. Enable the sync integration: `--enable-features=BraveAccountSync`
   (requires `BraveAccount`).
3. The flag is also exposed as the `braveAccountSyncEnabled` load-time datum on
   both the settings and account WebUIs.

## Backend contract (must be provided by account servers)

- `login_init` must return a **public per-account salt**; `derive_account_seed.ts`
  should use it instead of the fixed application salt (the salt parameter is
  already plumbed through `deriveAccountSyncSeed(email, password, accountSalt)`).
- The `Authentication.GetServiceToken(Service::kSync)` scope must be usable to
  authenticate the sync client to `BRAVE_SYNC_ENDPOINT` (authorization token,
  separate from the E2E seed).
- The sync server must NOT store the password or seed; it stores only the OPAQUE
  verifier and encrypted sync data.

## Known limitations / follow-ups

- "Wipe local synced data" on sign-out currently leaves the local copies
  (consistent with `StopAndClear`); full local-data deletion is a follow-up.
- Seed derivation currently uses a fixed application salt; wire the server
  `account_salt` (parameter already present).
- Requires a full Chromium build to compile/verify (mojom + TS bindings are
  generated at build time). Not build-verified in this change set.
- Android/iOS reach: `BraveSyncServiceImpl` is cross-platform, but the account
  dialog/UI wiring here targets desktop; mobile entry points need the analogous
  `establishAccountSync` call from their login flows.
