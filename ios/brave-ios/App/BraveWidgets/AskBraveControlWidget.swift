// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import DesignSystem
import Strings
import SwiftUI
import WidgetKit

struct AskBraveControlWidget: ControlWidget {
  var body: some ControlWidgetConfiguration {
    StaticControlConfiguration(kind: "AskBraveControlWidget") {
      ControlWidgetButton(action: AskBraveControlWidgetIntent()) {
        Label(Strings.Widgets.askBrave, braveSystemImage: "leo.brave.ask")
      }
    }
    .displayName(LocalizedStringResource(stringLiteral: Strings.Widgets.askBraveWidgetTitle))
    .description(
      LocalizedStringResource(stringLiteral: Strings.Widgets.askBraveWidgetDescription)
    )
  }
}
