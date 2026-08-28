// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import BraveWidgetsModels

// This intent must belong to both the app target and the control widget extension because
// `perform()` runs in the app process once `openAppWhenRun` foregrounds it
struct ShortcutControlIntent: AppIntent {
  static var title: LocalizedStringResource = "Open Leo Voice Input"
  static var openAppWhenRun: Bool = true

  func perform() async throws -> some IntentResult {
    PendingWidgetIntentAction.set(.braveLeoVoiceInput)
    return .result()
  }
}
