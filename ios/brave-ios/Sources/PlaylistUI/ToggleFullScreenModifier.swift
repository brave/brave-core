// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// An action that updates the current full screen layout mode in the playlist environment
///
/// Use the ``EnvironmentValues/toggleFullScreen`` environment value to get an instance
/// of this action in the current Environment. Call the instance directly to perform the request.
///
/// For example:
///
///     @Environment(\.toggleFullScreen) private var toggleFullScreen
///
///     var body: some View {
///       Button {
///         toggleFullScreen(explicitFullScreenMode: true)
///       } label: {
///         Text("Enter Full Screen Mode")
///       }
///     }
struct ToggleFullScreenAction {
  @Binding fileprivate var isFullScreen: Bool

  /// Toggles the `isFullScreen` environment value (or explicitly sets it to a given value)
  ///
  /// Do not call this method, instead use the Swift language feature to call it directly from
  /// the instance. E.g. `toggleFullScreen()`
  func callAsFunction(explicitFullScreenMode: Bool? = nil) {
    if let explicitFullScreenMode {
      isFullScreen = explicitFullScreenMode
    } else {
      isFullScreen.toggle()
    }
  }
}

extension EnvironmentValues {
  private struct IsFullScreenKey: EnvironmentKey {
    static var defaultValue: Bool = false
  }

  /// Whether or not playlist is currently presenting its selected item in a full screen layout
  ///
  /// - Note: This enviroment value will be always set to false unless a parent View uses the
  ///         `prepareFullScreenEnvironment` modifier.
  var isFullScreen: Bool {
    self[IsFullScreenKey.self]
  }

  /// Writeable reference to `isFullScreen`
  fileprivate var _isFullScreen: Bool {
    get { self[IsFullScreenKey.self] }
    set { self[IsFullScreenKey.self] = newValue }
  }

  private struct ToggleFullScreenActionKey: EnvironmentKey {
    static var defaultValue: ToggleFullScreenAction = .init(isFullScreen: .constant(false))
  }
  /// The action to allow you to toggle full screen layout mode
  ///
  /// - Note: This environment value will not work unless a parent View uses
  ///         the `prepareFullScreenEnvironment` modifier.
  var toggleFullScreen: ToggleFullScreenAction {
    self[ToggleFullScreenActionKey.self]
  }

  /// Writeable reference to `toggleFullScreen`
  fileprivate var _toggleFullScreen: ToggleFullScreenAction {
    get { self[ToggleFullScreenActionKey.self] }
    set { self[ToggleFullScreenActionKey.self] = newValue }
  }
}

/// A simple modifier which holds onto the current full screen state
private struct ToggleFullScreenModifier: ViewModifier {
  @State private var isFullScreen: Bool = false
  func body(content: Content) -> some View {
    content
      .environment(\._isFullScreen, isFullScreen)
      .environment(\._toggleFullScreen, .init(isFullScreen: $isFullScreen))
  }
}

extension View {
  /// Prepares the SwiftUI environment so that children can use the `isFullScreen` and
  /// `toggleFullScreen` environment values
  func prepareFullScreenEnvironment() -> some View {
    modifier(ToggleFullScreenModifier())
  }
}
