// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit

extension EnvironmentValues {
  private struct InterfaceOrientationKey: EnvironmentKey {
    static var defaultValue: UIInterfaceOrientation = .unknown
  }

  /// The interface orientation of this environment
  ///
  /// The value will be the current orientation of the View that reads it, regardless of horizontal
  /// and vertical size classes. Use this in conjunction with size classes to update UI accordingly.
  ///
  /// Use size classes if possible before reaching for this API.
  ///
  /// - Note: This environment value will be `UIInterfaceOrientation.unknown` and receive no changes
  ///         unless a parent View uses the `observingInterfaceOrientation` modifier.
  var interfaceOrientation: UIInterfaceOrientation {
    self[InterfaceOrientationKey.self]
  }

  /// Writable reference to `interfaceOrientation`
  ///
  /// - SeeAlso: `interfaceOrientation`
  fileprivate var _interfaceOrientation: UIInterfaceOrientation {
    get { self[InterfaceOrientationKey.self] }
    set { self[InterfaceOrientationKey.self] = newValue }
  }
}

extension View {
  /// Allows the view access to the `interfaceOrientation` environement value
  ///
  /// View's will receive a `interfaceOrientation` value when the View changes orientations even
  /// if the size class does not change such as when you rotate an non-Max/Plus iPhone to landscape.
  func observingInterfaceOrientation() -> some View {
    modifier(InterfaceOrientationViewModifier())
  }
}

private struct InterfaceOrientationViewModifier: ViewModifier {
  @State private var orientation: UIInterfaceOrientation = .unknown

  func body(content: Content) -> some View {
    content
      .environment(\._interfaceOrientation, orientation)
      .background {
        _Representable(orientation: $orientation)
          // Can't use the `hidden` modifier or the VC isn't added at all and can't receive updates
          .opacity(0)
          .accessibilityHidden(true)
      }
  }

  private struct _Representable: UIViewControllerRepresentable {
    @Binding var orientation: UIInterfaceOrientation

    func makeUIViewController(context: Context) -> some UIViewController {
      _RepresentableViewController(orientation: $orientation)
    }

    func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
    }
  }

  private class _RepresentableViewController: UIViewController {
    @Binding var orientation: UIInterfaceOrientation

    init(orientation: Binding<UIInterfaceOrientation>) {
      self._orientation = orientation
      super.init(nibName: nil, bundle: nil)
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    private func updateOrientation() {
      orientation = view.window?.windowScene?.interfaceOrientation ?? .unknown
    }

    override func viewIsAppearing(_ animated: Bool) {
      super.viewIsAppearing(animated)
      updateOrientation()
    }

    override func viewWillTransition(
      to size: CGSize,
      with coordinator: UIViewControllerTransitionCoordinator
    ) {
      super.viewWillTransition(to: size, with: coordinator)
      updateOrientation()
    }
  }
}
