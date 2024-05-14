// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Preferences
import Shared
import SwiftUI

public struct SpeechToTextInputView: View {
  @Environment(\.presentationMode) @Binding private var presentationMode
  @ObservedObject var speechModel: SpeechRecognizer
  let disclaimer: String

  var onEnterSearchKeyword: (() -> Void)?
  var dismiss: (() -> Void)?

  public init(speechModel: SpeechRecognizer, disclaimer: String, dismissAction: (() -> Void)? = nil)
  {
    self.speechModel = speechModel
    self.disclaimer = disclaimer
    self.dismiss = dismissAction
  }

  public var body: some View {
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
      Text(disclaimer)
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

  private func dismissView() {
    dismiss?()
    speechModel.clearSearch()
    presentationMode.dismiss()
  }
}

extension SpeechToTextInputView {

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

extension UIColor {
  fileprivate static var microphoneBackground: UIColor {
    UIColor(rgb: 0x423eee)
  }
}

public struct SpeechToTextInputContentView: UIViewControllerRepresentable {
  @Binding var isPresented: Bool

  var dismissAction: (() -> Void)?

  var speechModel: SpeechRecognizer
  var disclaimer: String

  public init(
    isPresented: Binding<Bool>,
    dismissAction: (() -> Void)? = nil,
    speechModel: SpeechRecognizer,
    disclaimer: String
  ) {
    _isPresented = isPresented
    self.dismissAction = dismissAction
    self.speechModel = speechModel
    self.disclaimer = disclaimer
  }

  public func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }

  public func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      if uiViewController.presentedViewController != nil {
        return
      }

      let controller = PopupViewController(
        rootView: SpeechToTextInputView(
          speechModel: speechModel,
          disclaimer: disclaimer,
          dismissAction: dismissAction
        )
      )

      context.coordinator.presentedViewController = .init(controller)
      uiViewController.present(controller, animated: true)
    } else {
      if let presentedViewController = context.coordinator.presentedViewController?.value,
        presentedViewController == uiViewController.presentedViewController
      {
        uiViewController.presentedViewController?.dismiss(animated: true)
      }
    }
  }

  public class Coordinator {
    var presentedViewController: WeakRef<UIViewController>?
  }

  public func makeCoordinator() -> Coordinator {
    Coordinator()
  }
}
