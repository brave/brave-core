// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import os.log

public struct PlaylistInfo: Codable, Identifiable, Hashable, Equatable {
  public let name: String
  public let src: String
  public let pageSrc: String
  public let pageTitle: String
  public let mimeType: String
  public let duration: TimeInterval
  public let lastPlayedOffset: TimeInterval
  public let detected: Bool
  public let dateAdded: Date
  public let tagId: String
  public let order: Int32
  public let isInvisible: Bool
  
  public var id: String {
    tagId
  }
  
  public init(pageSrc: String) {
    self.name = ""
    self.src = ""
    self.pageSrc = pageSrc
    self.pageTitle = ""
    self.mimeType = ""
    self.duration = 0.0
    self.lastPlayedOffset = 0.0
    self.dateAdded = Date()
    self.detected = false
    self.tagId = UUID().uuidString
    self.order = Int32.min
    self.isInvisible = false
  }

  public init(item: PlaylistItem) {
    self.name = item.name
    self.src = item.mediaSrc
    self.pageSrc = item.pageSrc
    self.pageTitle = item.pageTitle ?? ""
    self.mimeType = item.mimeType
    self.duration = item.duration
    self.lastPlayedOffset = item.lastPlayedOffset
    self.dateAdded = item.dateAdded
    self.detected = false
    self.tagId = item.uuid ?? UUID().uuidString
    self.order = item.order
    self.isInvisible = false
  }

  public init(name: String, src: String, pageSrc: String, pageTitle: String, mimeType: String, duration: TimeInterval, lastPlayedOffset: TimeInterval, detected: Bool, dateAdded: Date, tagId: String, order: Int32, isInvisible: Bool) {
    self.name = name
    self.src = src
    self.pageSrc = pageSrc
    self.pageTitle = pageTitle
    self.mimeType = mimeType
    self.duration = duration
    self.lastPlayedOffset = lastPlayedOffset
    self.detected = detected
    self.dateAdded = dateAdded
    self.tagId = tagId.isEmpty ? UUID().uuidString : tagId
    self.order = order
    self.isInvisible = isInvisible
  }

  public init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    self.name = try container.decode(String.self, forKey: .name)
    let src = try container.decodeIfPresent(String.self, forKey: .src) ?? ""
    self.pageSrc = try container.decode(String.self, forKey: .pageSrc)
    self.pageTitle = try container.decode(String.self, forKey: .pageTitle)
    self.mimeType = try container.decodeIfPresent(String.self, forKey: .mimeType) ?? ""
    self.duration = try container.decodeIfPresent(TimeInterval.self, forKey: .duration) ?? 0.0
    self.lastPlayedOffset = try container.decodeIfPresent(TimeInterval.self, forKey: .lastPlayedOffset) ?? 0.0
    self.detected = try container.decodeIfPresent(Bool.self, forKey: .detected) ?? false
    self.tagId = try container.decodeIfPresent(String.self, forKey: .tagId) ?? UUID().uuidString
    self.dateAdded = Date()
    self.src = PlaylistInfo.fixSchemelessURLs(src: src, pageSrc: pageSrc)
    self.order = try container.decodeIfPresent(Int32.self, forKey: .order) ?? Int32.min
    self.isInvisible = try container.decodeIfPresent(Bool.self, forKey: .isInvisible) ?? false
  }

  public static func from(message: WKScriptMessage) -> PlaylistInfo? {
    if !JSONSerialization.isValidJSONObject(message.body) {
      return nil
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body, options: [.fragmentsAllowed])
      return try JSONDecoder().decode(PlaylistInfo.self, from: data)
    } catch {
      Logger.module.error("Error Decoding PlaylistInfo: \(error.localizedDescription)")
    }

    return nil
  }
  
  public func hash(into hasher: inout Hasher) {
    hasher.combine(pageSrc.asURL?.normalizedHostAndPath ?? pageSrc)
    hasher.combine(tagId)
  }
  
  public static func == (lhs: Self, rhs: Self) -> Bool {
    if let lhsPageSrc = lhs.pageSrc.asURL?.normalizedHostAndPath, let rhsPageSrc = rhs.pageSrc.asURL?.normalizedHostAndPath {
      return lhsPageSrc == rhsPageSrc && lhs.tagId == rhs.tagId
    }
    return lhs.pageSrc == rhs.pageSrc && lhs.tagId == rhs.tagId
  }

  public static func fixSchemelessURLs(src: String, pageSrc: String) -> String {
    if src.hasPrefix("//") {
      return "\(URL(string: pageSrc)?.scheme ?? ""):\(src)"
    } else if src.hasPrefix("/"), let url = URL(string: src, relativeTo: URL(string: pageSrc))?.absoluteString {
      return url
    }
    return src
  }

  private enum CodingKeys: String, CodingKey {
    case name
    case src
    case pageSrc
    case pageTitle
    case mimeType
    case duration
    case lastPlayedOffset
    case detected
    case tagId
    case dateAdded
    case order
    case isInvisible = "invisible"
  }
}
