// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import SwiftUI
import UIKit

public struct BravePopoverViewModifier<PopoverContent: View>: ViewModifier {
  @Binding var isPresented: Bool
  var arrowDirection: UIPopoverArrowDirection = [.any]
  let content: () -> PopoverContent

  public func body(content: Content) -> some View {
    content
      .background(
        BravePopoverView(
          isPresented: self.$isPresented,
          arrowDirection: self.arrowDirection,
          content: self.content
        )
      )
  }
}

extension View {
  public func bravePopover<Content>(
    isPresented: Binding<Bool>,
    arrowDirection: UIPopoverArrowDirection = [.any],
    @ViewBuilder content: @escaping () -> Content
  ) -> some View where Content: View {
    self.modifier(
      BravePopoverViewModifier(
        isPresented: isPresented,
        arrowDirection: arrowDirection,
        content: content
      )
    )
  }
}

public struct BravePopoverView<Content: View>: UIViewControllerRepresentable {
  @Binding var isPresented: Bool
  private var arrowDirection: UIPopoverArrowDirection
  private var content: Content

  public init(
    isPresented: Binding<Bool>,
    arrowDirection: UIPopoverArrowDirection,
    @ViewBuilder content: () -> Content
  ) {
    self._isPresented = isPresented
    self.arrowDirection = arrowDirection
    self.content = content()
  }

  public func makeUIViewController(context: Context) -> UIViewController {
    return .init()
  }

  public func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    context.coordinator.updateView(content)

    if isPresented {
      guard uiViewController.presentedViewController == nil
      else {
        DispatchQueue.main.async {
          context.coordinator.dismiss()
        }
        return
      }

      if !uiViewController.isBeingDismissed {
        context.coordinator.prepareForPresentation(
          on: uiViewController,
          arrowDirections: arrowDirection
        )

        DispatchQueue.main.async {
          if let presentedViewController = uiViewController.presentedViewController {
            presentedViewController.dismiss(animated: true) {
              context.coordinator.present(on: uiViewController)
            }
          } else {
            context.coordinator.present(on: uiViewController)
          }
        }
      }
    } else {
      if context.coordinator.isPresentedOn(uiViewController) {
        context.coordinator.dismiss()
      }
    }
  }

  public func makeCoordinator() -> Coordinator {
    Coordinator(self, content: content)
  }

  public class Coordinator: NSObject, UIPopoverPresentationControllerDelegate {
    private let parent: BravePopoverView
    private var host: UIHostingController<SelfSizingView>?

    init(_ parent: BravePopoverView, content: Content) {
      self.parent = parent
      super.init()

      host = UIHostingController(
        rootView: SelfSizingView(view: content, coordinator: .init(self))
      )
    }

    public func updateView(_ content: Content) {
      host?.rootView = SelfSizingView(view: content, coordinator: .init(self))
    }

    public func isPresentedOn(_ controller: UIViewController) -> Bool {
      return host == controller.presentedViewController
    }

    public func prepareForPresentation(
      on controller: UIViewController,
      arrowDirections: UIPopoverArrowDirection
    ) {
      guard let host = host else { return }
      host.modalPresentationStyle = .popover
      host.popoverPresentationController?.delegate = self
      host.popoverPresentationController?.sourceView = controller.view
      host.popoverPresentationController?.sourceRect = controller.view.bounds
      host.popoverPresentationController?.permittedArrowDirections = arrowDirections
    }

    public func present(on controller: UIViewController) {
      if let host = host {
        controller.present(host, animated: true)
      }
    }

    public func dismiss() {
      host?.dismiss(animated: true)
    }

    public func setPreferredContentSize(_ size: CGSize) {
      host?.preferredContentSize = size
    }

    public func popoverPresentationControllerDidDismissPopover(
      _ popoverPresentationController: UIPopoverPresentationController
    ) {
      parent.isPresented = false
    }

    public func adaptivePresentationStyle(
      for controller: UIPresentationController
    ) -> UIModalPresentationStyle {
      return .none
    }
  }

  private struct SelfSizingView: View {
    @State
    var size: CGSize = .zero

    let view: Content
    let coordinator: WeakRef<Coordinator>

    var body: some View {
      view.onGeometryChange(
        for: CGSize.self,
        of: { $0.size },
        action: {
          coordinator.wrappedValue?.setPreferredContentSize($0)
        }
      )
    }
  }
}
