// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine

/// An implemntation of `NetworkSession` which is able to return dynamic mock results as defined by a completion callback
class BaseMockNetworkSession: NetworkSession, @unchecked Sendable {
  /// A completion block that returns dynamic responses based on the given URL
  var completion: @Sendable (URL) async throws -> NetworkSessionDataResponse
  
  /// Initialize this session with a completion block
  init(completion: @Sendable @escaping (URL) async throws -> NetworkSessionDataResponse) {
    self.completion = completion
  }
  
  func dataRequest(with url: URL, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    Task {
      do {
        let result = try await self.completion(url)
        completion(.success(result))
      } catch {
        completion(.failure(error))
      }
    }
  }
  
  func dataRequest(with urlRequest: URLRequest, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    guard let url = urlRequest.url else {
      completion(.failure(URLError(.badURL)))
      return
    }
    
    self.dataRequest(with: url, completion)
  }
  
  func dataRequest(with url: URL) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    Combine.Deferred {
      Future { completion in
        self.dataRequest(with: url, completion)
      }
    }.eraseToAnyPublisher()
  }
  
  func dataRequest(with urlRequest: URLRequest) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    Combine.Deferred {
      Future { completion in
        self.dataRequest(with: urlRequest, completion)
      }
    }.eraseToAnyPublisher()
  }

  func dataRequest(with url: URL) async throws -> NetworkSessionDataResponse {
    try await Task.detached(priority: .userInitiated) {
      return try await self.completion(url)
    }.value
  }

  func dataRequest(with urlRequest: URLRequest) async throws -> NetworkSessionDataResponse {
    try await Task.detached(priority: .userInitiated) {
      guard let url = urlRequest.url else {
        throw URLError(.badURL)
      }
      
      return try await self.dataRequest(with: url)
    }.value
  }
}

/// A fixed NetworkSession mock that is only able to return fixed results
class NetworkSessionMock: BaseMockNetworkSession {
  var data: Data?
  var response: URLResponse?
  var error: Error?
  
  init() {
    super.init(completion: { _ in
      preconditionFailure("Not yet initialized")
    })
    
    super.completion = { [weak self] _ in
      guard let self = self else {
        return (.init(), .init())
      }

      if let error = self.error {
        throw error
      }

      let data = self.data ?? Data()
      let response = self.response ?? HTTPURLResponse()
      return (data, response)
    }
  }
}
