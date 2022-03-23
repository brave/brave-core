// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

class NetworkManager {
  private let session: NetworkSession

  init(session: NetworkSession = URLSession.shared) {
    self.session = session
  }

  func dataRequest(with url: URL) async throws -> NetworkSessionDataResponse {
    try await session.dataRequest(with: url)
  }

  func dataRequest(with urlRequest: URLRequest) async throws -> NetworkSessionDataResponse {
    try await session.dataRequest(with: urlRequest)
  }

  /// - parameter checkLastServerSideModification: If true, the `CachedNetworkResource` will contain a timestamp
  /// when the file was last time modified on the server.
  func downloadResource(
    with url: URL,
    resourceType: NetworkResourceType = .regular,
    retryTimeout: TimeInterval? = 60,
    checkLastServerSideModification: Bool = false,
    customHeaders: [String: String] = [:]
  ) async throws -> CachedNetworkResource {
    var request = URLRequest(url: url)

    // Makes the request conditional, returns 304 if Etag value did not change.
    let ifNoneMatchHeader = "If-None-Match"
    let fileNotModifiedStatusCode = 304

    // Identifier for a specific version of a resource for a HTTP request
    let etagHeader = "Etag"

    switch resourceType {
    case .cached(let etag):
      let requestEtag = etag ?? UUID().uuidString

      // This cache policy is required to support `If-None-Match` header.
      request.cachePolicy = .reloadIgnoringLocalAndRemoteCacheData
      request.addValue(requestEtag, forHTTPHeaderField: ifNoneMatchHeader)
    default: break
    }

    customHeaders.forEach {
      request.addValue($0.value, forHTTPHeaderField: $0.key)
    }

    let (data, response) = try await task(for: request, retryTimeout: retryTimeout)
    guard let response = response as? HTTPURLResponse else {
      throw "Invalid Response Type (Not HTTPURLResponse)"
    }

    switch response.statusCode {
    case 400...499:
      let error = """
        Failed to download, status code: \(response.statusCode),\
        URL:\(String(describing: response.url))
        """
      throw error

    case fileNotModifiedStatusCode:
      throw "File not modified"

    default:
      let responseEtag = resourceType.isCached() ? response.allHeaderFields[etagHeader] as? String : nil

      var lastModified: TimeInterval?

      if checkLastServerSideModification,
        let lastModifiedHeaderValue = response.allHeaderFields["Last-Modified"] as? String
      {
        let formatter = DateFormatter().then {
          $0.timeZone = TimeZone(abbreviation: "GMT")
          $0.dateFormat = "EEE, dd MMM yyyy HH:mm:ss zzz"
          $0.locale = Locale(identifier: "en_US")
        }

        lastModified = formatter.date(from: lastModifiedHeaderValue)?.timeIntervalSince1970
      }

      return CachedNetworkResource(
        data: data,
        etag: responseEtag,
        lastModifiedTimestamp: lastModified)
    }
  }

  private func task(for request: URLRequest, retryTimeout: TimeInterval?) async throws -> NetworkSessionDataResponse {
    if let retryTimeout = retryTimeout {
      return try await Task.retry(retryCount: 3, retryDelay: retryTimeout) {
        try await self.dataRequest(with: request)
      }.value
    }

    return try await self.dataRequest(with: request)
  }
}
