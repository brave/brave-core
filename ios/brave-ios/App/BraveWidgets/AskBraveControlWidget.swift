// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import BraveWidgetsModels
import DesignSystem
import Strings
import SwiftUI
import WidgetKit

struct AskBraveControlWidget: ControlWidget {
  var body: some ControlWidgetConfiguration {
    if #available(iOS 26.0, *) {
      let shortcut = WidgetShortcut.askBrave
      return StaticControlConfiguration(kind: "AskBraveControlWidget") {
        ControlWidgetButton(action: OpenControlWidgetShortcutIntent(shortcut: shortcut)) {
          Label(shortcut.displayString, braveSystemImage: shortcut.braveSystemImageName ?? "")
        }
      }
      .displayName(LocalizedStringResource(stringLiteral: Strings.Widgets.askBraveWidgetTitle))
      .description(
        LocalizedStringResource(stringLiteral: Strings.Widgets.askBraveWidgetDescription)
      )
    } else {
      return EmptyControlWidgetConfiguration()
    }
  }
}
