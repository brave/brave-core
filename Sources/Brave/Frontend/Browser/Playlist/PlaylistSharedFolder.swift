// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Data
import CoreData
import CodableHelpers

struct PlaylistSharedFolderModel: Decodable {
  let version: String
  let folderId: String
  let folderName: String
  @URLString private(set) var folderImage: URL?
  let creatorName: String
  @URLString private(set) var creatorLink: URL?
  let updateAt: String
  fileprivate(set) var folderUrl: String?
  fileprivate(set) var eTag: String?
  fileprivate(set) var mediaItems: [PlaylistInfo]
  
  init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    version = try container.decode(String.self, forKey: .version)
    folderId = try container.decode(String.self, forKey: .folderId)
    folderName = try container.decode(String.self, forKey: .folderName)
    _folderImage = try container.decode(URLString.self, forKey: .folderImage)
    creatorName = try container.decode(String.self, forKey: .creatorName)
    _creatorLink = try container.decode(URLString.self, forKey: .creatorLink)
    updateAt = try container.decode(String.self, forKey: .updateAt)
    mediaItems = try container.decode([MediaItem].self, forKey: .mediaItems).map { item in
      PlaylistInfo(name: item.title,
                   src: item.url.absoluteString,
                   pageSrc: item.url.absoluteString,
                   pageTitle: item.title,
                   mimeType: "video",
                   duration: 0.0,
                   lastPlayedOffset: 0.0,
                   detected: true,
                   dateAdded: Date(),
                   tagId: item.mediaItemId,
                   order: Int32(item.order) ?? -1)
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

struct PlaylistSharedFolderNetwork {
  enum Status: String, Error {
    case invalidURL
    case invalidResponse
    case cacheNotModified
  }
  
  @MainActor
  static func fetchPlaylist(folderUrl: String) async throws -> PlaylistSharedFolderModel {
    guard let playlistURL = URL(string: folderUrl)?.appendingPathComponent("playlist").appendingPathExtension("json") else {
      throw Status.invalidURL
    }
    
    let authenticator = BasicAuthCredentialsManager(for: Array(DomainUserScript.bravePlaylistFolderSharingHelper.associatedDomains))
    let session = URLSession(configuration: .ephemeral, delegate: authenticator, delegateQueue: .main)
    defer { session.finishTasksAndInvalidate() }
    
    func executeRequest(method: String, headers: [String: String]) async throws -> (Data, HTTPURLResponse) {
      var request = URLRequest(url: playlistURL)
      request.httpMethod = method
      headers.forEach({ request.setValue($0.value, forHTTPHeaderField: $0.key) })
      
      let (data, response) = try await NetworkManager(session: session).dataRequest(with: request)
      guard let response = response as? HTTPURLResponse,
                response.statusCode == 304 || response.statusCode >= 200 || response.statusCode <= 299 else {
        throw Status.invalidResponse
      }
      
      if response.statusCode == 304 {
        throw Status.cacheNotModified
      }
      
      return (data, response)
    }
    
    if let eTag = PlaylistFolder.getSharedFolder(sharedFolderUrl: folderUrl)?.sharedFolderETag {
      _ = try await executeRequest(method: "HEAD", headers: ["If-None-Match": eTag])
    }
    
    let (data, response) = try await executeRequest(method: "GET", headers: [:])
    var model = try JSONDecoder().decode(PlaylistSharedFolderModel.self, from: data)
    model.folderUrl = folderUrl
    model.eTag = response.allHeaderFields["ETag"] as? String
    return model
  }
  
  @MainActor
  static func createInMemoryStorage(for model: PlaylistSharedFolderModel) async -> PlaylistFolder {
    await withCheckedContinuation { continuation in
      // Create a local shared folder
      PlaylistFolder.addInMemoryFolder(title: model.folderName,
                                       creatorName: model.creatorName,
                                       creatorLink: model.creatorLink?.absoluteString,
                                       sharedFolderId: model.folderId,
                                       sharedFolderUrl: model.folderUrl,
                                       sharedFolderETag: model.eTag) { folder, folderId in
        // Add the items to the folder
        PlaylistItem.addInMemoryItems(model.mediaItems, folderUUID: folderId) {
          // Items were added
          continuation.resume(returning: folder)
        }
      }
    }
  }
  
  @MainActor
  static func saveToDiskStorage(memoryFolder: PlaylistFolder) async -> String {
    await withCheckedContinuation({ continuation in
      PlaylistFolder.saveInMemoryFolderToDisk(folder: memoryFolder) { folderId in
        PlaylistItem.saveInMemoryItemsToDisk(items: Array(memoryFolder.playlistItems ?? []), folderUUID: folderId) {
          continuation.resume(returning: folderId)
        }
      }
    })
  }
  
  static func fetchMediaItemInfo(item: PlaylistSharedFolderModel, viewForInvisibleWebView: UIView) async throws -> [PlaylistInfo] {
    @Sendable @MainActor
    func fetchTask(item: PlaylistInfo) async throws -> PlaylistInfo {
      var webLoader: PlaylistWebLoader?
      webLoader = PlaylistWebLoader().then {
        viewForInvisibleWebView.insertSubview($0, at: 0)
      }
      
      return try await withTaskCancellationHandler(operation: {
        return try await withCheckedThrowingContinuation { continuation in
          if let url = URL(string: item.pageSrc) {
            DispatchQueue.main.async {
              webLoader?.load(url: url) { newItem in
                if let newItem = newItem {
                  PlaylistManager.shared.getAssetDuration(item: newItem) { duration in
                    let item = PlaylistInfo(name: item.name,
                                       src: newItem.src,
                                       pageSrc: newItem.pageSrc,
                                       pageTitle: item.pageTitle,
                                       mimeType: newItem.mimeType,
                                       duration: duration ?? newItem.duration,
                                       lastPlayedOffset: 0.0,
                                       detected: newItem.detected,
                                       dateAdded: newItem.dateAdded,
                                       tagId: item.tagId,
                                       order: item.order)
                    
                    // Destroy the web loader when the callback is complete.
                    webLoader?.removeFromSuperview()
                    webLoader = nil
                    continuation.resume(returning: item)
                  }
                } else {
                  // Destroy the web loader when the callback is complete.
                  webLoader?.removeFromSuperview()
                  webLoader = nil
                  continuation.resume(returning: item)
                }
              }
            }
          } else {
            Task { @MainActor in
              webLoader?.stop()
              webLoader?.removeFromSuperview()
              webLoader = nil
            }
          }
        }
      }, onCancel: {
        Task { @MainActor in
          webLoader?.stop()
          webLoader?.removeFromSuperview()
          webLoader = nil
        }
      })
      
    }

    return try await withThrowingTaskGroup(of: PlaylistInfo.self, returning: [PlaylistInfo].self) { group in
      try item.mediaItems.forEach { item in
        try Task.checkCancellation()
        
        group.addTask {
          return try await fetchTask(item: item)
        }
      }
      
      var result = [PlaylistInfo]()
      for try await value in group {
        result.append(value)
      }
      return result
    }
  }
}
