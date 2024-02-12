// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import Shared
import Preferences
import Data

struct VoiceSearchInputView: View {
  @Environment(\.presentationMode) @Binding private var presentationMode
  @ObservedObject var speechModel: SpeechRecognizer

  var onEnterSearchKeyword: (() -> Void)?

  private func dismissView() {
    presentationMode.dismiss()
  }
  
  var body: some View {
    inputView
  }
  
  private var inputView: some View {
    VStack {
      HStack {
        Spacer()
        Button {
          dismissView()
        } label: {
          Image(systemName: "xmark")
            .foregroundColor(Color(.bravePrimary))
            .font(.system(.body))
        }
      }
      transcriptView
      Text(Strings.VoiceSearch.screenDisclaimer)
        .font(.footnote)
        .multilineTextAlignment(.center)
        .foregroundColor(Color(.secondaryBraveLabel))
        .padding(.horizontal, 25)
        .padding(.bottom, 25)
    }
    .onAppear {
      speechModel.startTranscribing()
      speechModel.startSilenceAnimation()
    }.onDisappear {
      speechModel.stopTranscribing()
    }
    .padding()
    .background(Color(.secondaryBraveBackground).ignoresSafeArea())
  }
    
  private var transcriptView: some View {
    VStack {
      microphoneView
      Text(speechModel.transcript)
        .multilineTextAlignment(.center)
        .padding(.horizontal, 25)
    }
    .padding(.bottom, 25)
  }
  
  private var microphoneView: some View {
    ZStack {
      Circle()
        .foregroundColor(Color(.braveDarkerBlurple).opacity(0.20))
        .frame(width: 150, height: 150, alignment: .center)
        .scaleEffect(outerCircleScale)
        .animation(outerCircleAnimation, value: outerCircleScale)
      Circle()
        .foregroundColor(Color(.braveDarkerBlurple).opacity(0.5))
        .frame(width: 100, height: 100, alignment: .center)
      Button {
        onEnterSearchKeyword?()
        dismissView()
      } label: {
        Circle()
          .foregroundColor(Color(.microphoneBackground))
          .frame(width: 75, height: 75, alignment: .center)
      }
      Image(braveSystemName: speechModel.transcriptedIcon)
        .renderingMode(.template)
        .font(.title)
        .aspectRatio(contentMode: .fit)
        .frame(width: 35, height: 35)
        .foregroundColor(.white)
    }
    .padding(.vertical, 45)
  }
}

extension VoiceSearchInputView {
    
  private var outerCircleScale: CGFloat {
    switch speechModel.animationType {
    case .pulse(let scale):
      return scale
    case .speech(let volume):
      return volume
    case .stable:
      return 0
    }
  }
  
  private var outerCircleAnimation: Animation {
    switch speechModel.animationType {
    case .pulse:
      return .easeInOut(duration: 1.5).repeatForever()
    case .speech:
      return .linear(duration: 0.1)
    case .stable:
      return .linear(duration: 0)
    }
  }
}

private extension UIColor {
  static var microphoneBackground: UIColor {
    UIColor(rgb: 0x423eee)
  }
}
