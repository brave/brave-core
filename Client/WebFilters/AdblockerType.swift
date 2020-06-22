// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

enum FileType: String {
    case dat, json, tgz
}

enum AdblockerType {
    case general
    case httpse
    case regional(locale: String)
    
    var locale: String? {
        switch self {
        case .regional(let locale): return locale
        default: return nil
        }}
    
    var associatedFiles: [FileType] { return [.json, fileForStatsLibrary] }
    
    private var fileForStatsLibrary: FileType {
        switch self {
        case .general, .regional: return .dat
        case .httpse: return .tgz
        }
    }
    
    /// A name under which given resource is stored locally in the app.
    var identifier: String {
        switch self {
        case .general: return BlocklistName.ad.filename
        case .httpse: return BlocklistName.https.filename
        case .regional(let locale): return locale
        }
    }
    
    /// A name under which given resource is stored on server.
    func resourceName(for fileType: FileType) -> String? {
        switch self {
        case .general: return AdblockResourcesMappings.generalAdblockName(for: fileType)
        case .httpse: return AdblockResourcesMappings.generalHttpseName
        case .regional(let locale):
            guard let regionalName = ResourceLocale(rawValue: locale)?.resourceName(for: fileType) else {
                return nil
            }
            return "\(regionalName)-latest"
        }
    }
    
    var blockListName: BlocklistName? {
        switch self {
        case .general: return BlocklistName.ad
        case .httpse: return BlocklistName.https
        case .regional(let locale): return ContentBlockerRegion.with(localeCode: locale)
        }
    }
}
