// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine

class NetworkSessionMock: NetworkSession {
  var data: Data?
  var response: URLResponse?
  var error: Error?
  
  func dataRequest(with url: URL, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    if let error = self.error {
      completion(.failure(error))
      return
    }

    let data = self.data ?? Data()
    let response = self.response ?? HTTPURLResponse()
    completion(.success((data, response)))
  }
  
  func dataRequest(with urlRequest: URLRequest, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    if let error = self.error {
      completion(.failure(error))
      return
    }

    let data = self.data ?? Data()
    let response = self.response ?? HTTPURLResponse()
    completion(.success((data, response)))
  }
  
  func dataRequest(with url: URL) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    return Future { completion in
      if let error = self.error {
        completion(.failure(error))
        return
      }

      let data = self.data ?? Data()
      let response = self.response ?? HTTPURLResponse()
      completion(.success((data, response)))
    }.eraseToAnyPublisher()
  }
  
  func dataRequest(with urlRequest: URLRequest) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    return Future { completion in
      if let error = self.error {
        completion(.failure(error))
        return
      }

      let data = self.data ?? Data()
      let response = self.response ?? HTTPURLResponse()
      completion(.success((data, response)))
    }.eraseToAnyPublisher()
  }

  func dataRequest(with url: URL) async throws -> NetworkSessionDataResponse {
    try await Task.detached(priority: .userInitiated) {
      if let error = self.error {
        throw error
      }

      let data = self.data ?? Data()
      let response = self.response ?? HTTPURLResponse()
      return (data, response)
    }.value
  }

  func dataRequest(with urlRequest: URLRequest) async throws -> NetworkSessionDataResponse {
    try await Task.detached(priority: .userInitiated) {
      if let error = self.error {
        throw error
      }

      let data = self.data ?? Data()
      let response = self.response ?? HTTPURLResponse()
      return (data, response)
    }.value
  }
}
