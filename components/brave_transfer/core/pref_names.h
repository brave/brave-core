// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_PREF_NAMES_H_

namespace brave_transfer::prefs {

// Profile prefs.

// Master switch, set by the BraveTransferEnabled policy. When false, Brave
// Transfer is unavailable in the profile.
inline constexpr char kEnabled[] = "brave_transfer.enabled";
// brave_transfer::mojom::ReceiveMode.
inline constexpr char kReceiveMode[] = "brave_transfer.receive_mode";
inline constexpr char kAutoAcceptTrusted[] =
    "brave_transfer.auto_accept_trusted";
inline constexpr char kAutoTrustSyncChain[] =
    "brave_transfer.auto_trust_sync_chain";
inline constexpr char kNotifyOnReceive[] = "brave_transfer.notify_on_receive";
// Empty means the browser download folder, resolved for each received file.
inline constexpr char kSaveDirectory[] = "brave_transfer.save_directory";
// The profile's X25519 static private key, encrypted with OSCrypt and base64
// encoded.
inline constexpr char kStaticKey[] = "brave_transfer.static_key";
// Dictionary keyed by sync device GUID: pinned static key, last known name and
// type, per-device policy and last-seen time.
inline constexpr char kTrustedDevices[] = "brave_transfer.trusted_devices";
inline constexpr char kShowToolbarButton[] =
    "brave_transfer.show_toolbar_button";

}  // namespace brave_transfer::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_PREF_NAMES_H_
