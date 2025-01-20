//
//  BraveSheetView.swift
//  Brave
//
//  Created by Brandon T on 2025-01-23.

import Shared
import SwiftUI

public struct DismissHandlerKey: EnvironmentKey {
  public static let defaultValue: (() -> Void)? = nil
}

extension EnvironmentValues {
  public var dismissHandler: (() -> Void)? {
    get { self[DismissHandlerKey.self] }
    set { self[DismissHandlerKey.self] = newValue }
  }
}

public struct BraveSheetViewModifier<SheetContent>: ViewModifier
where SheetContent: View {
  @Binding var isPresented: Bool
  let onDismiss: (() -> Void)?
  let content: () -> SheetContent

  public func body(content: Content) -> some View {
    content
      .background(
        BraveSheetView(
          isPresented: self.$isPresented,
          onDismiss: onDismiss,
          content: self.content
        )
      )
  }
}

extension View {
  public func braveSheet<Content>(
    isPresented: Binding<Bool>,
    onDismiss: (() -> Void)? = nil,
    @ViewBuilder content: @escaping () -> Content
  ) -> some View where Content: View {
    self.modifier(
      BraveSheetViewModifier(
        isPresented: isPresented,
        onDismiss: onDismiss,
        content: content
      )
    )
  }
}

public struct BraveSheetView<Content: View>: UIViewControllerRepresentable {
  @Binding var isPresented: Bool
  private var onDismiss: (() -> Void)?
  private var content: Content

  public init(
    isPresented: Binding<Bool>,
    onDismiss: (() -> Void)?,
    @ViewBuilder content: () -> Content
  ) {
    self._isPresented = isPresented
    self.onDismiss = onDismiss
    self.content = content()
  }

  public func makeUIViewController(context: Context) -> UIViewController {
    let controller = UIViewController()
    controller.view.backgroundColor = .clear
    return controller
  }

  public func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    if isPresented {
      guard uiViewController.presentedViewController == nil
      else {
        return
      }

      let hostingController = UIHostingController(
        rootView: content.environment(
          \.dismissHandler,
          {
            onDismiss?()
            isPresented = false
          }
        )
      )
      hostingController.view.backgroundColor = .clear
      hostingController.modalPresentationStyle = .pageSheet
      hostingController.presentationController?.delegate = context.coordinator

      if let sheet = hostingController.sheetPresentationController {
        sheet.detents = [.medium()]
        sheet.prefersGrabberVisible = true
        sheet.preferredCornerRadius = 12.0
      }

      context.coordinator.presentedViewController = hostingController

      DispatchQueue.main.async {
        if KeyboardHelper.defaultHelper.currentState != nil {
          UIApplication.shared.sendAction(
            #selector(UIResponder.resignFirstResponder),
            to: nil,
            from: nil,
            for: nil
          )

          DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            uiViewController.present(hostingController, animated: true)
          }
        } else {
          uiViewController.present(hostingController, animated: true)
        }
      }
    } else {
      if let presentedViewController = context.coordinator.presentedViewController,
        presentedViewController == uiViewController.presentedViewController
      {
        uiViewController.presentedViewController?.dismiss(animated: true) {
          context.coordinator.presentedViewController = nil
        }
      }
    }
  }

  public class Coordinator: NSObject, UIAdaptivePresentationControllerDelegate {
    @Binding var isPresented: Bool
    private var onDismiss: (() -> Void)?
    weak var presentedViewController: UIViewController?

    init(isPresented: Binding<Bool>, onDismiss: (() -> Void)? = nil) {
      self._isPresented = isPresented
      self.onDismiss = onDismiss
    }

    public func presentationControllerWillDismiss(
      _ presentationController: UIPresentationController
    ) {
      onDismiss?()
    }

    public func presentationControllerDidDismiss(_ presentationController: UIPresentationController)
    {
      isPresented = false
    }
  }

  public func makeCoordinator() -> Coordinator {
    Coordinator(isPresented: $isPresented)
  }
}
