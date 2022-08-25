// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import DesignSystem

struct WalletPromptContentView<Content, Footer>: View where Content: View, Footer: View {
  let content: () -> Content
  let buttonTitle: String
  let action: (_ proceed: Bool) -> Void
  let footer: () -> Footer
  
  init(
    buttonTitle: String,
    action: @escaping (_ proceed: Bool) -> Void,
    @ViewBuilder content: @escaping () -> Content,
    @ViewBuilder footer: @escaping () -> Footer
  ) {
    self.buttonTitle = buttonTitle
    self.action = action
    self.content = content
    self.footer = footer
  }
  
  var body: some View {
    VStack {
      content()
        .padding(.bottom)
      Button(action: { action(true) }) {
        Text(buttonTitle)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      footer()
    }
    .frame(maxWidth: .infinity)
    .padding(20)
    .overlay(
      Button(action: { action(false) }) {
        Image(systemName: "xmark")
          .padding(16)
      }
        .font(.headline)
        .foregroundColor(.gray),
      alignment: .topTrailing
    )
    .accessibilityEmbedInScrollView()
  }
}

struct WalletPromptView<Content, Footer>: UIViewControllerRepresentable where Content: View, Footer: View {
  @Binding var isPresented: Bool
  var buttonTitle: String
  var action: (Bool, UINavigationController?) -> Bool
  var content: () -> Content
  var footer: () -> Footer
  
  func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }
  
  func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      if uiViewController.presentedViewController != nil {
        return
      }
      let controller = PopupViewController(
        rootView: WalletPromptContentView(
          buttonTitle: buttonTitle,
          action: { proceed in
            if action(proceed, uiViewController.navigationController) {
              uiViewController.dismiss(animated: true) {
                isPresented = false
              }
            }
          },
          content: content,
          footer: footer
        )
      )
      uiViewController.present(controller, animated: true)
    } else {
      uiViewController.presentedViewController?.dismiss(animated: true)
    }
  }
}

extension WalletPromptView where Content: View, Footer == EmptyView {
  init(
    isPresented: Binding<Bool>,
    buttonTitle: String,
    action: @escaping (Bool, UINavigationController?) -> Bool,
    @ViewBuilder content: @escaping () -> Content
  ) {
    _isPresented = isPresented
    self.buttonTitle = buttonTitle
    self.action = action
    self.content = content
    self.footer = { EmptyView() }
  }
}

