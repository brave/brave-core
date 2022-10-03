// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// An enum representing the unmodified local scripts stored in the application.
///
/// - Warning: Some of these scripts are not usable "as-is". Rather, you should be using `UserScriptType`.
enum ScriptSourceType {
  /// A script that informs iOS of site state changes
  case siteStateListener
  /// A simple encryption library found here:
  /// https://www.npmjs.com/package/tweetnacl
  case nacl
  /// This script farbles certian system methods to output slightly randomized output.
  /// This script has a dependency on `nacl`.
  case farblingProtection
  case braveSearchHelper
  case braveTalkHelper
  case braveSkus
  case bravePlaylistFolderSharingHelper
  case youtubeAdblock

  var fileName: String {
    switch self {
    case .siteStateListener: return "SiteStateListenerScript"
    case .nacl: return "nacl.min"
    case .farblingProtection: return "FarblingProtectionScript"
    case .braveSearchHelper: return "BraveSearchScript"
    case .braveTalkHelper: return "BraveTalkScript"
    case .bravePlaylistFolderSharingHelper: return "PlaylistFolderSharingScript"
    case .braveSkus: return "BraveSkusScript"
    case .youtubeAdblock: return "YoutubeAdblock"
    }
  }

  func loadScript() throws -> String {
    guard let path = Bundle.current.path(forResource: fileName, ofType: "js") else {
      assertionFailure("Cannot load script. This should not happen as it's part of the codebase")
      throw ScriptLoadFailure.notFound
    }

    return try String(contentsOfFile: path)
  }
}
