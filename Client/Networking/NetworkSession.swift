// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

typealias NetworkSessionDataResponse = (Data, URLResponse)

protocol NetworkSession {
  func dataRequest(with url: URL) async throws -> NetworkSessionDataResponse
  func dataRequest(with urlRequest: URLRequest) async throws -> NetworkSessionDataResponse
}

extension URLSession: NetworkSession {
  private func createResponse(code: Int = 200, url: URL?) -> URLResponse {
    guard let url = url else {
      return URLResponse()
    }

    return HTTPURLResponse(
      url: url,
      statusCode: code,
      httpVersion: "HTTP/1.1",
      headerFields: [:])!
  }

  func dataRequest(with url: URL) async throws -> NetworkSessionDataResponse {
    if #available(iOS 15, *) {
      return try await data(from: url)
    } else {
      return try await withCheckedThrowingContinuation { continuation in
        self.dataTask(with: url) { data, response, error in
          if let error = error {
            continuation.resume(throwing: error)
          } else {
            continuation.resume(with: .success((data ?? Data(), response ?? self.createResponse(url: url))))
          }
        }.resume()
      }
    }
  }

  func dataRequest(with urlRequest: URLRequest) async throws -> NetworkSessionDataResponse {
    if #available(iOS 15, *) {
      return try await data(for: urlRequest)
    } else {
      return try await withCheckedThrowingContinuation { continuation in
        self.dataTask(with: urlRequest) { data, response, error in
          if let error = error {
            continuation.resume(throwing: error)
          } else {
            continuation.resume(with: .success((data ?? Data(), response ?? self.createResponse(url: urlRequest.url))))
          }
        }.resume()
      }
    }
  }
}
