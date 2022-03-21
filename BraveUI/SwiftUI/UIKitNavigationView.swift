// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// A view that embeds a SwiftUI hierarchy within a UINavigationController
///
/// The root view controller will be set to the SwiftUI View provided
///
/// In iOS 15 using `StackNavigationStyle` will cause issues with toolbar buttons and large title mode. In
/// those instances you can switch to use a ``UIKitNavigationView`` to fallback to UIKit.
public struct UIKitNavigationView<Content: View>: View {
  public var content: Content

  public init(@ViewBuilder content: () -> Content) {
    self.content = content()
  }

  public var body: some View {
    _UIKitNavigationView(content: content)
      .ignoresSafeArea()
  }

  private struct _UIKitNavigationView<Content: View>: UIViewControllerRepresentable {
    var content: Content

    func makeUIViewController(context: Context) -> UINavigationController {
      UINavigationController(
        rootViewController: UIHostingController(
          rootView: content
        )
      )
    }
    func updateUIViewController(_ uiViewController: UINavigationController, context: Context) {
    }
  }
}
