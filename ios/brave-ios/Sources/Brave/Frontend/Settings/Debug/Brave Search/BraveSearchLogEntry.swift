// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

class BraveSearchLogEntry: ObservableObject {
  static let shared = BraveSearchLogEntry()

  init(isEnabled: Bool = false, logs: [FallbackLogEntry] = []) {
    self.isEnabled = isEnabled
    self.logs = logs
  }

  @Published var isEnabled = false {
    didSet {
      if !isEnabled {
        logs.removeAll()
      }
    }
  }

  @Published var logs: [FallbackLogEntry] = []

  struct FallbackLogEntry: Identifiable {
    let id = UUID()

    var date: Date
    let url: URL
    var query: String
    var cookies: [HTTPCookie]
    var canAnswerTime: String?
    var backupQuery: String?
    var fallbackTime: String?
    var fallbackData: Data?
  }
}
