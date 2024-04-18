// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveCore
import DesignSystem
import Introspect
import SpeechRecognition
import SwiftUI

private struct MenuScaleTransition: GeometryEffect {
  var scalePercent: Double

  var animatableData: Double {
    get { scalePercent }
    set { scalePercent = newValue }
  }

  func effectValue(size: CGSize) -> ProjectionTransform {
    let projection = ProjectionTransform(
      CGAffineTransform(scaleX: 1.0, y: scalePercent)
    )
    return projection
  }
}

private struct DropdownView<ActionView, MenuView>: View where ActionView: View, MenuView: View {
  @Binding
  var showMenu: Bool

  @ViewBuilder
  let actionView: () -> ActionView

  @ViewBuilder
  let menuView: () -> MenuView

  var body: some View {
    actionView()
      .opacity(showMenu ? 0.7 : 1.0)
      .zIndex(1)
      .overlay(alignment: .bottom) {
        Group {
          if showMenu {
            menuView()
              .transition(
                .modifier(
                  active: MenuScaleTransition(scalePercent: 0.01),
                  identity: MenuScaleTransition(scalePercent: 1.0)
                )
                .combined(with: .opacity)
              )
          }
        }
        .alignmentGuide(.bottom) { $0[.top] }
      }
      .animation(.easeInOut(duration: 0.20), value: showMenu)
  }
}

private struct AIChatDropdownButton: View {
  var action: () -> Void
  var title: String

  var body: some View {
    Button {
      action()
    } label: {
      HStack {
        Text(title)
          .font(.subheadline)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .frame(maxWidth: .infinity, alignment: .leading)

        Image(braveSystemName: "leo.arrow.small-down")
          .foregroundStyle(Color(braveSystemName: .iconDefault))
          .padding(.leading)
      }
      .padding(12.0)
      .overlay(
        ContainerRelativeShape()
          .strokeBorder(Color(braveSystemName: .iconDefault), lineWidth: 0.5)
      )
      .background(
        Color(braveSystemName: .containerBackground),
        in: RoundedRectangle(cornerRadius: 8.0, style: .continuous)
      )
    }
    .buttonStyle(.plain)
  }
}

private struct AIChatDropdownMenu<Item>: View
where Item: RawRepresentable, Item.RawValue: StringProtocol, Item: Identifiable, Item: Hashable {
  @Binding
  var selectedItem: Item
  var items: [Item]

  var body: some View {
    VStack(spacing: 0.0) {
      ForEach(items) { item in
        Button {
          selectedItem = item
        } label: {
          HStack {
            Text(item.rawValue)
              .font(.subheadline)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
              .frame(maxWidth: .infinity, alignment: .leading)

            if item == selectedItem {
              Image(braveSystemName: "leo.check.normal")
                .foregroundStyle(Color(braveSystemName: .iconDefault))
            }
          }
          .padding()
        }

        if let last = items.last, item != last {
          Color(braveSystemName: .dividerSubtle)
            .frame(height: 1.0)
        }
      }
    }
    .overlay(
      ContainerRelativeShape()
        .strokeBorder(Color(braveSystemName: .iconDefault), lineWidth: 0.5)
    )
    .background(
      ContainerRelativeShape()
        .fill(Color(braveSystemName: .containerBackground))
        .shadow(color: .black.opacity(0.25), radius: 10.0, x: 0.0, y: 2.0)
    )
    .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
    .accessibilityRepresentation {
      Picker(selection: $selectedItem) {
        ForEach(items) { item in
          Text(item.rawValue).tag(item.id)
        }
      } label: {
        Text(Strings.AIChat.feedbackOptionsViewTitle)
      }
      .pickerStyle(.segmented)
    }
  }
}

private struct AIChatDropdownView: View {
  @Binding
  var selectedItem: AIChatFeedbackOption

  @State
  private var showMenu = false

  var body: some View {
    VStack {
      HStack(spacing: 0.0) {
        Text(Strings.AIChat.feedbackOptionsViewTitle)
          .font(.caption.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textPrimary))

        Text(verbatim: "*")
          .font(.caption.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorText))
      }
      .frame(maxWidth: .infinity, alignment: .leading)

      DropdownView(showMenu: $showMenu) {
        AIChatDropdownButton(
          action: {
            showMenu.toggle()
          },
          title: selectedItem.rawValue
        )
      } menuView: {
        AIChatDropdownMenu(selectedItem: $selectedItem, items: AIChatFeedbackOption.allCases)
          .offset(x: 0.0, y: 12.0)
          .onChange(of: selectedItem) { _ in
            showMenu = false
          }
      }
    }
  }
}

private struct AIChatFeedbackInputView: View {
  private var speechRecognizer: SpeechRecognizer

  @State
  private var isVoiceEntryPresented = false

  @State
  private var isNoMicrophonePermissionPresented = false

  @Binding
  var text: String

  init(speechRecognizer: SpeechRecognizer, text: Binding<String>) {
    self.speechRecognizer = speechRecognizer
    _text = text
  }

