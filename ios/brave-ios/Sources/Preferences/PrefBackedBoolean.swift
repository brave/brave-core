// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

/// An observable Chromium boolean pref
@Observable
public class PrefBackedBoolean {
  let prefs: any PrefService
  let key: String
  let prefsChangeRegistrar: PrefChangeRegistrar

  public init(prefs: any PrefService, key: String) {
    self.prefs = prefs
    self.key = key
    self.prefsChangeRegistrar = .init(prefService: prefs)

    prefsChangeRegistrar.addObserver(
      forPath: key,
      callback: { [weak self] _ in
        self?.withMutation(keyPath: \.value, {})
      }
    )
  }

  public var value: Bool {
    get {
      access(keyPath: \.value)
      return prefs.boolean(forPath: key)
    }
    set {
      withMutation(keyPath: \.value) {
        prefs.set(newValue, forPath: key)
      }
    }
  }
}
