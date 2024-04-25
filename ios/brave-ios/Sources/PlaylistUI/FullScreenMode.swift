// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

// FIXME: Add doc
struct ToggleFullScreenAction {
  @Binding fileprivate var isFullScreen: Bool

  func callAsFunction(explicitFullScreenMode: Bool? = nil) {
    if let explicitFullScreenMode {
      isFullScreen = explicitFullScreenMode
    } else {
      isFullScreen.toggle()
    }
  }
}

extension EnvironmentValues {
  // FIXME: Add doc
  var isFullScreen: Bool {
    self[IsFullScreenKey.self]
  }
  // FIXME: Add doc
  var toggleFullScreen: ToggleFullScreenAction {
    self[ToggleFullScreenActionKey.self]
  }

  fileprivate var _isFullScreen: Bool {
    get { self[IsFullScreenKey.self] }
    set { self[IsFullScreenKey.self] = newValue }
  }

  fileprivate var _toggleFullScreen: ToggleFullScreenAction {
    get { self[ToggleFullScreenActionKey.self] }
    set { self[ToggleFullScreenActionKey.self] = newValue }
  }

  private struct IsFullScreenKey: EnvironmentKey {
    static var defaultValue: Bool = false
  }

  private struct ToggleFullScreenActionKey: EnvironmentKey {
    static var defaultValue: ToggleFullScreenAction = .init(isFullScreen: .constant(false))
  }
}

private struct ToggleFullScreenModifier: ViewModifier {
  @State private var isFullScreen: Bool = false
  func body(content: Content) -> some View {
    content
      .environment(\._isFullScreen, isFullScreen)
      .environment(\._toggleFullScreen, .init(isFullScreen: $isFullScreen))
  }
}

extension View {
  // FIXME: Better naming
  func setUpFullScreenEnvironment() -> some View {
    modifier(ToggleFullScreenModifier())
  }
}
