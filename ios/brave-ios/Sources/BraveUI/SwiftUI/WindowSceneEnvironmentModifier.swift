// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

extension EnvironmentValues {
  private struct WindowSceneEnvironmentKey: EnvironmentKey {
    static var defaultValue: UIWindowScene?
  }

  /// The window scene that this SwiftUI view hierarchy belongs to.
  public var windowScene: UIWindowScene? {
    self[WindowSceneEnvironmentKey.self]
  }

  /// Writable reference to `windowScene`
  fileprivate var _windowScene: UIWindowScene? {
    get { self[WindowSceneEnvironmentKey.self] }
    set { self[WindowSceneEnvironmentKey.self] = newValue }
  }
}

extension View {
  /// Allows the view access to the `windowScene` environment value.
  public func prepareWindowSceneEnvironment() -> some View {
    modifier(WindowSceneEnvironmentModifier())
  }
}

/// A modifier that injects a `UIViewControllerRepresentable` into the SwiftUI hierarchy to obtain
/// the underlying `UIWindowScene` associated with this UI and then injects that into the
/// SwiftUI environment.
private struct WindowSceneEnvironmentModifier: ViewModifier {
  @State private var windowScene: UIWindowScene?

  func body(content: Content) -> some View {
    content
      .environment(\._windowScene, windowScene)
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
      windowScene = nil
    }
  }
}
