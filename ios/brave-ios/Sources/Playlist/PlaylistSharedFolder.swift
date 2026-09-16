// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CodableHelpers
import Data
import Foundation

public struct PlaylistSharedFolderModel: Decodable {
  public let version: String
  public let folderId: String
  public let folderName: String
  @URLString public private(set) var folderImage: URL?
  public let creatorName: String
  @URLString public private(set) var creatorLink: URL?
  public let updateAt: String
  public fileprivate(set) var folderUrl: String?
  public fileprivate(set) var eTag: String?
  public fileprivate(set) var mediaItems: [PlaylistInfo]

  public init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    version = try container.decode(String.self, forKey: .version)
    folderId = try container.decode(String.self, forKey: .folderId)
    folderName = try container.decode(String.self, forKey: .folderName)
    _folderImage = try container.decode(URLString.self, forKey: .folderImage)
    creatorName = try container.decode(String.self, forKey: .creatorName)
    _creatorLink = try container.decode(URLString.self, forKey: .creatorLink)
    updateAt = try container.decode(String.self, forKey: .updateAt)
    mediaItems = try container.decode([MediaItem].self, forKey: .mediaItems).map { item in
      PlaylistInfo(
        name: item.title,
        src: item.url.absoluteString,
        pageSrc: item.url.absoluteString,
        pageTitle: item.title,
        mimeType: "video",
        duration: 0.0,
        lastPlayedOffset: 0.0,
        detected: true,
        dateAdded: Date(),
        tagId: item.mediaItemId,
        order: Int32(item.order) ?? -1,
        isInvisible: false
      )
    }.sorted(by: { $0.order < $1.order })
  }

  private struct MediaItem: Codable {

    let mediaItemId: String
    let title: String
    let url: URL
    let order: String

    private enum CodingKeys: String, CodingKey {
      case mediaItemId = "mediaitemid"
      case title
      case url
      case order
    }
  }

  private enum CodingKeys: String, CodingKey {
    case version
    case folderId = "folderid"
    case folderName = "foldername"
    case folderImage = "folderimage"
    case creatorName = "creatorname"
    case creatorLink = "creatorlink"
    case updateAt = "updateat"
    case mediaItems = "mediaitems"
  }
}
