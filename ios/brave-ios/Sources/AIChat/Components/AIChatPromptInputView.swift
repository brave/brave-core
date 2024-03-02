// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem
import SpeechRecognition
import AVFoundation

struct AIChatPromptInputView: View {
  @ObservedObject
  var model: AIChatSpeechRecognitionModel
  
  var onSubmit: (String) -> Void

  @State 
  private var prompt: String = ""

  var body: some View {
    HStack(spacing: 0.0) {
      TextField(
        Strings.AIChat.promptPlaceHolderDescription,
        text: $prompt,
        prompt: Text(Strings.AIChat.promptPlaceHolderDescription)
          .font(.subheadline)
          .foregroundColor(Color(braveSystemName: .textTertiary))
      )
      .font(.subheadline)
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .submitLabel(.send)
      .onSubmit {
        if !prompt.isEmpty {
          onSubmit(prompt)
          prompt = ""
        }
      }
      .padding(.leading)
      
      if prompt.isEmpty {
        Button {
          Task { @MainActor in
            await model.activateVoiceRecognition()
          }
        } label: {
          Image(braveSystemName: "leo.microphone")
            .foregroundStyle(Color(braveSystemName: .iconDefault))
            .padding()
        }
        .opacity(model.speechRecognizer.isVoiceSearchAvailable ? 1.0 : 0.0)
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
        .shadow(color: .black.opacity(0.15), radius: 4.0, x: 0.0, y: 1.0)
    )
    .overlay(
      ContainerRelativeShape()
        .strokeBorder(Color(braveSystemName: .dividerStrong), lineWidth: 1.0)
    )
    .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
    .onReceive(model.speechRecognizer.$finalizedRecognition) { recognition in
      if recognition.status && model.activeInputView == .promptView {
        // Feedback indicating recognition is finalized
        AudioServicesPlayAlertSound(SystemSoundID(kSystemSoundID_Vibrate))
        UIImpactFeedbackGenerator(style: .medium).bzzt()
        
        // Update Prompt
        prompt = recognition.searchQuery
        
        // Clear the SpeechRecognizer
        model.speechRecognizer.clearSearch()
        model.isVoiceEntryPresented = false
        model.activeInputView = .none
      }
    }
  }
}

#if DEBUG
struct AIChatPromptInputView_Preview: PreviewProvider {
  static var previews: some View {
    AIChatPromptInputView(
      model: AIChatSpeechRecognitionModel(
        speechRecognizer: SpeechRecognizer(),
        activeInputView: .constant(.none),
        isVoiceEntryPresented: .constant(false),
        isNoMicrophonePermissionPresented: .constant(false)
      ),
      onSubmit: {
        print("Prompt Submitted: \($0)")
      }
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
