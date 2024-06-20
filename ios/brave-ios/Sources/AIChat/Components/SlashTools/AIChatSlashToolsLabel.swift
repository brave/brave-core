// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

struct AIChatSlashToolsLabel: View {
  let group: AiChat.ActionGroup
  let entry: AiChat.ActionEntry?

  var body: some View {
    HStack(spacing: 2.0) {
      Text(group.category)

      Image(braveSystemName: "leo.carat.right")

      if let entry = entry {
        if entry.tag == .subheading {
          Text(entry.subheading ?? "")
        } else if entry.tag == .details {
          Text(entry.details?.label ?? "")
        }

        Label(Strings.close, braveSystemImage: "leo.close")
          .labelStyle(.iconOnly)
          .padding(.leading, 2.0)
      }
    }
    .font(.caption2.weight(.semibold))
    .foregroundColor(Color(braveSystemName: .primary50))
    .padding(4.0)
    .background(Color(braveSystemName: .primary20), in: .containerRelative)
    .containerShape(RoundedRectangle(cornerRadius: 4.0, style: .continuous))
  }
}

#Preview {
  AIChatSlashToolsLabel(
    group: .init(
      category: "Rewrite",
      entries: [
        .init(details: .init(label: "Change tone / Academic", type: .academicize))
      ]
    ),
    entry: .init(details: .init(label: "Change tone / Academic", type: .academicize))
  )
}
