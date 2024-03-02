// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import SpeechRecognition

enum AIChatSpeechRecognitionActiveView {
  case none
  case promptView
  case feedbackView
}

class AIChatSpeechRecognitionModel: ObservableObject {
  @ObservedObject
  var speechRecognizer: SpeechRecognizer
  
  @Binding
  var activeInputView: AIChatSpeechRecognitionActiveView
  
  @Binding
  var isVoiceEntryPresented: Bool
  
  @Binding
  var isNoMicrophonePermissionPresented: Bool
  
  init(speechRecognizer: SpeechRecognizer, activeInputView: Binding<AIChatSpeechRecognitionActiveView>, isVoiceEntryPresented: Binding<Bool>, isNoMicrophonePermissionPresented: Binding<Bool>) {
    self.speechRecognizer = speechRecognizer
    self._activeInputView = activeInputView
    self._isVoiceEntryPresented = isVoiceEntryPresented
    self._isNoMicrophonePermissionPresented = isNoMicrophonePermissionPresented
  }
  
  @MainActor
  func activateVoiceRecognition() async {
    let permissionStatus = await speechRecognizer.askForUserPermission()
    if permissionStatus {
      isVoiceEntryPresented = true
      activeInputView = .promptView
    } else {
      isNoMicrophonePermissionPresented = true
      activeInputView = .none
    }
  }
}
