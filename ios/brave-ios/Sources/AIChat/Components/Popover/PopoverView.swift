// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import SwiftUI
import UIKit

struct BravePopoverViewModifier<PopoverContent>: ViewModifier
where PopoverContent: View & PopoverContentComponent {
  @Binding var isPresented: Bool
  var arrowDirection: PopoverController.ArrowDirectionBehavior = .automatic
  let content: () -> PopoverContent

  func body(content: Content) -> some View {
    content
      .background(
        BravePopoverView(
          isPresented: self.$isPresented,
          arrowDirection: arrowDirection,
          content: self.content
        )
      )
  }
}

extension View {
  func bravePopover<Content>(
    isPresented: Binding<Bool>,
    arrowDirection: PopoverController.ArrowDirectionBehavior = .automatic,
    @ViewBuilder content: @escaping () -> Content
  ) -> some View where Content: View & PopoverContentComponent {
    self.modifier(
      BravePopoverViewModifier(
        isPresented: isPresented,
        arrowDirection: arrowDirection,
        content: content
      )
    )
  }
}

struct BravePopoverView<Content: View & PopoverContentComponent>: UIViewControllerRepresentable {
  @Binding var isPresented: Bool

  private var arrowDirection: PopoverController.ArrowDirectionBehavior
  private var content: Content

  init(
    isPresented: Binding<Bool>,
    arrowDirection: PopoverController.ArrowDirectionBehavior,
    @ViewBuilder content: () -> Content
  ) {
    self._isPresented = isPresented
    self.arrowDirection = arrowDirection
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
        controller.arrowDirectionBehavior = arrowDirection
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
        presentedViewController == uiViewController.presentedViewController
      {
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
