// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI

struct BiometricsPromptView: UIViewControllerRepresentable {
  @Binding var isPresented: Bool
  var action: (Bool, UINavigationController?) -> Bool
  
  func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }
  
  func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      if uiViewController.presentedViewController != nil {
        return
      }
      let controller = PopupViewController(rootView: EnableBiometricsView(action: { enabled in
        if action(enabled, uiViewController.navigationController) {
          uiViewController.dismiss(animated: true) {
            isPresented = false
          }
        }
      }))
      uiViewController.present(controller, animated: true)
    } else {
      if uiViewController.presentedViewController != nil {
        uiViewController.presentedViewController?.dismiss(animated: true)
      }
    }
  }
}

struct BiometricsPromptView_Previews: PreviewProvider {
  static var previews: some View {
    BiometricsPromptView(isPresented: .constant(true), action: { _, _ in false })
  }
}
