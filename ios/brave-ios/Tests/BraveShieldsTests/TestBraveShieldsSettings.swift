// Copyright 202t The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

/// Implementation of `BraveShieldsSettings` used for unit testing
class TestBraveShieldsSettings: BraveShieldsSettings {

  var _isBraveShieldsEnabled: ((URL) -> Bool)?
  var _setBraveShieldsEnabled: ((Bool, URL) -> Void)?

  var _defaultAdBlockMode: (() -> BraveShields.AdBlockMode)?
  var _setDefaultAdBlockMode: ((BraveShields.AdBlockMode) -> Void)?
  var _adBlockMode: ((URL) -> BraveShields.AdBlockMode)?
  var _setAdBlockMode: ((BraveShields.AdBlockMode, URL) -> Void)?

  var _isBlockScriptsEnabledByDefault: (() -> Bool)?
  var _setBlockScriptsEnabledByDefault: ((Bool) -> Void)?
  var _isBlockScriptsEnabled: ((URL) -> Bool)?
  var _setBlockScriptsEnabled: ((Bool, URL) -> Void)?

  var _defaultFingerprintMode: (() -> BraveShields.FingerprintMode)?
  var _setDefaultFingerprintMode: ((BraveShields.FingerprintMode) -> Void)?
  var _fingerprintMode: ((URL) -> BraveShields.FingerprintMode)?
  var _setFingerprintMode: ((BraveShields.FingerprintMode, URL) -> Void)?

  var _defaultAutoShredMode: (() -> BraveShields.AutoShredMode)?
  var _setDefaultAutoShredMode: ((BraveShields.AutoShredMode) -> Void)?
  var _autoShredMode: ((URL) -> BraveShields.AutoShredMode)?
  var _setAutoShredMode: ((BraveShields.AutoShredMode, URL) -> Void)?

  var _domainsWith: ((BraveShields.AutoShredMode) -> [URL])?

  // MARK: Brave ShieldsEnabled

  func isBraveShieldsEnabled(for url: URL) -> Bool {
    return _isBraveShieldsEnabled?(url) ?? true
  }
  func setBraveShieldsEnabled(_ isEnabled: Bool, for url: URL!) {
    _setBraveShieldsEnabled?(isEnabled, url)
  }

  // MARK: AdBlockMode

  var defaultAdBlockMode: BraveShields.AdBlockMode {
    get {
      _defaultAdBlockMode?() ?? .standard
    }
    set {
      _setDefaultAdBlockMode?(newValue)
    }
  }
  func adBlockMode(for url: URL) -> BraveShields.AdBlockMode {
    return _adBlockMode?(url) ?? defaultAdBlockMode
  }
  func setAdBlockMode(_ adBlockMode: BraveShields.AdBlockMode, for url: URL!) {
    _setAdBlockMode?(adBlockMode, url)
  }

  // MARK: Block Scripts

  var isBlockScriptsEnabledByDefault: Bool {
    get {
      _isBlockScriptsEnabledByDefault?() ?? false
    }
    set {
      _setBlockScriptsEnabledByDefault?(newValue)
    }
  }
  func isBlockScriptsEnabled(for url: URL) -> Bool {
    return _isBlockScriptsEnabled?(url) ?? false
  }
  func setBlockScriptsEnabled(_ isEnabled: Bool, for url: URL!) {
    _setBlockScriptsEnabled?(isEnabled, url)
  }

  // MARK: FingerprintMode

  var defaultFingerprintMode: BraveShields.FingerprintMode {
    get {
      _defaultFingerprintMode?() ?? .standardMode
    }
    set {
      _setDefaultFingerprintMode?(newValue)
    }
  }
  func fingerprintMode(for url: URL) -> BraveShields.FingerprintMode {
    return _fingerprintMode?(url) ?? defaultFingerprintMode
  }
  func setFingerprintMode(_ fingerprintMode: BraveShields.FingerprintMode, for url: URL!) {
    _setFingerprintMode?(fingerprintMode, url)
  }

  // MARK: AutoShredMode

  var defaultAutoShredMode: BraveShields.AutoShredMode {
    get {
      _defaultAutoShredMode?() ?? .never
    }
    set {
      _setDefaultAutoShredMode?(newValue)
    }
  }
  func autoShredMode(for url: URL) -> BraveShields.AutoShredMode {
    return _autoShredMode?(url) ?? defaultAutoShredMode
  }
  func setAutoShredMode(_ fingerprintMode: BraveShields.AutoShredMode, for url: URL!) {
    _setAutoShredMode?(fingerprintMode, url)
  }

  func domains(with autoShredMode: BraveShields.AutoShredMode) -> [URL] {
    return _domainsWith?(autoShredMode) ?? []
  }
}
