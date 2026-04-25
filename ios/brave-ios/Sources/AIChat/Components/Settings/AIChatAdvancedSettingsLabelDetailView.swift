// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveUI
import SwiftUI

public struct AIChatAdvancedSettingsLabelDetailView: View {
  let title: String
  var detail: String?  // When detail is nil, a Progress View is shown

  public init(title: String, detail: String?) {
    self.title = title
    self.detail = detail
  }

  public var body: some View {
    HStack {
      LabelView(title: title)
      Spacer()

      if let detail = detail {
        Text(detail)
          .foregroundColor(Color(.braveLabel))
      } else {
        ProgressView()
      }
    }
  }
}
