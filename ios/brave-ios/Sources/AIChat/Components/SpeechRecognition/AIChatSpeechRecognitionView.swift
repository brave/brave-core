// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Foundation
import SpeechRecognition
import SwiftUI

struct AIChatSpeechRecognitionView: View {
  @ObservedObject
  var speechRecognizer: SpeechRecognizer

  @Binding
  var isVoiceEntryPresented: Bool

  @Binding
  var isNoMicrophonePermissionPresented: Bool

  @Binding
  var recognizedText: String

  var body: some View {
    SpeechToTextInputContentView(
      isPresented: $isVoiceEntryPresented,
      dismissAction: {
        isVoiceEntryPresented = false
      },
      speechModel: speechRecognizer,
      disclaimer: Strings.AIChat.speechRecognizerDisclaimer
    )
    .alert(isPresented: $isNoMicrophonePermissionPresented) {
      Alert(
        title: Text(Strings.AIChat.microphonePermissionAlertTitle),
        message: Text(Strings.AIChat.microphonePermissionAlertDescription),
        primaryButton: Alert.Button.default(
          Text(Strings.settings),
          action: {
            let url = URL(string: UIApplication.openSettingsURLString)!
            UIApplication.shared.open(url, options: [:], completionHandler: nil)
          }
        ),
        secondaryButton: Alert.Button.cancel(Text(Strings.CancelString))
      )
    }
    .onChange(of: speechRecognizer.finalizedRecognition) { recognition in
      if let recognition {
        // Feedback indicating recognition is finalized
        AudioServicesPlayAlertSound(SystemSoundID(kSystemSoundID_Vibrate))
        UIImpactFeedbackGenerator(style: .medium).vibrate()

        // Update Text
        recognizedText = recognition

        // Clear the SpeechRecognizer
        speechRecognizer.clearSearch()
        isVoiceEntryPresented = false
      }
    }
  }
}
