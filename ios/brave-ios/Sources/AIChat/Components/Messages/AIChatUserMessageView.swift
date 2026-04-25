// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatUserMessageView: View {
  let message: String
  let lastEdited: Date?
  let isEditingMessage: Bool
  var focusedField: FocusState<AIChatView.Field?>.Binding
  let cancelEditing: () -> Void
  let submitEditedText: (String) -> Void

  var body: some View {
    if isEditingMessage {
      AIChatEditingMessageView(
        isUser: true,
        existingText: message,
        focusedField: focusedField,
        isEdited: lastEdited != nil,
        cancel: cancelEditing,
        submitEditedText: submitEditedText
      )
    } else {
      SentUserMessageView(
        message: message,
        lastEdited: lastEdited
      )
    }
  }
}

#if DEBUG
struct AIChatUserMessageView_Previews: PreviewProvider {

  @FocusState static var focusedField: AIChatView.Field?

  static var previews: some View {
    Group {
      AIChatUserMessageView(
        message: "Does it work with Apple devices?",
        lastEdited: Date(),
        isEditingMessage: false,
        focusedField: $focusedField,
        cancelEditing: {},
        submitEditedText: { _ in }
      )
      AIChatUserMessageView(
        message: "Does it work with Apple devices?",
        lastEdited: Date(),
        isEditingMessage: true,
        focusedField: $focusedField,
        cancelEditing: {},
        submitEditedText: { _ in }
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif

private struct SentUserMessageView: View {
  var message: String
  let lastEdited: Date?

  var body: some View {
    VStack(alignment: .leading) {
      AIChatMessageHeaderView(isUser: true, isEdited: lastEdited != nil)

      Text(message)
        .font(.subheadline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)

      if let lastEdited {
        AIChatMessageEditedLabelView(
          lastEdited: lastEdited
        )
        .padding(.top, 8)
      }
    }
  }
}
