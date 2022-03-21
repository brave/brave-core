/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * Value types representing a page's metadata
 */
public struct PageMetadata: Decodable {
  public let siteURL: String
  public let mediaURL: String?
  public let title: String?
  public let description: String?
  public let type: String?
  public let providerName: String?
  public let faviconURL: String?
  public let largeIconURL: String?
  public let keywords: Set<String>?
  public let search: Link?
  public let feeds: [Link]

  enum CodingKeys: String, CodingKey {
    case mediaURL = "image"
    case siteURL = "url"
    case title
    case description
    case type
    case providerName = "provider"
    case faviconURL = "icon"
    case largeIconURL = "largeIcon"
    case keywords
    case search
    case feeds
  }

  public init(
    siteURL: String,
    mediaURL: String?,
    title: String?,
    description: String?,
    type: String?,
    providerName: String?,
    faviconURL: String? = nil,
    largeIconURL: String? = nil,
    keywords: Set<String>? = nil,
    search: Link? = nil,
    feeds: [Link] = []
  ) {
    self.siteURL = siteURL
    self.mediaURL = mediaURL
    self.title = title
    self.description = description
    self.type = type
    self.providerName = providerName
    self.faviconURL = faviconURL
    self.largeIconURL = largeIconURL
    self.keywords = keywords
    self.search = search
    self.feeds = feeds
  }

  public struct Link: Decodable {
    public var href: String
    public var title: String
  }
}
