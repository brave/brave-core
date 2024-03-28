// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import DesignSystem
import SpeechRecognition
import SwiftUI

struct AIChatPromptInputView: View {
  private var speechRecognizer: SpeechRecognizer

  @State
  private var isVoiceEntryPresented = false

  @State
  private var isNoMicrophonePermissionPresented = false

  @State
  private var prompt: String = ""

  var onSubmit: (String) -> Void

  init(speechRecognizer: SpeechRecognizer, onSubmit: @escaping (String) -> Void) {
    self.speechRecognizer = speechRecognizer
    self.onSubmit = onSubmit
  }

  var body: some View {
    HStack(spacing: 0.0) {
      AIChatPaddedTextField(
        Strings.AIChat.promptPlaceHolderDescription,
        text: $prompt,
        textColor: UIColor(braveSystemName: .textPrimary),
        prompt: Strings.AIChat.promptPlaceHolderDescription,
        promptColor: UIColor(braveSystemName: .textTertiary),
        font: .preferredFont(forTextStyle: .subheadline),
        submitLabel: .send,
        onSubmit: {
          if !prompt.isEmpty {
            onSubmit(prompt)
            prompt = ""
          }
        },
        insets: .init(width: 16.0, height: 16.0)
      )

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
              .padding()
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
            .padding()
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
    AIChatPromptInputView(speechRecognizer: SpeechRecognizer()) {
      print("Prompt Submitted: \($0)")
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
