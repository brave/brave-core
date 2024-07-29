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
  var isFieldFocused: FocusState<Bool>.Binding
  let cancelEditing: () -> Void
  let submitEditedText: (String) -> Void

  var body: some View {
    if isEditingMessage {
      EditingUserMessageView(
        existingText: message,
        isFieldFocused: isFieldFocused,
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

  @FocusState static var isPreviewFieldFocused: Bool

  static var previews: some View {
    Group {
      AIChatUserMessageView(
        message: "Does it work with Apple devices?",
        lastEdited: Date(),
        isEditingMessage: false,
        isFieldFocused: $isPreviewFieldFocused,
        cancelEditing: {},
        submitEditedText: { _ in }
      )
      AIChatUserMessageView(
        message: "Does it work with Apple devices?",
        lastEdited: Date(),
        isEditingMessage: true,
        isFieldFocused: $isPreviewFieldFocused,
        cancelEditing: {},
        submitEditedText: { _ in }
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif

private struct UserHeaderView: View {

  var body: some View {
    HStack {
      Image(braveSystemName: "leo.user.circle")
        .renderingMode(.template)
        .foregroundStyle(Color(braveSystemName: .iconDefault))
        .padding(8.0)
        .background(Color(braveSystemName: .containerHighlight), in: Circle())

      Text(Strings.AIChat.youMessageTitle)
        .font(.body.weight(.semibold))
        .foregroundStyle(Color(braveSystemName: .textTertiary))
    }
  }
}

private struct SentUserMessageView: View {
  var message: String
  let lastEdited: Date?

  private var lastEditedTime: String? {
    guard let lastEdited else { return nil }
    let formatter = DateFormatter()
    formatter.dateFormat = "MMM d yyyy - h:mm a"
    return formatter.string(from: lastEdited)
  }

  var body: some View {
    VStack(alignment: .leading) {
      UserHeaderView()

      VStack(alignment: .leading, spacing: 16) {
        Text(message)
          .font(.subheadline)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .multilineTextAlignment(.leading)
          .frame(maxWidth: .infinity, alignment: .leading)
          .fixedSize(horizontal: false, vertical: true)

        if let lastEditedTime {
          HStack {
            Image(braveSystemName: "leo.edit.pencil")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
            Text(Strings.AIChat.editedMessageCaption)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
            Text(lastEditedTime)
              .underline()
              .foregroundStyle(Color(braveSystemName: .textTertiary))
          }
          .font(.caption)
        }
      }
    }
  }
}

private struct EditingUserMessageView: View {

  let existingText: String
  @State private var text: String
  var isFieldFocused: FocusState<Bool>.Binding
  let cancel: () -> Void
  let submitEditedText: (String) -> Void

  @ScaledMetric private var buttonSize: CGFloat = 28

  init(
    existingText: String,
    isFieldFocused: FocusState<Bool>.Binding,
    cancel: @escaping () -> Void,
    submitEditedText: @escaping (String) -> Void
  ) {
    self.existingText = existingText
    self._text = State(wrappedValue: existingText)
    self.isFieldFocused = isFieldFocused
    self.cancel = cancel
    self.submitEditedText = submitEditedText
  }

  var body: some View {
    VStack(alignment: .leading) {
      UserHeaderView()

      HStack {
        TextField("", text: $text, axis: .vertical)
          .focused(isFieldFocused)
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
              .foregroundStyle(Color.white)
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
