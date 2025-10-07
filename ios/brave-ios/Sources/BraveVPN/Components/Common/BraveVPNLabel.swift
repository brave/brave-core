// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct BraveVPNLabelView: View {
  let title: String
  let titleAccessory: UIImage?
  var subtitle: String?

  init(
    title: String,
    titleAccessory: UIImage? = nil,
    subtitle: String? = nil
  ) {
    self.title = title
    self.titleAccessory = titleAccessory
    self.subtitle = subtitle
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 4) {
      HStack {
        Text(title)
          .foregroundColor(Color(.bravePrimary))
        if let titleAccessory = titleAccessory {
          Image(uiImage: titleAccessory)
            .resizable()
            .renderingMode(.template)
            .aspectRatio(contentMode: .fit)
            .foregroundColor(Color(braveSystemName: .iconDefault))
            .frame(width: 14, height: 14)
            .padding(4)
            .background(
              RoundedRectangle(cornerRadius: 4, style: .continuous)
                .fill(Color(braveSystemName: .containerHighlight))
            )
        }
      }
      if let subtitle = subtitle {
        Text(LocalizedStringKey(subtitle))
          .foregroundColor(Color(.braveLabel))
          .font(.caption)
      }
    }
  }
}
