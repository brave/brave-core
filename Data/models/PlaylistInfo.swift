// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

private let log = Logger.browserLogger

public struct PlaylistInfo: Codable {
    public let name: String
    public let src: String
    public let pageSrc: String
    public let pageTitle: String
    public let mimeType: String
    public let duration: TimeInterval
    public let detected: Bool
    public let dateAdded: Date
    public let tagId: String
    
    public init(item: PlaylistItem) {
        self.name = item.name ?? ""
        self.src = item.mediaSrc ?? ""
        self.pageSrc = item.pageSrc ?? ""
        self.pageTitle = item.pageTitle ?? ""
        self.mimeType = item.mimeType ?? ""
        self.duration = item.duration
        self.dateAdded = item.dateAdded ?? Date()
        self.detected = false
        self.tagId = ""
    }
    
    public init(name: String, src: String, pageSrc: String, pageTitle: String, mimeType: String, duration: TimeInterval, detected: Bool, dateAdded: Date, tagId: String) {
        self.name = name
        self.src = src
        self.pageSrc = pageSrc
        self.pageTitle = pageTitle
        self.mimeType = mimeType
        self.duration = duration
        self.detected = detected
        self.dateAdded = dateAdded
        self.tagId = tagId
    }
    
    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        self.name = try container.decode(String.self, forKey: .name)
        self.src = try container.decode(String.self, forKey: .src)
        self.pageSrc = try container.decode(String.self, forKey: .pageSrc)
        self.pageTitle = try container.decode(String.self, forKey: .pageTitle)
        self.mimeType = try container.decodeIfPresent(String.self, forKey: .mimeType) ?? ""
        self.duration = try container.decodeIfPresent(TimeInterval.self, forKey: .duration) ?? 0.0
        self.detected = try container.decodeIfPresent(Bool.self, forKey: .detected) ?? false
        self.tagId = try container.decodeIfPresent(String.self, forKey: .tagId) ?? ""
        self.dateAdded = Date()
    }
    
    public static func from(message: WKScriptMessage) -> PlaylistInfo? {
        if !JSONSerialization.isValidJSONObject(message.body) {
            return nil
        }
        
        do {
            let data = try JSONSerialization.data(withJSONObject: message.body, options: [.fragmentsAllowed])
            return try JSONDecoder().decode(PlaylistInfo.self, from: data)
        } catch {
            log.error("Error Decoding PlaylistInfo: \(error)")
        }
        
        return nil
    }
    
    private enum CodingKeys: String, CodingKey {
        case name
        case src
        case pageSrc
        case pageTitle
        case mimeType
        case duration
        case detected
        case tagId
        case dateAdded
    }
}
