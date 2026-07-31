// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

/// QA tooling for playlist migrations. Surfaced in Settings → Developer Options → Playlist Debug.
extension Migration {
  @MainActor internal static func debugResetPlaylistLastPlayedDateMigration() {
    Preferences.Migration.playlistLastPlayedDateMigrationCompleted.value = false
  }
}
