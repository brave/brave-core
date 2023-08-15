// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

struct ShieldLabelView: View {
  let title: String
  var subtitle: String?
  
  var body: some View {
    VStack(alignment: .leading, spacing: 4) {
      Text(title)
        .foregroundColor(Color(.bravePrimary))
      if let subtitle = subtitle {
        Text(LocalizedStringKey(subtitle))
          .foregroundColor(Color(.braveLabel))
          .font(.caption)
      }
    }
  }
}
