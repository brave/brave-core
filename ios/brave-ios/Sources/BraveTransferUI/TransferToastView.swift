// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Strings
import SwiftUI

/// The receive toast (design plan §5.2), shown over the current tab while
/// "Show notification on receive" is on.
public struct TransferToastView: View {
  public enum Style {
    case received
    case warning
  }

  public var message: String
  public var style: Style
  public var onView: (() -> Void)?

  public init(message: String, style: Style = .received, onView: (() -> Void)? = nil) {
    self.message = message
    self.style = style
    self.onView = onView
  }

  public static func receivedMessage(count: Int, from deviceName: String) -> String {
    count == 1
      ? String.localizedStringWithFormat(Strings.BraveTransfer.receivedOne, deviceName)
      : String.localizedStringWithFormat(Strings.BraveTransfer.receivedMany, count, deviceName)
  }

  public var body: some View {
    HStack(spacing: 12) {
      Image(
        systemName: style == .received
          ? "checkmark.circle.fill" : "exclamationmark.triangle.fill"
      )
      .foregroundStyle(
        Color(
          braveSystemName: style == .received
            ? .systemfeedbackSuccessIcon : .systemfeedbackErrorIcon
        )
      )
      .accessibilityHidden(true)
      Text(message)
        .font(.subheadline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .frame(maxWidth: .infinity, alignment: .leading)
      if let onView {
        Button(Strings.BraveTransfer.view, action: onView)
          .font(.subheadline.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textInteractive))
      }
    }
    .padding(.horizontal, 16)
    .padding(.vertical, 12)
    .background(
      Color(braveSystemName: .containerBackground),
      in: .rect(cornerRadius: 12)
    )
    .shadow(color: .black.opacity(0.15), radius: 8, y: 2)
    .padding(.horizontal, 16)
    .accessibilityElement(children: .combine)
  }
}

#if DEBUG
#Preview {
  VStack {
    TransferToastView(
      message: TransferToastView.receivedMessage(count: 3, from: "James's MacBook"),
      onView: {}
    )
    TransferToastView(message: "Blocked a connection from Mac", style: .warning)
  }
}
#endif
