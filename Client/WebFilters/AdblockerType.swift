// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

enum FileType: String {
  case dat, json
}

enum AdblockerType {
  case general
  case regional(locale: String)
  var associatedFiles: [FileType] { return [.json, fileForStatsLibrary] }

  private var fileForStatsLibrary: FileType {
    switch self {
    case .general, .regional: return .dat
    }
  }

  /// A name under which given resource is stored locally in the app.
  var identifier: String {
    switch self {
    case .general: return BlocklistName.ad.filename
    case .regional(let locale): return locale
    }
  }

  /// A name under which given resource is stored on server.
  func resourcePath(for fileType: FileType) -> String? {
    switch self {
    case .general:
      switch fileType {
      // We take the same regional file as desktop/android
      case .dat: return "/4/rs-ABPFilterParserData.dat"
      // This file contains the regional content blocking json format (only iOS)
      case .json: return "/ios/latest.json"
      }
    case .regional(let locale):
      guard let regionalName = ResourceLocale(rawValue: locale)?.resourceName(for: fileType) else {
        return nil
      }
      
      switch fileType {
      // We take the same regional file as desktop/android
      case .dat: return "/4/rs-\(regionalName).dat"
      // This file contains the regional content blocking json format (only iOS)
      case .json: return "/ios/\(regionalName)-latest.json"
      }
    }
  }

  var blockListName: BlocklistName? {
    switch self {
    case .general: return BlocklistName.ad
    case .regional(let locale): return ContentBlockerRegion.with(localeCode: locale)
    }
  }
}
