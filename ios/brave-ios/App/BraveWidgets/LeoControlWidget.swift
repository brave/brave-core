// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import DesignSystem
import Strings
import SwiftUI
import WidgetKit

struct LeoControlWidget: ControlWidget {
  var body: some ControlWidgetConfiguration {
    StaticControlConfiguration(kind: "LeoControlWidget") {
      ControlWidgetButton(action: LeoControlWidgetIntent()) {
        Label(Strings.Widgets.braveLeo, braveSystemImage: "leo.product.brave-leo")
      }
    }
    .displayName(LocalizedStringResource(stringLiteral: Strings.Widgets.leoAIWidgetTitle))
    .description(
      LocalizedStringResource(stringLiteral: Strings.Widgets.leoAIWidgetDescription)
    )
  }
}
