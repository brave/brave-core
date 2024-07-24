// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

private struct AIChatFeedbackToastModifier: ViewModifier {
  @State
  private var task: Task<Void, Error>?

  private let displayDuration = 5.0

  @Binding
  var toastType: AIChatFeedbackToastType

  func body(content: Content) -> some View {
    content
      .overlay(alignment: .bottom) {
        VStack(spacing: 0.0) {
          if toastType != .none {
            AIChatFeedbackToastView(toastType: $toastType)
              .transition(.move(edge: .bottom).combined(with: .opacity))
              .offset(y: -8.0)
          }
        }
        .animation(.spring(), value: toastType)
      }
      .onChange(of: toastType) { toastType in
        if toastType != .none {
          show()
        }
      }
  }

  private func show() {
    guard displayDuration > 0.0 else { return }
    UIImpactFeedbackGenerator(style: .light).impactOccurred()

    task?.cancel()
    task = Task.delayed(bySeconds: displayDuration) { @MainActor in
      dismiss()
    }
  }

  private func dismiss() {
    task?.cancel()
    task = nil
    toastType = .none
  }
}

enum AIChatFeedbackToastType: Equatable {
  case none
  case error(message: String)
  case success(isLiked: Bool, onAddFeedback: (() -> Void)? = nil)
  case submitted

  static func == (lhs: AIChatFeedbackToastType, rhs: AIChatFeedbackToastType) -> Bool {
    switch (lhs, rhs) {
    case (.none, .none), (.submitted, .submitted):
      return true
    case (.error(let errorA), .error(let errorB)):
      return errorA == errorB
    case (
      .success(let isLikedA, let onAddFeedbackA),
      .success(let isLikedB, let onAddFeedbackB)
    ):
      return isLikedA == isLikedB
        && String(describing: onAddFeedbackA) == String(describing: onAddFeedbackB)
    default:
      return false
    }
  }
}

struct AIChatFeedbackToastView: View {
  @Binding
  var toastType: AIChatFeedbackToastType

  var body: some View {
    HStack(spacing: 0.0) {
      Text(title)
        .font(.subheadline)
        .foregroundStyle(Color(braveSystemName: .neutral10))
        .fixedSize(horizontal: false, vertical: true)
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.trailing)

      trailingViewButton()
    }
    .padding()
    .background(
      ContainerRelativeShape()
        .fill(Color(braveSystemName: .primary70))
        .shadow(color: Color.black.opacity(0.25), radius: 8.0, x: 0.0, y: 1.0)
    )
    .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
    .padding(.horizontal)
    .colorScheme(.light)
  }

  private var title: String {
    switch toastType {
    case .none:
      return ""
    case .error(let message):
      return message
    case .success(let isLiked, _):
      return isLiked
        ? Strings.AIChat.feedbackSuccessAnswerLikedTitle
        : Strings.AIChat.feedbackSuccessAnswerDisLikedTitle
    case .submitted:
      return Strings.AIChat.feedbackSubmittedTitle
    }
  }

  @ViewBuilder
  private func trailingViewButton() -> some View {
    switch toastType {
    case .none:
      EmptyView()
    case .success(let isLiked, let onAddFeedback):
      if isLiked {
        Button {
          toastType = .none
        } label: {
          Label {
            Text(Strings.close)

          } icon: {
            Image(systemName: "xmark")
              .foregroundStyle(Color(braveSystemName: .primary30))
          }
          .labelStyle(.iconOnly)
        }
      } else {
        Button {
          toastType = .none
          onAddFeedback?()
        } label: {
          Text(Strings.AIChat.addFeedbackActionTitle)
            .font(.subheadline)
            .foregroundStyle(Color(braveSystemName: .neutral10))
        }
      }
    case .error, .submitted:
      Button {
        toastType = .none
      } label: {
        Label {
          Text(Strings.close)
        } icon: {
          Image(systemName: "xmark")
            .foregroundStyle(Color(braveSystemName: .primary30))
        }
        .labelStyle(.iconOnly)
      }
    }
  }
}

extension View {
  func toastView(_ toastType: Binding<AIChatFeedbackToastType>) -> some View {
    self.modifier(AIChatFeedbackToastModifier(toastType: toastType))
  }
}

#if DEBUG
struct AIChatFeedbackToastView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatFeedbackToastView(toastType: .constant(.success(isLiked: true)))
      .previewLayout(.sizeThatFits)
  }
}
#endif
