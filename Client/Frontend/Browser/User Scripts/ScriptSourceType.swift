// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
  /// A YouTube ad blocking script
  case youtubeAdBlock
  case archive
  case braveSearchHelper
  case braveTalkHelper
  case bravePlaylistFolderSharingHelper

  var fileName: String {
    switch self {
    case .nacl: return "nacl.min"
    case .farblingProtection: return "FarblingProtection"
    case .youtubeAdBlock: return "YoutubeAdblock"
    case .archive: return "ArchiveIsCompat"
    case .braveSearchHelper: return "BraveSearchHelper"
    case .braveTalkHelper: return "BraveTalkHelper"
    case .bravePlaylistFolderSharingHelper: return "PlaylistFolderSharing"
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