  var body: some View {
    HStack {
      ZStack(alignment: .leading) {
        TextEditor(text: $text)
          .font(.subheadline)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .autocorrectionDisabled()
          .autocapitalization(.none)
          .submitLabel(.done)
          .frame(minHeight: 80.0)
          .osAvailabilityModifiers({ view in
            if #available(iOS 16.4, *) {
              view
                .scrollContentBackground(.hidden)
            } else {
              view.introspectTextView { textView in
                textView.backgroundColor = .clear
              }
            }
          })

        if text.isEmpty {
          Text(Strings.AIChat.feedbackInputViewTitle)
            .font(.subheadline)
            .foregroundColor(Color(braveSystemName: .textTertiary))
            .disabled(true)
            .allowsHitTesting(false)
            .padding(.vertical, 8.0)
            .padding(.horizontal, 5.0)
            .frame(maxHeight: .infinity, alignment: .top)
        }
      }
      .fixedSize(horizontal: false, vertical: true)
      .padding(.trailing)

      if speechRecognizer.isVoiceSearchAvailable {
        Button {
          Task { @MainActor in
            await activateSpeechRecognition()
          }
        } label: {
          Label {
            Text(Strings.AIChat.voiceInputButtonTitle)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
          } icon: {
            Image(braveSystemName: "leo.microphone")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
          }
          .labelStyle(.iconOnly)
        }
      }
    }
    .padding(12.0)
    .overlay {
      ContainerRelativeShape()
        .strokeBorder(Color(braveSystemName: .iconDefault), lineWidth: 0.5)
    }
    .background(
      Color(braveSystemName: .containerBackground),
      in: RoundedRectangle(cornerRadius: 8.0, style: .continuous)
    )
    .background {
      AIChatSpeechRecognitionView(
        speechRecognizer: speechRecognizer,
        isVoiceEntryPresented: $isVoiceEntryPresented,
        isNoMicrophonePermissionPresented: $isNoMicrophonePermissionPresented,
        recognizedText: $text
      )
    }
  }

  @MainActor
  private func activateSpeechRecognition() async {
    let permissionStatus = await speechRecognizer.askForUserPermission()
    if permissionStatus {
      isVoiceEntryPresented = true
    } else {
      isNoMicrophonePermissionPresented = true
    }
  }
}

private struct AIChatFeedbackLeoPremiumAdView: View {
  var body: some View {
    Text(LocalizedStringKey(Strings.AIChat.feedbackPremiumAdTitle))
      .tint(Color(braveSystemName: .textInteractive))
      .font(.subheadline)
      .foregroundStyle(Color(braveSystemName: .textSecondary))
      .fixedSize(horizontal: false, vertical: true)
      .frame(maxWidth: .infinity, alignment: .leading)
      .padding()
      .background(
        LinearGradient(
          gradient:
            Gradient(colors: [
              Color(UIColor(rgb: 0xF8BEDA)).opacity(0.1),
              Color(UIColor(rgb: 0xAD99FF)).opacity(0.1),
            ]),
          startPoint: .init(x: 1.0, y: 1.0),
          endPoint: .zero
        ),
        in: RoundedRectangle(cornerRadius: 8.0, style: .continuous)
      )
  }
}

enum AIChatFeedbackOption: String, CaseIterable, Identifiable {
  case notHelpful
  case incorrect
  case unsafeHarmful
  case other

  // These values are taken from: https://github.com/brave/brave-core/blob/master/components/ai_chat/resources/page/components/feedback_form/index.tsx#L16
  // and must not be changed!
  var id: RawValue {
    switch self {
    case .notHelpful:
      return "not-helpful"
    case .incorrect:
      return "incorrect"
    case .unsafeHarmful:
      return "unsafe-harmful"
    case .other:
      return "other"
    }
  }

  var rawValue: String {
    switch self {
    case .notHelpful:
      return Strings.AIChat.feedbackOptionTitleNotHelpful
    case .incorrect:
      return Strings.AIChat.feedbackOptionTitleIncorrect
    case .unsafeHarmful:
      return Strings.AIChat.feedbackOptionTitleUnsafeHarmful
    case .other:
      return Strings.AIChat.feedbackOptionTitleOther
    }
  }
}

struct AIChatFeedbackModelToast: Equatable {
  let turnId: Int
  let ratingId: String
}

struct AIChatFeedbackView: View {
  @State
  private var category: AIChatFeedbackOption = .notHelpful

  @State
  private var feedbackText: String = ""

  var speechRecognizer: SpeechRecognizer
  var premiumStatus: AiChat.PremiumStatus

  var shouldShowPremiumAd: Bool

  let onSubmit: (String, String) -> Void
  let onCancel: () -> Void

  var body: some View {
    VStack {
      Text(Strings.AIChat.feedbackViewMainTitle)
        .font(.body.weight(.semibold))
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()

      AIChatDropdownView(selectedItem: $category)
        .zIndex(999)
        .padding(.horizontal)

      Text(Strings.AIChat.feedbackInputViewTitle)
        .font(.caption.weight(.semibold))
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding([.horizontal, .top])

      AIChatFeedbackInputView(speechRecognizer: speechRecognizer, text: $feedbackText)
        .padding([.horizontal, .bottom])

      if premiumStatus != .active && premiumStatus != .activeDisconnected && shouldShowPremiumAd {
        AIChatFeedbackLeoPremiumAdView()
          .padding(.horizontal)
      }

      HStack {
        Button {
          onCancel()
        } label: {
          Text(Strings.CancelString)
            .font(.callout.weight(.semibold))
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        .padding()

        Button {
          onSubmit(category.id, feedbackText)
        } label: {
          Text(Strings.AIChat.feedbackSubmitActionTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
      }
      .frame(maxWidth: .infinity, alignment: .trailing)
      .padding()
    }
    .background(Color(braveSystemName: .blue10))
    .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
  }
}

#if DEBUG
struct AIChatFeedbackView_Previews: PreviewProvider {
  static var previews: some View {
    AIChatFeedbackView(
      speechRecognizer: SpeechRecognizer(),
      premiumStatus: .inactive,
      shouldShowPremiumAd: true,
      onSubmit: {
        print("Submitted Feedback: \($0) -- \($1)")
      },
      onCancel: {
        print("Cancelled Feedback")
      }
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
