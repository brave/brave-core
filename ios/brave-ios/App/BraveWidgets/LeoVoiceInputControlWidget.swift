// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import DesignSystem
import Strings
import SwiftUI
import WidgetKit

struct LeoVoiceInputControlWidget: ControlWidget {
  var body: some ControlWidgetConfiguration {
    StaticControlConfiguration(kind: "LeoVoiceInputControlWidget") {
      ControlWidgetButton(action: ShortcutControlIntent()) {
        Label(Strings.Widgets.braveLeoVoiceInput, braveSystemImage: "leo.leo.voice-input")
      }
    }
    .displayName(LocalizedStringResource(stringLiteral: Strings.Widgets.leoVoiceInputWidgetTitle))
    .description(
      LocalizedStringResource(stringLiteral: Strings.Widgets.leoVoiceInputWidgetDescription)
    )
  }
}
