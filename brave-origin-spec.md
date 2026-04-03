# Brave Origin -- Product Specification

## Overview

Brave Origin is a product offering that strips down or disables all
non-privacy/security essential features from the Brave browser. It addresses
users who want a simple, privacy-focused browser without Wallet, Leo AI, VPN,
Tor, Rewards, News, and other optional features.

Brave Origin is available in two modes:

1. **Brave Origin Branded Build** -- A separate build product where
   non-essential features are compiled out at build time.
2. **Brave Origin Upgrade** -- An upgrade for existing Brave installations that
   disables non-essential features via policies at runtime.

## Distribution Modes

### Mode 1: Brave Origin Branded Build (Separate Product)

- **Tracking issue:** brave/brave-browser#50156
- **GN build flag:** `is_brave_origin_branded`
- **Platform restriction:** Desktop only (macOS, Windows, Linux). A GN assertion
  fails the build if this flag is enabled on Android or iOS
  (brave/brave-core#33846).

In this mode, non-essential features are **compiled out** and cannot be turned
on. The build has its own branding, app icons, binary names, installer GUIDs,
and update paths.

**Build flags set to false for Origin builds:**

| Flag                           | Feature                                                      |
| ------------------------------ | ------------------------------------------------------------ |
| `enable_brave_rewards`         | Brave Rewards                                                |
| `enable_ai_chat`               | Leo AI Chat                                                  |
| `enable_brave_news`            | Brave News                                                   |
| `enable_brave_talk`            | Brave Talk                                                   |
| `enable_tor`                   | Tor                                                          |
| `enable_brave_vpn`             | Brave VPN                                                    |
| `enable_brave_wallet`          | Brave Wallet                                                 |
| `enable_brave_wayback_machine` | Wayback Machine                                              |
| `enable_speedreader`           | Speedreader                                                  |
| `enable_web_discovery_native`  | Web Discovery                                                |
| Brave Ads build flags          | Brave Ads (buildflag static asserts, brave/brave-core#33309) |

**Additional runtime defaults for branded builds:**

- P3A / statistics reporting: off by default, P3A infobar compiled out
  (brave/brave-core#34612)
- Crash reporting: default off (brave/brave-browser#54131)
- Sidebar: default off (brave/brave-browser#54130)
- Playlist: excluded (brave/brave-browser#54129)

**Startup dialog:**

Brave Origin branded builds show a purchase validation dialog before any browser
window opens. This is a standalone modal WebUI dialog that:

- Intercepts `StartupBrowserCreator::Start` to block browser launch until
  validated (brave/brave-core#34555)
- Manages system profile creation and web contents lifecycle
  (brave/brave-core#34554)
- Has full Mojo interface, WebUI handler, frontend, and unit tests
  (brave/brave-core#34553)
- Removes the Profile menu on macOS to prevent bypass (brave/brave-core#34699)
- On Linux, shows a free tier option allowing use without purchase
  (brave/brave-core#34721)
- Styling uses user profile to enable Nala theming (brave/brave-core#34739)
- Scrollable for long translations (brave/brave-browser#53790)

**Branding and packaging:**

- Separate branding directory (`brave_origin/`) with its own GRD files, icons,
  DS_Store files, and theme assets (brave/brave-core#33045,
  brave/brave-core#33239)
- macOS DMG has custom layout and background image (brave/brave-core#34710,
  brave/brave-core#34740)
- Windows: unique GUIDs, installer names, update paths (brave/brave-core#33184),
  RC brand generation fix (brave/brave-core#33279), correct icon resolution
  (brave/brave-core#35081, brave/brave-core#35157)
- Linux: origin-branded DEB, RPM, ZIP, symbols, appdata.xml
  (brave/brave-core#34939, brave/brave-core#34998)
- Strings: shares XTB translation files with Brave but has separate GRD branding
  files (brave/brave-core#33261, brave/brave-browser#51497)
- URL bar icon for `brave://` pages uses Origin branding
  (brave/brave-core#33186)
- Settings footer, help page, and channel logo use Origin theme
  (brave/brave-core#33239)
- Windows product names, shortcuts, and accessible window titles show "Brave
  Origin" (brave/brave-core#34870)

**Onboarding:**

- Removes WDP (Web Discovery Project) step
- Removes telemetry checkbox
- Changes title on last step
- Crash reporting defaults to on during onboarding (brave/brave-core#33257)

**Browser import:**

- Allows importing data from standard Brave into Brave Origin
  (brave/brave-core#34650)

### Mode 2: Brave Origin Upgrade (Policy-Based)

- **Tracking issue:** brave/brave-browser#49212
- **Platform:** Desktop (macOS, Windows, Linux), Android, iOS

In this mode, existing Brave installations can be upgraded to the Origin
experience. Features are disabled by default via admin policies, but users can
selectively re-enable what they want.

**Settings page:**

- Available at `brave://settings/origin` with toggles for each feature
  (brave/brave-core#31677)
- Shown when the feature flag is on AND (purchased OR branded build)
- Reset to defaults functionality (brave/brave-browser#47977)
- Smart about not disabling features users are actively using (e.g., paid Leo
  subscription) (brave/brave-browser#48145)
- Relaunch notification overlay when policies change (brave/brave-browser#48142)

**Purchase option in existing Brave:**

- Located in System settings at the bottom (intentionally hidden)
  (brave/brave-core#33325)
- Uses `brave_domains::GetServicesDomain` for the buy URL
  (brave/brave-core#33838)
- Shows purchase UI when feature flag is on AND NOT a branded build
  (brave/brave-core#35173)

## Architecture

### Core Components

**BraveOriginService** -- A keyed service per profile
(brave/brave-browser#49214) that:

- Manages the Origin state for each profile
- Handles purchase state tracking for the upgrade case (brave/brave-core#34044)
- Communicates with the Settings UI via Mojo interface (brave/brave-core#31404)
- Is force-enabled for branded builds to keep settings in sync
  (brave/brave-core#33258)

**BraveOriginServiceFactory** -- Factory for the keyed service.

**Brave Origin Policy Manager** -- A singleton that:

- Holds preference definitions
- Manages access to policy values from local state (brave/brave-browser#49343)
- Implements a custom policy source type added to Chromium's policy system
  (brave/brave-browser#49042)
- Provides browser-level and profile-level policy providers
  (brave/brave-core#30984, brave/brave-core#30996)
- Policies stored in local state, scoped by Profile ID
  (brave/brave-browser#49285, brave/brave-browser#49299)

**BraveOriginUtils** -- Utility functions (brave/brave-browser#49316).

**"Managed By" UI** -- Hidden for Brave Origin-only policies so users do not see
enterprise management indicators (brave/brave-browser#49679).

### Features Controlled by Policies

| Feature          | Policy Control                  |
| ---------------- | ------------------------------- |
| Brave Rewards    | Yes                             |
| Brave Ads        | Yes                             |
| Leo AI Chat      | Yes                             |
| Brave News       | Yes                             |
| Brave Talk       | Yes                             |
| Tor              | Yes                             |
| Brave VPN        | Yes                             |
| Brave Wallet     | Yes                             |
| Wayback Machine  | Yes                             |
| Speedreader      | Yes                             |
| Web Discovery    | Yes (brave/brave-browser#50009) |
| P3A / Statistics | Default off, no infobar         |
| Crash Reporting  | Default off for branded builds  |
| Email Aliases    | Yes                             |
| Sidebar          | Yes                             |
| Playlist         | Yes (brave/brave-browser#54128) |

### GN Build Targets

- Pref registration and feature flag targets are split out for better modularity
  (brave/brave-core#33322)
- Mojo common directory removed as unnecessary (brave/brave-core#33277)

## Pricing Model

- **Free on Linux** for both modes
- **One-time payment** on macOS, Windows, Android, and iOS (changed from
  subscription to one-time during development)
- Uses Brave's SKUs SDK for purchase verification

### Platform-Specific Payment Integration

**Desktop:**

- Client-side SKU code (brave/brave-browser#47463)
- Purchase through `account.brave.com` via `brave_domains::GetServicesDomain`

**Android:**

- Google Play Store billing client integration (brave/brave-browser#53006,
  brave/brave-browser#52855)
- `OriginIAPSubscription` Mojo interface handles `product=origin` in
  `SubscriptionRenderFrameObserver` (brave/brave-core#34719)
- Injects purchase receipt/order ID into localStorage, handles link-order flow
- Hides purchase section when already linked
- Loading state during credential fetch (brave/brave-browser#53915,
  brave/brave-core#34955)

**iOS:**

- App Store paywall UI
- BraveOriginService accessed via Obj-C++ bridge methods
  (brave/brave-core#34116)
- Local state pref registration (brave/brave-core#33091)
- Subscription actions in settings (brave/brave-browser#53709)

### Cross-Device Features

- Subscription linking across devices (brave/brave-browser#53613,
  brave/brave-core#34719)
- Purchase restoration on device change (brave/brave-browser#53405)

## Platform Support

| Platform                        | Branded Build | Upgrade Mode | Tracking Issue            |
| ------------------------------- | ------------- | ------------ | ------------------------- |
| Desktop (macOS, Windows, Linux) | Yes           | Yes          | brave/brave-browser#37127 |
| Android                         | No            | Yes          | brave/brave-browser#37128 |
| iOS                             | No            | Yes          | brave/brave-browser#37129 |

### Android-Specific Work

- Settings screen (brave/brave-browser#50270)
- SKUS integration (brave/brave-browser#49619)
- Policy integration (brave/brave-browser#51333, brave/brave-browser#51412)
- Per-feature policy-based UI updates:
  - Rewards (brave/brave-browser#51826)
  - AI Chat (brave/brave-browser#51974)
  - News (brave/brave-browser#52170)
  - VPN (brave/brave-browser#52355)
  - Wallet (brave/brave-browser#52496)
- Restart dialog (brave/brave-browser#52774)
- One-time payment billing API (brave/brave-browser#53006,
  brave/brave-browser#52855)
- Purchase restoration (brave/brave-browser#53405)
- Subscription linking (brave/brave-browser#53613)
- Loading state during credential fetch (brave/brave-browser#53915)
- Leo design tokens for settings icons (brave/brave-core#34411)
- Settings search indexing respects Origin policies (brave/brave-core#34985)
- App icon update on purchase (brave/brave-browser#53614, post-MVP)

### iOS-Specific Work

- Policy connector overrides
- Keyed service factory with valid skus service getter (brave/brave-core#34116)
- Obj-C++ wrappers for the service
- Settings UI integration
- Reset to defaults in settings (brave/brave-core#34938)
- Link options in settings view (brave/brave-core#34943)
- App Store paywall UI
- SKUS integration for one-time payments
- Subscription actions in settings (brave/brave-browser#53709)
- App icon update on purchase (brave/brave-browser#53612, post-MVP)

## Quality and Security

- **Network request audit** to minimize telemetry (brave/brave-browser#47557),
  referencing sizeof.cat browser telemetry analysis
- **Running processes/services audit** to ensure nothing runs for disabled
  features (brave/brave-browser#47474)
- **Memory leak fix** in BraveOriginService (brave/brave-browser#52757)
- **CI builds** for Origin with each PR, tracked with `bot/flavor/origin` label

## Complete Issue and PR Reference

### Tracking/Parent Issues (brave/brave-browser)

- #37127 -- Desktop tracking
- #37128 -- Android tracking
- #37129 -- iOS tracking
- #37130, #37131 -- Payments tracking
- #49212 -- Upgrade mode tracking
- #50156 -- Branded build tracking

### Core Architecture Issues (brave/brave-browser)

#49042, #49214, #49219, #49285, #49299, #49316, #49343, #49462, #49605, #48157,
#48200, #50009, #50078, #52757

### Settings UI Issues (brave/brave-browser)

#47461, #47977, #48142, #48144, #48145, #50270, #52687, #52716, #53510

### Build and Branding Issues (brave/brave-browser)

#50078, #51239, #51497, #51976, #52711

### Android Issues (brave/brave-browser)

#51333, #51412, #51826, #51894, #51952, #51974, #52170, #52355, #52496, #52774,
#53323, #53334, #53435, #53614

### iOS Issues (brave/brave-browser)

#53612, #53709

### Branded Build Issues (brave/brave-browser)

#53551, #53780, #53790, #54128, #54129, #54130, #54131

### Payments Issues (brave/brave-browser)

#47463, #49619, #52855, #53006, #53405, #53613, #53915

### Audit Issues (brave/brave-browser)

#47474, #47557, #49968

### Pull Requests (brave/brave-core)

**Core Architecture:** #30621, #30984, #30996, #31242, #31249, #31404, #33258,
#33277, #33322, #33846

**Settings UI:** #30360, #31677, #33321, #33325, #33838, #34411, #34938, #34943,
#34955, #34985, #35173

**Branding and Packaging:** #32649, #33031, #33045, #33151, #33184, #33186,
#33239, #33261, #33279, #34710, #34740, #34870, #34939, #34998, #35081, #35156,
#35157

**Startup Dialog:** #34540, #34541, #34542, #34543, #34553, #34554, #34555,
#34699, #34721, #34739

**Feature Disabling:** #33257, #33309, #34612

**Purchase and Subscription:** #33091, #34044, #34116, #34719
