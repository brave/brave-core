// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Combine
import Dispatch
import os.log

enum NetworkManagerError: Error {
  /// The eTag in the response headers matched one on file
  case fileNotModified
}

final class NetworkManager: Sendable {
  private let session: NetworkSession

  init(session: NetworkSession = URLSession.shared) {
    self.session = session
  }
  
  func dataRequest(with url: URL, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    session.dataRequest(with: url, completion)
  }
  
  func dataRequest(with urlRequest: URLRequest, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    session.dataRequest(with: urlRequest, completion)
  }
  
  func dataRequest(with url: URL) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    session.dataRequest(with: url)
  }
  
  func dataRequest(with urlRequest: URLRequest) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    session.dataRequest(with: urlRequest)
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
    customHeaders: [String: String] = [:],
    _ completion: @escaping (Result<CachedNetworkResource, Error>) -> Void
  ) {
    var cancellable: AnyCancellable?
    cancellable = self.downloadResource(with: url,
                                        resourceType: resourceType,
                                        retryTimeout: retryTimeout,
                                        checkLastServerSideModification: checkLastServerSideModification,
                                        customHeaders: customHeaders)
      .sink(receiveCompletion: { res in
        if case .failure(let error) = res {
          completion(.failure(error))
        }
        _ = cancellable // To silence warning
        cancellable = nil
      }, receiveValue: { res in
        completion(.success(res))
      })
  }
  
  /// - parameter checkLastServerSideModification: If true, the `CachedNetworkResource` will contain a timestamp
  /// when the file was last time modified on the server.
  func downloadResource(
    with url: URL,
    resourceType: NetworkResourceType = .regular,
    retryTimeout: TimeInterval? = 60,
    checkLastServerSideModification: Bool = false,
    customHeaders: [String: String] = [:]
  ) -> AnyPublisher<CachedNetworkResource, Error> {
    let request = createDownloadRequest(with: url,
                                        resourceType: resourceType,
                                        customHeaders: customHeaders)
    
    if let retryTimeout = retryTimeout {
      return dataRequest(with: request)
        .retry(3, delay: .seconds(retryTimeout), scheduler: DispatchQueue.main)
        .tryMap({ try self.createDownloadResponse(resourceType: resourceType,
                                                  checkLastServerSideModification: checkLastServerSideModification,
                                                  data: $0, response: $1) })
        .eraseToAnyPublisher()
    }
    
    return dataRequest(with: request)
      .tryMap({ try self.createDownloadResponse(resourceType: resourceType,
                                                checkLastServerSideModification: checkLastServerSideModification,
                                                data: $0, response: $1) })
      .eraseToAnyPublisher()
    
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
    let request = createDownloadRequest(with: url,
                                        resourceType: resourceType,
                                        customHeaders: customHeaders)

    let (data, response) = try await task(for: request, retryTimeout: retryTimeout)
    return try createDownloadResponse(resourceType: resourceType,
                                      checkLastServerSideModification:
                                        checkLastServerSideModification,
                                      data: data,
                                      response: response)
  }
}

extension NetworkManager {
  private func task(for request: URLRequest, retryTimeout: TimeInterval?) async throws -> NetworkSessionDataResponse {
    if let retryTimeout = retryTimeout {
      return try await Task.retry(retryCount: 3, retryDelay: retryTimeout) {
        try await self.dataRequest(with: request)
      }.value
    }

    return try await self.dataRequest(with: request)
  }
  
  private func createDownloadRequest(with url: URL,
                                     resourceType: NetworkResourceType,
                                     customHeaders: [String: String]) -> URLRequest {
    var request = URLRequest(url: url)

    // Makes the request conditional, returns 304 if Etag value did not change.
    let ifNoneMatchHeader = "If-None-Match"

    switch resourceType {
    case .cached(let etag):
      // This cache policy is required to support `If-None-Match` header.
      request.cachePolicy = .reloadIgnoringLocalAndRemoteCacheData
      request.addValue(etag ?? UUID().uuidString, forHTTPHeaderField: ifNoneMatchHeader)
    default: break
    }

    customHeaders.forEach {
      request.addValue($0.value, forHTTPHeaderField: $0.key)
    }
    return request
  }
  
  private func createDownloadResponse(resourceType: NetworkResourceType,
                                      checkLastServerSideModification: Bool,
                                      data: Data,
                                      response: URLResponse) throws -> CachedNetworkResource {
    let fileNotModifiedStatusCode = 304

    guard let response = response as? HTTPURLResponse else {
      throw URLError(.badServerResponse)
    }

    switch response.statusCode {
    case 400...499:
      let error = """
        Failed to download, status code: \(response.statusCode),\
        URL:\(response.url?.absoluteString ?? "nil")
        """
      Logger.module.error("\(error)")
      throw URLError(.badServerResponse)

    case fileNotModifiedStatusCode:
      throw NetworkManagerError.fileNotModified

    default:
      let responseEtag = resourceType.isCached() ? response.value(forHTTPHeaderField: "ETag") : nil

      var lastModified: TimeInterval?

      if checkLastServerSideModification,
         let lastModifiedHeaderValue = response.value(forHTTPHeaderField: "Last-Modified") {
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
}

private extension Publisher {
    func retry<S>(_ retries: Int,
                  delay: S.SchedulerTimeType.Stride,
                  scheduler: S) -> AnyPublisher<Output, Failure> where S: Scheduler {
      self.catch { error in
        Future { completion in
          scheduler.schedule(after: scheduler.now.advanced(by: delay)) {
            completion(.failure(error))
          }
        }
      }
      .retry(retries)
      .eraseToAnyPublisher()
    }
}
