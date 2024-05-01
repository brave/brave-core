// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit
import os

/// An action that requests a geometry update on the parent window scene to force an interface
/// orientation change.
///
/// Use the ``EnvironmentValues/requestGeometryUpdate`` environment value to get an instance
/// of this action in the current Environment. Call the instance directly to perform the request.
///
/// For example:
///
///     @Environment(\.requestGeometryUpdate) private var requestGeometryUpdate
///
///     var body: some View {
///       Button {
///         requestGeometryUpdate(orientation: .portrait)
///       } label: {
///         Text("Request Portrait Mode")
///       }
///     }
///
/// - Note: This action will fail if used on iPad when multitasking is enabled so consider feature
///         blocking to only run when the interface idiom is `phone`.
@available(iOS 16.0, *)
struct RequestGeometryUpdateAction {
  // FIXME: May have to be weak
  fileprivate var windowScene: UIWindowScene?

  /// Calls `requestGeometryUpdate` on the associated `UIWindowScene`.
  ///
  /// Do not call this method, instead use the Swift language feature to call it directly from
  /// the instance. E.g. `requestGeometryUpdate(orientation: .portrait)`
  func callAsFunction(orientation: UIInterfaceOrientation) {
    let mask: UIInterfaceOrientationMask =
      switch orientation {
      case .portrait: .portrait
      case .portraitUpsideDown: .portraitUpsideDown
      case .landscapeLeft: .landscapeLeft
      case .landscapeRight: .landscapeRight
      case .unknown: []
      @unknown default: []
      }
    windowScene?.requestGeometryUpdate(.iOS(interfaceOrientations: mask)) { error in
      // This method will fail with an error when you attempt to request an orientation change
      // while in split view/slide over windowing states. `UIWindowScene.isFullScreen` is
      // unfortunately not usable to check, as its always false for some reason.
      let log = Logger(
        subsystem: Bundle.main.bundleIdentifier!,
        category: "RequestGeometryUpdateAction"
      )
      log.warning(
        "Geometry update request failed: \(error.localizedDescription) (\((error as NSError).code, privacy: .public))"
      )
    }
  }
}

@available(iOS 16.0, *)
extension EnvironmentValues {
  private struct RequestGeometryUpdateActionKey: EnvironmentKey {
    static var defaultValue: RequestGeometryUpdateAction = .init()
  }

  /// The action to allow you to request geometry updates to the window scene of this environment
  ///
  /// - Note: This environment value will not work unless a parent View uses
  ///         the `creatingRequestGeometryUpdateAction` modifier.
  var requestGeometryUpdate: RequestGeometryUpdateAction {
    self[RequestGeometryUpdateActionKey.self]
  }

  /// Writable reference to `requestGeometryUpdate`
  fileprivate var _requestGeometryUpdate: RequestGeometryUpdateAction {
    get { self[RequestGeometryUpdateActionKey.self] }
    set { self[RequestGeometryUpdateActionKey.self] = newValue }
  }
}

@available(iOS 16.0, *)
extension View {
  /// Allows the view access to the `requestGeometryUpdate` environement value
  func creatingRequestGeometryUpdateAction() -> some View {
    modifier(RequestGeometryUpdateModifier())
  }
}

// FIXME: Consider combining InterfaceOrientationModifier and RequestGeometryUpdateModifier
// Could use `UIWindowScene.effectiveGeometry` to fetch the interface orientation which is KVO-compliant
@available(iOS 16.0, *)
private struct RequestGeometryUpdateModifier: ViewModifier {
  // FIXME: May have to be a weak box
  @State private var windowScene: UIWindowScene?

  func body(content: Content) -> some View {
    content
      .environment(\._requestGeometryUpdate, .init(windowScene: windowScene))
      .background {
        _Representable(windowScene: $windowScene)
          // Can't use the `hidden` modifier or the VC isn't added at all and can't receive updates
          .opacity(0)
          .accessibilityHidden(true)
      }
  }

  private struct _Representable: UIViewControllerRepresentable {
    @Binding var windowScene: UIWindowScene?

    func makeUIViewController(context: Context) -> some UIViewController {
      _RepresentableViewController(windowScene: $windowScene)
    }

    func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
    }
  }

  private class _RepresentableViewController: UIViewController {
    @Binding var windowScene: UIWindowScene?

    init(windowScene: Binding<UIWindowScene?>) {
      self._windowScene = windowScene
      super.init(nibName: nil, bundle: nil)
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    override func viewIsAppearing(_ animated: Bool) {
      super.viewIsAppearing(animated)
      windowScene = view.window?.windowScene
    }

    override func viewDidDisappear(_ animated: Bool) {
      super.viewDidDisappear(animated)
      // FIXME: Probably redundant
      windowScene = nil
    }
  }
}
