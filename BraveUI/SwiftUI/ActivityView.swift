// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

public struct ActivityView: UIViewControllerRepresentable {
  @Binding public var isPresented: Bool
  public var activityItems: [Any]

  public init(isPresented: Binding<Bool>, activityItems: [Any]) {
    self._isPresented = isPresented
    self.activityItems = activityItems
  }

  public func makeUIViewController(context: UIViewControllerRepresentableContext<ActivityView>) -> UIViewController {
    return UIViewController()
  }

  public func updateUIViewController(_ uiViewController: UIViewController, context: UIViewControllerRepresentableContext<ActivityView>) {
    if isPresented {
      if uiViewController.presentedViewController != nil {
        return
      }
      let alert = UIActivityViewController(activityItems: activityItems, applicationActivities: nil)
      alert.completionWithItemsHandler = { _, _, _, _ in
        self.isPresented = false
      }
      uiViewController.present(alert, animated: true, completion: nil)
    } else {
      uiViewController.dismiss(animated: true, completion: nil)
    }
  }
}
