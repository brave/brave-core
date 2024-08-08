// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// An enum representing the unmodified local scripts stored in the application.
///
/// - Warning: Some of these scripts are not usable "as-is". Rather, you should be using `UserScriptType`.
enum ScriptSourceType {
  /// A simple encryption library found here:
  /// https://www.npmjs.com/package/tweetnacl
  case nacl
  /// This script farbles certian system methods to output slightly randomized output.
  /// This script has a dependency on `nacl`.
  case farblingProtection
  /// This script wraps engine scripts and executes them for the correct frame
  case frameCheckWrapper
  /// A script that polls selectors from a frame and sends it to iOS which then returns the hidden elements
  ///
  /// This script is a modification of the android and desktop script found here:
  /// https://github.com/brave/brave-core/blob/master/components/cosmetic_filters/resources/data/content_cosmetic.ts
  case selectorsPoller
  /// Global Privacy Control script
  case gpc
  /// Used in conjunction with selectorsPoller to provide procedural filters support
  case proceduralFilters

  var fileName: String {
    switch self {
    case .nacl: return "nacl.min"
    case .farblingProtection: return "FarblingProtectionScript"
    case .frameCheckWrapper: return "FrameCheckWrapper"
    case .selectorsPoller: return "SelectorsPollerScript"
    case .gpc: return "gpc"
    case .proceduralFilters: return "ProceduralFilters"
    }
  }

  func loadScript() throws -> String {
    guard let path = Bundle.module.path(forResource: fileName, ofType: "js") else {
      assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
      throw ScriptLoadFailure.notFound
    }

    return try String(contentsOfFile: path)
  }
}
