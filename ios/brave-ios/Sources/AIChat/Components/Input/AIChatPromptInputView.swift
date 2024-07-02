// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveCore
import DesignSystem
import SpeechRecognition
import SwiftUI

struct AIChatPromptInputView: View {
  private var speechRecognizer: SpeechRecognizer

  @State
  private var isVoiceEntryPresented = false

  @State
  private var isNoMicrophonePermissionPresented = false

  @Binding
  private var prompt: String

  @Binding
  var isShowingSlashTools: Bool

  @Binding
  var slashToolsOption:
    (
      group: AiChat.ActionGroup,
      entry: AiChat.ActionEntry
    )?

  var onSubmit: (String) -> Void

  init(
    prompt: Binding<String>,
    speechRecognizer: SpeechRecognizer,
    isShowingSlashTools: Binding<Bool>,
    slashToolsOption: Binding<
      (
        group: AiChat.ActionGroup,
        entry: AiChat.ActionEntry
      )?
    >,
    onSubmit: @escaping (String) -> Void
  ) {
    self._prompt = prompt
    self.speechRecognizer = speechRecognizer
    self._isShowingSlashTools = isShowingSlashTools
    self._slashToolsOption = slashToolsOption
    self.onSubmit = onSubmit
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      if let slashToolsOption = slashToolsOption {
        Button {
          self.slashToolsOption = nil
        } label: {
          AIChatSlashToolsLabel(group: slashToolsOption.group, entry: slashToolsOption.entry)
            .padding([.leading, .trailing, .top])
            .frame(alignment: .leading)
        }
      }

      AIChatPaddedTextView(
        Strings.AIChat.promptPlaceHolderDescription,
        text: $prompt,
        textColor: UIColor(braveSystemName: .textPrimary),
        prompt: Strings.AIChat.promptPlaceHolderDescription,
        promptColor: UIColor(braveSystemName: .textTertiary),
        font: .preferredFont(forTextStyle: .subheadline),
        submitLabel: .send,
        onBackspace: { wasEmpty in
          if wasEmpty {
            isShowingSlashTools = false
            slashToolsOption = nil
          }
        },
        onTextChanged: { text in
          if text.hasPrefix("/") {
            isShowingSlashTools = true
          } else {
            isShowingSlashTools = false
          }
        },
        onSubmit: {
          if !prompt.isEmpty {
            onSubmit(prompt)
            prompt = ""
          }
        },
        insets: UIEdgeInsets(top: 16.0, left: 15.0, bottom: 0.0, right: 16.0)
      )
      .padding(.top, slashToolsOption == nil ? 8.0 : 0.0)

      HStack(spacing: 0.0) {
        Button(
          action: {
            isShowingSlashTools.toggle()
          },
          label: {
            Label {
              Text(Strings.AIChat.leoSlashToolsButtonAccessibilityTitle)
                .foregroundStyle(Color(braveSystemName: .textPrimary))
            } icon: {
              Image(braveSystemName: "leo.slash")
                .foregroundStyle(Color(braveSystemName: .iconDefault))
                .padding(.horizontal, 1.0)
                .padding(.vertical, 4.0)
                .padding([.leading, .trailing, .bottom])
            }
            .labelStyle(.iconOnly)
          }
        )

        Spacer()

        if prompt.isEmpty {
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
                .padding([.leading, .trailing, .bottom])
            }
            .labelStyle(.iconOnly)
          }
          .opacity(speechRecognizer.isVoiceSearchAvailable ? 1.0 : 0.0)
          .disabled(!speechRecognizer.isVoiceSearchAvailable)
          .frame(width: speechRecognizer.isVoiceSearchAvailable ? nil : 0.0)
        } else {
          Button {
            onSubmit(prompt)
            prompt = ""
          } label: {
            Image(braveSystemName: "leo.send")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
              .padding([.leading, .trailing, .bottom])
          }
        }
      }
    }
    .background(
      ContainerRelativeShape()
        .fill(Color(braveSystemName: .containerBackground))
        .shadow(color: .black.opacity(0.10), radius: 4.0, x: 0.0, y: 1.0)
    )
    .overlay(
      ContainerRelativeShape()
        .strokeBorder(Color(braveSystemName: .dividerStrong), lineWidth: 1.0)
    )
    .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
    .background {
      AIChatSpeechRecognitionView(
        speechRecognizer: speechRecognizer,
        isVoiceEntryPresented: $isVoiceEntryPresented,
        isNoMicrophonePermissionPresented: $isNoMicrophonePermissionPresented,
        recognizedText: $prompt
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

#if DEBUG
struct AIChatPromptInputView_Preview: PreviewProvider {
  static var previews: some View {
    let entry = AiChat.ActionEntry(details: .init(label: "Professional", type: .academicize))

    let group = AiChat.ActionGroup(category: "Change Tone", entries: [entry])

    AIChatPromptInputView(
      prompt: .constant(""),
      speechRecognizer: SpeechRecognizer(),
      isShowingSlashTools: .constant(false),
      slashToolsOption: .constant((group, entry))
    ) {
      print("Prompt Submitted: \($0)")
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
