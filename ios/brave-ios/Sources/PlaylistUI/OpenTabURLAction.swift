// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

// FIXME: Add doc
// FIXME: Maybe make this shared somewhere
public struct OpenTabURLAction {
  private var handler: (URL, _ isPrivateMode: Bool) -> Void

  public init(handler: @escaping (URL, _ isPrivateMode: Bool) -> Void) {
    self.handler = handler
  }

  public func callAsFunction(_ url: URL, privateMode: Bool = false) {
    handler(url, privateMode)
  }
}

extension EnvironmentValues {
  private struct OpenPlaylistURLActionKey: EnvironmentKey {
    static var defaultValue: OpenTabURLAction = .init(handler: { _, _ in })
  }

  public var openTabURL: OpenTabURLAction {
    get { self[OpenPlaylistURLActionKey.self] }
    set { self[OpenPlaylistURLActionKey.self] = newValue }
  }
}
