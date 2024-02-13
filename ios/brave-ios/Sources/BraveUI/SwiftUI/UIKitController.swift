/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import os.log

public struct UIKitController: View {
  private var makeVC: () -> UIViewController

  public init(_ makeVC: @escaping @autoclosure () -> UIViewController) {
    self.makeVC = makeVC
  }

  private var controller: some View {
    _UIKitController(makeVC)
  }

  public var body: some View {
    controller
      .ignoresSafeArea()
  }

  private struct _UIKitController: UIViewControllerRepresentable {
    private var makeVC: () -> UIViewController
    init(_ makeVC: @escaping () -> UIViewController) {
      self.makeVC = makeVC
    }
    func makeUIViewController(context: Context) -> UIViewController {
      makeVC()
    }
    func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    }
    static func dismantleUIViewController(_ uiViewController: UIViewController, coordinator: ()) {
      Logger.module.debug("Dismantled \(uiViewController)")
    }
  }
}
