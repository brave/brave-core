// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// An enum representing the unmodified local scripts stored in the application.
///
/// - Warning: Some of these scripts are not usable "as-is". Rather, you should be using `UserScriptType`.
enum ScriptSourceType {
  /// This script farbles certian system methods to output slightly randomized output.
  case farblingProtection
  /// This script wraps engine scripts and executes them for the correct frame
  case frameCheckWrapper
  /// Global Privacy Control script
  case gpc

  var fileName: String {
    switch self {
    case .farblingProtection: return "FarblingProtectionScript"
    case .frameCheckWrapper: return "FrameCheckWrapper"
    case .gpc: return "gpc"
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
