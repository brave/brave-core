# Brave Origin -- Test Plan

## Overview

This test plan covers both Brave Origin distribution modes:

1. **Branded Build** -- Separate product with features compiled out
2. **Upgrade Mode** -- Existing Brave with policy-based feature disabling

Each section references the relevant issues and PRs.

---

## 1. Branded Build -- Build and Packaging

**Related PRs:** brave/brave-core#33045, #33184, #33261, #33279, #34939, #34998,
#35081, #35157

### 1.1 Build Flag Validation

- [ ] Verify `is_brave_origin_branded` GN flag produces a successful build on
      macOS, Windows, and Linux
- [ ] Verify the build fails if `is_brave_origin_branded` is enabled on Android
      or iOS (brave/brave-core#33846)
- [ ] Verify that all disabled build flags are false: `enable_brave_rewards`,
      `enable_ai_chat`, `enable_brave_news`, `enable_brave_talk`, `enable_tor`,
      `enable_brave_vpn`, `enable_brave_wallet`, `enable_brave_wayback_machine`,
      `enable_speedreader`

### 1.2 Windows Installer

- [ ] Verify unique GUIDs are used for Brave Origin (brave/brave-core#33184)
- [ ] Verify installer names contain "BraveOrigin"
- [ ] Verify update paths are separate from standard Brave
- [ ] Verify RC brand generation produces correct resources
      (brave/brave-core#33279)
- [ ] Verify correct Brave Origin icons are used in the exe
      (brave/brave-core#35081, #35157)
- [ ] Verify product names, shortcuts, Start Menu folders, firewall rules, and
      accessible window titles show "Brave Origin" (brave/brave-core#34870)

### 1.3 macOS Installer

- [ ] Verify DMG has correct DS_Store layout (brave/brave-core#34710)
- [ ] Verify DMG uses Brave Origin background image (brave/brave-core#34740)
- [ ] Verify icon sizing is correct in macOS tray (brave/brave-core#33031)
- [ ] Verify Mac helper names show "Brave Origin" (brave/brave-core#34870)

### 1.4 Linux Packages

- [ ] Verify DEB package is named `brave-origin-nightly-*`
      (brave/brave-core#34939)
- [ ] Verify RPM package is named correctly
- [ ] Verify dist ZIP is named `brave-origin-v*`
- [ ] Verify symbols package is named correctly
- [ ] Verify appdata.xml and chromium-browser.info have correct Origin
      descriptions (brave/brave-core#34998)

---

## 2. Branded Build -- Branding and Theme

**Related PRs:** brave/brave-core#33045, #33186, #33239

### 2.1 Visual Branding

- [ ] Verify `brave://settings` footer shows Origin branding
      (brave/brave-core#33239)
- [ ] Verify `brave://settings/help` shows Origin branding
- [ ] Verify `chrome://theme/current-channel-logo` shows Origin logo
- [ ] Verify URL bar icon for `brave://` pages shows Origin icon
      (brave/brave-core#33186)
- [ ] Verify branded strings show "Brave Origin" not "Brave Nightly" on Windows
      (brave/brave-core#34870)

### 2.2 Strings and Translations

- [ ] Verify XTB files are shared with standard Brave (brave/brave-core#33261)
- [ ] Verify GRD branding files are Origin-specific (brave/brave-core#33045)
- [ ] Verify Windows `generate_embedded_i18n` works with Origin XTB paths
      (brave/brave-core#33261)

---

## 3. Branded Build -- Startup Dialog

**Related PRs:** brave/brave-core#34553, #34554, #34555, #34699, #34721, #34739

### 3.1 Dialog Display

- [ ] Verify the startup dialog appears before any browser window on first
      launch (brave/brave-core#34555)
- [ ] Verify no browser windows are accessible while the dialog is shown
- [ ] Verify the Profile menu is removed on macOS while the dialog is shown
      (brave/brave-core#34699)
- [ ] Verify Mac app commands are blocked while the dialog is showing
      (brave/brave-core#34555)
- [ ] Verify dialog is scrollable for long translations
      (brave/brave-browser#53790)

### 3.2 Purchase Flow

- [ ] Verify the dialog allows purchasing via SKU validation
- [ ] Verify successful purchase dismisses the dialog and opens the browser
- [ ] Verify invalid/expired SKU is rejected
- [ ] Verify the buy window navigation works correctly (brave/brave-core#34554)

### 3.3 Linux Free Tier

- [ ] Verify on Linux the dialog shows a free tier option
      (brave/brave-core#34721)
- [ ] Verify clicking the free tier option proceeds to the browser without
      purchase
- [ ] Verify macOS and Windows do NOT show the free tier option

### 3.4 Styling

- [ ] Verify dialog uses Nala theming via user profile (brave/brave-core#34739)
- [ ] Verify dialog matches Figma designs

### 3.5 Bypass Prevention

- [ ] Verify there is no way to bypass the startup dialog and use the browser
      without validation (brave/brave-browser#53551, #53780)
- [ ] Verify Profile menu bypass is blocked on macOS (brave/brave-core#34699)

---

## 4. Branded Build -- Feature Compilation

**Related PRs:** brave/brave-core#33309, #34612

### 4.1 Compiled-Out Features

- [ ] Verify Brave Rewards UI and functionality is absent
- [ ] Verify Leo AI Chat UI and functionality is absent
- [ ] Verify Brave News UI and functionality is absent
- [ ] Verify Brave Talk UI and functionality is absent
- [ ] Verify Tor UI and functionality is absent
- [ ] Verify Brave VPN UI and functionality is absent
- [ ] Verify Brave Wallet UI and functionality is absent
- [ ] Verify Wayback Machine UI and functionality is absent
- [ ] Verify Speedreader UI and functionality is absent
- [ ] Verify Brave Ads code is compiled out with buildflag static asserts
      (brave/brave-core#33309)

### 4.2 Default-Off Features

- [ ] Verify P3A is off by default and the P3A infobar does not appear
      (brave/brave-core#34612)
- [ ] Verify crash reporting is off by default (brave/brave-browser#54131)
- [ ] Verify sidebar is off by default (brave/brave-browser#54130)
- [ ] Verify Playlist is excluded (brave/brave-browser#54129)
- [ ] Verify Web Discovery is disabled
- [ ] Verify statistics reporting is off

### 4.3 Audit Verification

- [ ] Verify no network requests are made for disabled features
      (brave/brave-browser#47557)
- [ ] Verify no background processes/services run for disabled features
      (brave/brave-browser#47474)
- [ ] Run sizeof.cat-style telemetry analysis on branded build

---

## 5. Branded Build -- Onboarding

**Related PR:** brave/brave-core#33257

- [ ] Verify WDP step is removed from onboarding
- [ ] Verify telemetry checkbox is removed from onboarding
- [ ] Verify last step title is changed appropriately
- [ ] Verify crash reporting defaults to on during onboarding

---

## 6. Branded Build -- Browser Import

**Related PR:** brave/brave-core#34650

- [ ] Verify Brave appears as an import source in Brave Origin
- [ ] Verify bookmarks can be imported from standard Brave
- [ ] Verify history can be imported from standard Brave
- [ ] Verify passwords can be imported from standard Brave
- [ ] Verify settings/preferences can be imported from standard Brave
- [ ] Verify import works when standard Brave is not installed (graceful
      handling)

---

## 7. Upgrade Mode -- Core Policy System

**Related PRs:** brave/brave-core#31242, #31249, #31404, #33258, #33277, #33322

### 7.1 BraveOriginService

- [ ] Verify BraveOriginService is created per profile
      (brave/brave-browser#49214)
- [ ] Verify service correctly manages policy values from local state
      (brave/brave-browser#49343)
- [ ] Verify policies are stored in local state scoped by Profile ID
      (brave/brave-browser#49285, #49299)
- [ ] Verify no memory leaks in BraveOriginService (brave/brave-browser#52757)

### 7.2 Policy Provider

- [ ] Verify custom policy source type works with Chromium's policy system
      (brave/brave-browser#49042)
- [ ] Verify browser-level policy provider works (brave/brave-core#30984)
- [ ] Verify profile-level policy provider works (brave/brave-core#30996)
- [ ] Verify "Managed By" UI is hidden for Origin-only policies
      (brave/brave-browser#49679)
- [ ] Verify policies are un-gated in official builds
      (brave/brave-browser#49212)

### 7.3 Mojo Interface

- [ ] Verify Mojo interface communicates correctly between frontend and backend
      (brave/brave-core#31404)
- [ ] Verify end-to-end flow works (brave/brave-core#31249)

---

## 8. Upgrade Mode -- Settings UI (Desktop)

**Related PRs:** brave/brave-core#31677, #33321, #33325, #33838, #35173

### 8.1 Settings Page

- [ ] Verify `brave://settings/origin` page is accessible when feature flag is
      enabled and purchased
- [ ] Verify settings page is NOT shown when feature flag is off
- [ ] Verify settings page is NOT shown when not purchased (and not a branded
      build)
- [ ] Verify each feature has a toggle control
- [ ] Verify toggling a feature off disables it via policy
- [ ] Verify toggling a feature back on re-enables it

### 8.2 Reset to Defaults

- [ ] Verify "Reset to defaults" button exists (brave/brave-browser#47977)
- [ ] Verify clicking reset disables all optional features
- [ ] Verify reset shows confirmation/relaunch notification

### 8.3 Smart Defaults

- [ ] Verify features the user is actively using (e.g., paid Leo subscription)
      are not disabled by default (brave/brave-browser#48145)

### 8.4 Relaunch Notification

- [ ] Verify relaunch notification overlay appears when policies change
      (brave/brave-browser#48142)
- [ ] Verify relaunching applies the policy changes

### 8.5 Purchase in System Settings

- [ ] Verify purchase option appears in System settings at the bottom
      (brave/brave-core#33325)
- [ ] Verify purchase option is NOT shown in branded builds
      (brave/brave-core#35173)
- [ ] Verify purchase URL uses `brave_domains::GetServicesDomain`
      (brave/brave-core#33838)
- [ ] Verify sidebar refreshes correctly after purchase (brave/brave-core#35173)
- [ ] Verify no crashes during purchase flow (brave/brave-core#35173)

---

## 9. Upgrade Mode -- Purchase and Subscription (Desktop)

**Related PRs:** brave/brave-core#34044

### 9.1 Purchase State

- [ ] Verify purchase state is tracked correctly for the upgrade case
      (brave/brave-core#34044)
- [ ] Verify successful purchase enables the Origin settings page
- [ ] Verify purchase state persists across browser restarts

### 9.2 SKU Validation

- [ ] Verify SKU token validation works via account.brave.com
      (brave/brave-browser#47463)
- [ ] Verify expired or invalid tokens are rejected
- [ ] Verify free tier works on Linux

---

## 10. Android -- Settings and Policies

**Related PRs:** brave/brave-core#34411, #34985

### 10.1 Settings Screen

- [ ] Verify Brave Origin settings screen is accessible
      (brave/brave-browser#50270)
- [ ] Verify icons use Leo design tokens (brave/brave-core#34411)
- [ ] Verify all feature toggles are present and functional

### 10.2 Policy Integration

- [ ] Verify policies are applied correctly on Android
      (brave/brave-browser#51333, #51412)
- [ ] Verify per-feature policy-based UI updates:
  - [ ] Rewards (brave/brave-browser#51826)
  - [ ] AI Chat (brave/brave-browser#51974)
  - [ ] News (brave/brave-browser#52170)
  - [ ] VPN (brave/brave-browser#52355)
  - [ ] Wallet (brave/brave-browser#52496)
- [ ] Verify restart dialog appears when needed (brave/brave-browser#52774)
- [ ] Verify policy-disabled features are removed from settings search index
      (brave/brave-core#34985)

---

## 11. Android -- Purchase Flow

**Related PRs:** brave/brave-core#34719, #34955

### 11.1 Google Play Billing

- [ ] Verify Google Play Store billing client integration works
      (brave/brave-browser#53006, #52855)
- [ ] Verify one-time payment flow completes successfully
- [ ] Verify purchase receipt/order ID is injected into localStorage
      (brave/brave-core#34719)
- [ ] Verify link-order flow works (brave/brave-core#34719)
- [ ] Verify purchase section is hidden when already linked
      (brave/brave-core#34719)

### 11.2 Post-Purchase UX

- [ ] Verify loading state is shown during credential fetch
      (brave/brave-core#34955, brave/brave-browser#53915)
- [ ] Verify toggles are disabled during credential fetch
      (brave/brave-core#34955)
- [ ] Verify loading spinner is visible (brave/brave-core#34955)
- [ ] Verify "Restart now" appears on success (brave/brave-core#34955)

### 11.3 Subscription Features

- [ ] Verify subscription linking across devices (brave/brave-browser#53613,
      brave/brave-core#34719)
- [ ] Verify purchase restoration on device change (brave/brave-browser#53405)

---

## 12. iOS -- Settings and Policies

**Related PRs:** brave/brave-core#33091, #34116, #34938, #34943

### 12.1 Settings Screen

- [ ] Verify Brave Origin settings are accessible on iOS
- [ ] Verify reset to defaults button works (brave/brave-core#34938)
- [ ] Verify all toggles disable back to defaults on reset
- [ ] Verify link options are present in settings view (brave/brave-core#34943)

### 12.2 Service Integration

- [ ] Verify Obj-C++ bridge methods access BraveOriginService correctly
      (brave/brave-core#34116)
- [ ] Verify purchase state is accessible via bridge methods
      (brave/brave-core#34116)
- [ ] Verify local state prefs are registered (brave/brave-core#33091)
- [ ] Verify BraveOriginServiceFactory passes valid skus service getter
      (brave/brave-core#34116)

### 12.3 Purchase Flow

- [ ] Verify App Store paywall UI is presented correctly
- [ ] Verify one-time payment completes successfully
- [ ] Verify subscription actions work in settings (brave/brave-browser#53709)
- [ ] Verify purchase restoration works on new device

---

## 13. Cross-Platform Regression Tests

### 13.1 Standard Brave Not Affected

- [ ] Verify standard Brave builds are unaffected by Origin code paths
- [ ] Verify Origin feature flag defaults to off in standard builds
- [ ] Verify no Origin UI appears in standard Brave without the feature flag
- [ ] Verify no performance regression in standard Brave

### 13.2 Profile Isolation

- [ ] Verify Origin policies are scoped per profile (brave/brave-browser#49285,
      #49299)
- [ ] Verify changing policies in one profile does not affect another

### 13.3 Upgrade/Downgrade

- [ ] Verify upgrading from standard Brave to Origin (upgrade mode) preserves
      user data
- [ ] Verify disabling Origin upgrade restores all features
- [ ] Verify importing from standard Brave to branded Origin works
      (brave/brave-core#34650)

---

## 14. CI and Automated Testing

- [ ] Verify CI builds pass for Origin flavor with each PR (`bot/flavor/origin`
      label)
- [ ] Verify unit tests for startup dialog pass (brave/brave-core#34553)
- [ ] Verify Mojo interface tests pass
- [ ] Verify buildflag static assert tests compile correctly
      (brave/brave-core#33309)
