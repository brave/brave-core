// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SwiftUI
import BraveUI
import Shared

struct BravePopoverViewModifier<PopoverContent>: ViewModifier where PopoverContent: View & PopoverContentComponent {
  @Binding var isPresented: Bool
  let content: () -> PopoverContent
  
  func body(content: Content) -> some View {
    content
      .background(
        BravePopoverView(
          isPresented: self.$isPresented,
          content: self.content
        )
      )
  }
}

extension View {
  func bravePopover<Content>(
    isPresented: Binding<Bool>,
    @ViewBuilder content: @escaping () -> Content
  ) -> some View where Content: View & PopoverContentComponent {
    self.modifier(
      BravePopoverViewModifier(
        isPresented: isPresented,
        content: content
      )
    )
  }
}

struct BravePopoverView<Content: View & PopoverContentComponent>: UIViewControllerRepresentable {
  @Binding var isPresented: Bool
  private var content: Content
  
  init(isPresented: Binding<Bool>, @ViewBuilder content: () -> Content) {
    self._isPresented = isPresented
    self.content = content()
  }
  
  func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }
  
  func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      guard uiViewController.presentedViewController == nil
      else {
        // The system dismissed our popover automatically, but never updated our presentation state
        // It usually does this if you present another popover or sheet
        // Manually update it
        if context.coordinator.presentedViewController != nil {
          DispatchQueue.main.async {
            isPresented = false
          }
        }
        return
      }

      if let parent = uiViewController.parent, !parent.isBeingDismissed {
        let controller = PopoverController(content: content)
        context.coordinator.presentedViewController = .init(controller)
        controller.popoverDidDismiss = { _ in
          self.isPresented = false
        }

        DispatchQueue.main.async {
          if KeyboardHelper.defaultHelper.currentState != nil {
            UIApplication.shared.sendAction(
              #selector(UIResponder.resignFirstResponder),
              to: nil,
              from: nil,
              for: nil
            )

            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
              controller.present(from: uiViewController.view, on: parent)
            }
          } else {
            controller.present(from: uiViewController.view, on: parent)
          }
        }
      }
    } else {
      if let presentedViewController = context.coordinator.presentedViewController?.value,
         presentedViewController == uiViewController.presentedViewController {
        uiViewController.presentedViewController?.dismiss(animated: true)
      }
    }
  }
  
  class Coordinator {
    var presentedViewController: WeakRef<UIViewController>?
  }
  
  func makeCoordinator() -> Coordinator {
    Coordinator()
  }
}
