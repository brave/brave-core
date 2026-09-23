// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// An action that when called should handle opening a URL in the browser (optionally in private
/// mode)
///
/// Use the ``EnvironmentValues/openTabURL`` environment value to get an instance
/// of this action in the current Environment. Call the instance directly to perform the request.
///
/// For example:
///
///     @Environment(\.openTabURL) private var openTabURL
///
///     var body: some View {
///       Button {
///         openTabURL(url, isPrivateMode: true)
///       } label: {
///         Text("Open in Private Tab")
///       }
///     }
public struct OpenTabURLAction {
  private var handler: (URL, _ isPrivateMode: Bool) -> Void

  public init(handler: @escaping (URL, _ isPrivateMode: Bool) -> Void) {
    self.handler = handler
  }

  /// Calls the handler provided
  ///
  /// Do not call this method, instead use the Swift language feature to call it directly from
  /// the instance. E.g. `openTabURL(url)`
  public func callAsFunction(_ url: URL, privateMode: Bool = false) {
    handler(url, privateMode)
  }
}

// FIXME: Move to BraveUI
extension EnvironmentValues {
  private struct OpenPlaylistURLActionKey: EnvironmentKey {
    static var defaultValue: OpenTabURLAction = .init(handler: { _, _ in })
  }

  /// The action to allow you to open tabs in the browser from a SwiftUI view hierarchy
  ///
  /// - Note: This environment value will not work unless a parent View sets a `OpenTabURLAction`
  ///         for this `openTabURL` environment value.
  public var openTabURL: OpenTabURLAction {
    get { self[OpenPlaylistURLActionKey.self] }
    set { self[OpenPlaylistURLActionKey.self] = newValue }
  }
}
