// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import SwiftUI
import UIKit

public struct BravePopupViewModifier<PopupContent>: ViewModifier
where PopupContent: View {
  @Binding var isPresented: Bool
  let content: () -> PopupContent

  public func body(content: Content) -> some View {
    content
      .background(
        BravePopupView(
          isPresented: self.$isPresented,
          content: self.content
        )
      )
  }
}

extension View {
  public func bravePopup<Content>(
    isPresented: Binding<Bool>,
    @ViewBuilder content: @escaping () -> Content
  ) -> some View where Content: View {
    self.modifier(
      BravePopupViewModifier(
        isPresented: isPresented,
        content: content
      )
    )
  }
}

public struct BravePopupView<Content: View>: UIViewControllerRepresentable {
  @Binding var isPresented: Bool
  private var content: Content

  public init(
    isPresented: Binding<Bool>,
    @ViewBuilder content: () -> Content
  ) {
    self._isPresented = isPresented
    self.content = content()
  }

  public func makeUIViewController(context: Context) -> UIViewController {
    .init()
  }

  public func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      guard uiViewController.presentedViewController == nil
      else {
        // The system dismissed our Popup automatically, but never updated our presentation state
        // It usually does this if you present another Popup or sheet
        // Manually update it
        if let controller = context.coordinator.presentedViewController
          as? PopupViewController<Content>
        {
          DispatchQueue.main.async {
            controller.dismiss(animated: true) {
              context.coordinator.presentedViewController = nil
              self.isPresented = false
            }
          }
        } else if context.coordinator.presentedViewController != nil {
          DispatchQueue.main.async {
            isPresented = false
          }
        }
        return
      }

      if let parent = uiViewController.parent, !parent.isBeingDismissed {
        let controller = PopupViewController(rootView: content, isDismissable: true)
        context.coordinator.presentedViewController = controller

        DispatchQueue.main.async {
          if KeyboardHelper.defaultHelper.currentState != nil {
            UIApplication.shared.sendAction(
              #selector(UIResponder.resignFirstResponder),
              to: nil,
              from: nil,
              for: nil
            )

            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
              uiViewController.present(controller, animated: true)
            }
          } else {
            uiViewController.present(controller, animated: true)
          }
        }
      }
    } else {
      if let presentedViewController = context.coordinator.presentedViewController,
        presentedViewController == uiViewController.presentedViewController
      {
        uiViewController.presentedViewController?.dismiss(animated: true) {
          context.coordinator.presentedViewController = nil
          self.isPresented = false
        }
      }
    }
  }

  public class Coordinator {
    weak var presentedViewController: UIViewController?
  }

  public func makeCoordinator() -> Coordinator {
    Coordinator()
  }
}
