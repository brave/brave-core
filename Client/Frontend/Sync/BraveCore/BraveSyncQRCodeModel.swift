// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import Shared
import BraveCore

private let log = Logger.braveCoreLogger

enum BraveSyncQRCodeError {
    case none
    case insecure
    case newerVersion
    case olderVersion
    case invalidFormat
    case expired
    case futureDate
}

struct BraveSyncQRCodeModel: Codable {
    let version: Int
    let syncHexCode: String
    let notValidAfter: Int64
    
    /// The currently supported version of QR code
    static let currentlySupportedVersion = 2
    
    /// The duration for which a QR code is valid
    static let validityDuration = 30.minutes
    
    /// The duration for which a QR code is invalid when generated too far into the future
    static let futureValidityDuration = 60.minutes
    
    /// TimeInterval of BraveSync Epoch
    /// 2021-11-02T00:00:00+0000 [yyyy-MM-dd'T'hh:mm:ssZ]
    static let braveSyncEpoch = 1635811200
    
    /// TimeInterval of Brave Sync Sunset
    /// 2022-01-01T00:00:00+0000 [yyyy-MM-dd'T'hh:mm:ssZ]
    static let sunsetDate = 1640995200
    
    init(syncHexCode: String) {
        self.version = BraveSyncQRCodeModel.currentlySupportedVersion
        self.syncHexCode = syncHexCode
        self.notValidAfter = Int64(Date().addingTimeInterval(BraveSyncQRCodeModel.validityDuration).timeIntervalSince1970)
    }
    
    init(version: Int, syncHexCode: String, notValidAfter: TimeInterval) {
        self.version = version
        self.syncHexCode = syncHexCode
        self.notValidAfter = Int64(notValidAfter)
    }
    
    private enum CodingKeys: String, CodingKey {
        case version
        case syncHexCode = "sync_code_hex"
        case notValidAfter = "not_after"
    }
    
    var jsonData: Data? {
        do {
            return try JSONEncoder().encode(self)
        } catch {
            log.error("Error converting QR Code to JSON: \(error)")
        }
        return nil
    }
    
    var jsonString: String? {
        if let data = jsonData {
            return String(data: data, encoding: .utf8)
        }
        return nil
    }
    
    static func from(string: String) -> BraveSyncQRCodeModel? {
        if let data = string.data(using: .utf8) {
            do {
                return try JSONDecoder().decode(Self.self, from: data)
            } catch {
                log.error("Error converting QR Code to BraveSyncQRCodeModel: \(error)")
            }
        }
        return nil
    }
    
    func validate(syncAPI: BraveSyncAPI) -> BraveSyncQRCodeError {
        // Older verison of the QR code (not backwards compatible)
        if version < BraveSyncQRCodeModel.currentlySupportedVersion {
            return .insecure
        }
        
        // Newer version of the QR code (not supported)
        if version > BraveSyncQRCodeModel.currentlySupportedVersion {
            return .newerVersion
        }
        
        // Currently supported version
        if notValidAfter < BraveSyncQRCodeModel.braveSyncEpoch {
            // Impossible to have a QR code of this version
            // before the brave sync epoch
            return .invalidFormat
        }
        
        // Date validation
        let minDate = Date().timeIntervalSince1970
        let maxDate = Date().timeIntervalSince1970 + BraveSyncQRCodeModel.futureValidityDuration
        
        // Currently Expired (QR code date is too far into the past)
        if TimeInterval(notValidAfter) <= minDate {
            return .expired
        }
        
        // Too future (QR code date date is too far into the future)
        if TimeInterval(notValidAfter) >= maxDate {
            return .futureDate
        }
        
        // Invalid empty sync code
        if syncHexCode.isEmpty {
            return .invalidFormat
        }
        
        // Invalid hex sync code
        if syncHexCode.filter(\.isHexDigit).count != syncHexCode.count {
            return .invalidFormat
        }
        
        // Invalid sync code validation
        let codeWords = syncAPI.syncCode(fromHexSeed: syncHexCode)
        if codeWords.isEmpty {
            return .invalidFormat
        }
        
        let codeWordsCount = codeWords.separatedBy(" ").count
        if codeWordsCount != 24 {
            return .invalidFormat
        }
        
        return .none
    }
}
