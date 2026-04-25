// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatEditingMessageView: View {

  let isUser: Bool
  let existingText: String
  @State private var text: String
  var focusedField: FocusState<AIChatView.Field?>.Binding
  let isEdited: Bool
  let cancel: () -> Void
  let submitEditedText: (String) -> Void

  @ScaledMetric private var buttonSize: CGFloat = 28

  init(
    isUser: Bool,
    existingText: String,
    focusedField: FocusState<AIChatView.Field?>.Binding,
    isEdited: Bool,
    cancel: @escaping () -> Void,
    submitEditedText: @escaping (String) -> Void
  ) {
    self.isUser = isUser
    self.existingText = existingText
    self._text = State(wrappedValue: existingText)
    self.focusedField = focusedField
    self.isEdited = isEdited
    self.cancel = cancel
    self.submitEditedText = submitEditedText
  }

  var body: some View {
    VStack(alignment: .leading) {
      AIChatMessageHeaderView(isUser: isUser, isEdited: isEdited)

      HStack {
        TextField("", text: $text, axis: .vertical)
          .focused(focusedField, equals: .editing)
        Button(
          action: cancel,
          label: {
            Image(braveSystemName: "leo.close")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
              .frame(width: buttonSize, height: buttonSize)
              .contentShape(Rectangle())
          }
        )
        Button(
          action: { submitEditedText(text) },
          label: {
            Image(braveSystemName: "leo.check.normal")
              .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
              .frame(width: buttonSize, height: buttonSize)
              .background(
                Color(braveSystemName: .buttonBackground)
                  .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
              )
              .contentShape(Rectangle())
          }
        )
      }
      .padding(8)
      .background(
        RoundedRectangle(cornerRadius: 8, style: .continuous)
          .stroke(Color(braveSystemName: .dividerSubtle), lineWidth: 1)
      )
    }
  }
}
