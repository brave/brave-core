// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

extension View {
  func menuController(prompt: String, action: @escaping () -> Void) -> some View {
    self.modifier(MenuControllerModifier(prompt: prompt, action: action))
  }
}

private struct MenuControllerModifier: ViewModifier {
  var prompt: String
  var action: () -> Void

  @State private var isPresented: Bool = false

  func body(content: Content) -> some View {
    content
      .contentShape(Rectangle())
      .onTapGesture {
        isPresented = true
      }
      .background {
        MenuControllerRepresentable(isMenuVisible: $isPresented, prompt: prompt, action: action)
      }
  }
}

private struct MenuControllerRepresentable: UIViewRepresentable {
  @Binding var isMenuVisible: Bool
  var prompt: String
  var action: () -> Void

  class _View: UIView {
    var action: () -> Void
    init(action: @escaping () -> Void) {
      self.action = action
      super.init(frame: .zero)
    }
    @objc func handleAction() {
      self.action()
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
  class Coordinator {
    @Binding var isMenuVisible: Bool
    private var observationHandle: NSObjectProtocol?
    init(isMenuVisible: Binding<Bool>) {
      self._isMenuVisible = isMenuVisible
      self.observationHandle = NotificationCenter.default.addObserver(
        forName: UIMenuController.willHideMenuNotification,
        object: nil,
        queue: .main,
        using: { [weak self] notification in
          self?.isMenuVisible = false
        }
      )
    }
  }
  func makeCoordinator() -> Coordinator {
    Coordinator(isMenuVisible: $isMenuVisible)
  }
  func makeUIView(context: Context) -> UIView {
    _View(action: action)
  }
  func updateUIView(_ uiView: UIView, context: Context) {
    if isMenuVisible {
      UIMenuController.shared.menuItems = [
        .init(title: prompt, action: #selector(_View.handleAction))
      ]
      UIMenuController.shared.showMenu(from: uiView, rect: uiView.bounds)
    } else {
      UIMenuController.shared.hideMenu(from: uiView)
    }
  }
}
