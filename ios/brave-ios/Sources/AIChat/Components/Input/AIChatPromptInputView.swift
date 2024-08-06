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

  @Environment(\.isEnabled) private var isEnabled

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
        textColor: isEnabled
          ? UIColor(braveSystemName: .textPrimary) : UIColor(braveSystemName: .textDisabled),
        prompt: Strings.AIChat.promptPlaceHolderDescription,
        promptColor: isEnabled
          ? UIColor(braveSystemName: .textTertiary) : UIColor(braveSystemName: .textDisabled),
        font: .preferredFont(forTextStyle: .subheadline),
        submitLabel: .default,
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
                .foregroundStyle(
                  isEnabled
                    ? Color(braveSystemName: .iconDefault) : Color(braveSystemName: .iconDisabled)
                )
                .padding(.horizontal, 8.0)
                .padding(.vertical, 4.0)
                .padding(.bottom, 16.0)
            }
            .labelStyle(.iconOnly)
          }
        )

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
              .foregroundStyle(
                isEnabled
                  ? Color(braveSystemName: .iconDefault) : Color(braveSystemName: .iconDisabled)
              )
              .padding(.horizontal, 8.0)
              .padding(.vertical, 4.0)
              .padding(.bottom, 16.0)
          }
          .labelStyle(.iconOnly)
        }
        .opacity(speechRecognizer.isVoiceSearchAvailable ? 1.0 : 0.0)
        .disabled(!speechRecognizer.isVoiceSearchAvailable)
        .frame(width: speechRecognizer.isVoiceSearchAvailable ? nil : 0.0)

        Spacer()

        Button {
          onSubmit(prompt)
          prompt = ""
        } label: {
          Image(braveSystemName: prompt.isEmpty ? "leo.send" : "leo.send.filled")
            .foregroundStyle(
              Color(
                braveSystemName: (prompt.isEmpty || !isEnabled) ? .iconDisabled : .iconInteractive
              )
            )
            .padding(.horizontal, 8.0)
            .padding(.vertical, 4.0)
            .padding(.bottom, 16.0)
        }
        .disabled(prompt.isEmpty)
      }
      .padding(.horizontal, 8.0)
    }
    .background(
      ContainerRelativeShape()
        .fill(Color(braveSystemName: .containerBackground))
    )
    .overlay(
      ContainerRelativeShape()
        .inset(by: -0.5)
        .stroke(
          LinearGradient(
            stops: [
              .init(color: Color(braveSystemName: .dividerSubtle), location: 0),
              .init(color: Color(braveSystemName: .dividerSubtle), location: 0.5),
              // avoid stroke at bottom of shape
              .init(color: Color(braveSystemName: .containerBackground), location: 1),
            ],
            startPoint: .top,
            endPoint: .bottom
          ),
          lineWidth: 1.0
        )
    )
    .containerShape(
      UnevenRoundedRectangle(
        topLeadingRadius: 8.0,
        bottomLeadingRadius: 0,
        bottomTrailingRadius: 0,
        topTrailingRadius: 8.0,
        style: .continuous
      )
    )
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
