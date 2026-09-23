// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit

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

  final class _View: UIView, UIEditMenuInteractionDelegate {
    var prompt: String = ""
    var action: () -> Void
    var onDismiss: (() -> Void)?
    private var editMenuInteraction: UIEditMenuInteraction?

    init(action: @escaping () -> Void) {
      self.action = action
      super.init(frame: .zero)
      let interaction = UIEditMenuInteraction(delegate: self)
      addInteraction(interaction)
      editMenuInteraction = interaction
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    func presentMenu() {
      let configuration = UIEditMenuConfiguration(
        identifier: nil,
        sourcePoint: CGPoint(x: bounds.midX, y: bounds.midY)
      )
      editMenuInteraction?.presentEditMenu(with: configuration)
    }

    func dismissMenu() {
      editMenuInteraction?.dismissMenu()
    }

    func editMenuInteraction(
      _ interaction: UIEditMenuInteraction,
      menuFor configuration: UIEditMenuConfiguration,
      suggestedActions: [UIMenuElement]
    ) -> UIMenu? {
      UIMenu(children: [
        UIAction(title: prompt) { [weak self] _ in
          self?.action()
        }
      ])
    }

    func editMenuInteraction(
      _ interaction: UIEditMenuInteraction,
      willDismissMenuFor configuration: UIEditMenuConfiguration,
      animator: any UIEditMenuInteractionAnimating
    ) {
      onDismiss?()
    }
  }

  class Coordinator {
    @Binding var isMenuVisible: Bool
    init(isMenuVisible: Binding<Bool>) {
      self._isMenuVisible = isMenuVisible
    }
    func menuDidEnd() {
      isMenuVisible = false
    }
  }

  func makeCoordinator() -> Coordinator {
    Coordinator(isMenuVisible: $isMenuVisible)
  }

  func makeUIView(context: Context) -> _View {
    let view = _View(action: action)
    view.onDismiss = { context.coordinator.menuDidEnd() }
    return view
  }

  func updateUIView(_ uiView: _View, context: Context) {
    uiView.prompt = prompt
    uiView.action = action
    if isMenuVisible {
      uiView.presentMenu()
    } else {
      uiView.dismissMenu()
    }
  }
}
