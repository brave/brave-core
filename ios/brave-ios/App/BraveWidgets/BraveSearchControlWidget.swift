// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import DesignSystem
import Strings
import SwiftUI
import WidgetKit

struct BraveSearchControlWidget: ControlWidget {
  var body: some ControlWidgetConfiguration {
    StaticControlConfiguration(kind: "BraveSearchControlWidget") {
      ControlWidgetButton(action: BraveSearchControlWidgetIntent()) {
        Label(Strings.Widgets.braveSearch, braveSystemImage: "leo.search")
      }
    }
    .displayName(LocalizedStringResource(stringLiteral: Strings.Widgets.braveSearchWidgetTitle))
    .description(
      LocalizedStringResource(stringLiteral: Strings.Widgets.braveSearchWidgetDescription)
    )
  }
}
