// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
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
public struct RequestGeometryUpdateAction {
  fileprivate var windowScene: UIWindowScene?

  /// Calls `requestGeometryUpdate` on the associated `UIWindowScene`.
  ///
  /// Do not call this method, instead use the Swift language feature to call it directly from
  /// the instance. E.g. `requestGeometryUpdate(orientation: .portrait)`
  public func callAsFunction(orientation: UIInterfaceOrientation) {
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

extension EnvironmentValues {
  private struct RequestGeometryUpdateActionKey: EnvironmentKey {
    static var defaultValue: RequestGeometryUpdateAction = .init()
  }

  /// The action to allow you to request geometry updates to the window scene of this environment
  ///
  /// - Note: This environment value will not work unless a parent View uses
  ///         the `prepareEffectiveGeometryEnvironment` modifier.
  public var requestGeometryUpdate: RequestGeometryUpdateAction {
    self[RequestGeometryUpdateActionKey.self]
  }

  /// Writable reference to `requestGeometryUpdate`
  fileprivate var _requestGeometryUpdate: RequestGeometryUpdateAction {
    get { self[RequestGeometryUpdateActionKey.self] }
    set { self[RequestGeometryUpdateActionKey.self] = newValue }
  }

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
  ///         unless a parent View uses the `prepareEffectiveGeometryEnvironment` modifier.
  public var interfaceOrientation: UIInterfaceOrientation {
    self[InterfaceOrientationKey.self]
  }

  /// Writable reference to `interfaceOrientation`
  fileprivate var _interfaceOrientation: UIInterfaceOrientation {
    get { self[InterfaceOrientationKey.self] }
    set { self[InterfaceOrientationKey.self] = newValue }
  }
}

extension View {
  /// Allows the view access to the `requestGeometryUpdate` and `interfaceOrientation` environment
  /// values.
  public func prepareEffectiveGeometryEnvironment() -> some View {
    self
      .modifier(EffectiveGeometryEnvironmentModifier())
      .prepareWindowSceneEnvironment()
  }
}

/// A modifier that uses this hierarchies UIWindowScene to inject effective gemoetry related values
/// into the SwiftUI environment.
private struct EffectiveGeometryEnvironmentModifier: ViewModifier {
  @Environment(\.windowScene) private var windowScene
  @State private var orientation: UIInterfaceOrientation = .unknown

  private var interfaceOrientationPublisher: some Publisher<UIInterfaceOrientation, Never> {
    if let windowScene {
      return windowScene.publisher(for: \.effectiveGeometry).map(\.interfaceOrientation)
        .eraseToAnyPublisher()
    }
    return Just(UIInterfaceOrientation.unknown).eraseToAnyPublisher()
  }

  func body(content: Content) -> some View {
    content
      .environment(\._requestGeometryUpdate, .init(windowScene: windowScene))
      .environment(\._interfaceOrientation, orientation)
      .onReceive(interfaceOrientationPublisher) { orientation in
        self.orientation = orientation
      }
  }
}
