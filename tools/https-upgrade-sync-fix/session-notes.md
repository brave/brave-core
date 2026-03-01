# HTTPS Upgrade Sync Overwrite Fix - Session Notes

## Issue
brave-browser#31940: HTTPS Upgrade setting overwritten by Sync.

`MigrateHttpsUpgradeSettings()` in `browser/profiles/brave_profile_manager.cc`
runs on every startup and checks `prefs::kHttpsOnlyModeEnabled`. This is a
synced pref. When another device has it set to `true`, the synced value
overwrites the local HTTPS upgrade content setting to Strict on every restart.

## Fix (implemented, not yet built)

Guard the forward migration with a one-time boolean pref
`kBraveHttpsUpgradeMigrationDone` so it only runs once per profile.

### Files modified

1. `components/constants/pref_names.h` - Added `kBraveHttpsUpgradeMigrationDone`
2. `browser/brave_profile_prefs.cc` - Registered the pref (default: false)
3. `browser/profiles/brave_profile_manager.cc` - Added guard to migration:
   - Forward path: early-return if migration already done; set flag after migrating
   - Backward path: reset flag to false so re-enabling feature re-migrates
4. `browser/profiles/brave_profile_manager_unittest.cc` - Added 4 tests:
   - `MigrateHttpsUpgradeSettings_FirstRun`
   - `MigrateHttpsUpgradeSettings_SyncDoesNotOverwrite`
   - `MigrateHttpsUpgradeSettings_NoMigrationNeeded`
   - `MigrateHttpsUpgradeSettings_BackwardMigration`

## Manual repro attempt - in progress

### Key findings

- The pref key is `"https_only_mode_enabled"` (top-level in Preferences JSON),
  defined in `chrome/common/pref_names.h:81`.
- The HTTPS upgrade content setting is stored under
  `profile.content_settings.exceptions.httpsUpgrades` (key name `"httpsUpgrades"`),
  registered via `ContentSettingsType::BRAVE_HTTPS_UPGRADE`.
- `kBraveHttpsByDefault` feature is enabled by default
  (`chromium_src/net/base/features.cc:44`).
- XYZZY profile = `Default` directory under
  `~/Library/Application Support/BraveSoftware/Brave-Browser-Development/`.

### Problem with manual repro

Directly editing the `Preferences` JSON file to set `https_only_mode_enabled = true`
did not trigger the migration on restart. Two issues found:

1. **JSON formatting**: `json.dump()` without `indent` compressed the 50KB
   Preferences file into a single line. Fixed by using `indent=3`.
2. **super_mac validation**: Chrome/Brave uses a `super_mac` HMAC in
   `Secure Preferences` to detect external tampering of the `Preferences` file.
   When the file is modified externally, the super_mac no longer matches and
   Chrome resets the tampered prefs to defaults on startup. Fix: delete the
   `protection.super_mac` key from `Secure Preferences` before restarting.

The `simulate_sync.py` script now handles both of these issues. Still waiting
to confirm the repro works.

### Manual test steps

1. Quit Brave Development (Cmd+Q, verify no processes via `pgrep`)
2. Run: `python3 tools/https-interstitial-check/simulate_sync.py`
3. Relaunch Brave Development
4. Check `brave://settings/shields`
   - Before fix: should show "Strict" (bug confirmed)
   - After fix: should show "Standard" (fix confirmed)

## Build / test commands

- Build: `npm run build` (from `src/brave`)
- Unit tests: `npm run test -- brave_all_unit_tests --filter=BraveProfileManager*HttpsUpgrade*`

## TODO

- [ ] Confirm manual repro (super_mac fix may resolve the issue)
- [ ] Build with fix
- [ ] Run unit tests
- [ ] Manual test with fix applied
- [ ] Presubmit check (PRESUBMIT.md)
