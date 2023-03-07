// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine

typealias NetworkSessionDataResponse = (Data, URLResponse)

protocol NetworkSession: Sendable {
  func dataRequest(with url: URL, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void)
  func dataRequest(with urlRequest: URLRequest, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void)
  
  func dataRequest(with url: URL) -> AnyPublisher<NetworkSessionDataResponse, Error>
  func dataRequest(with urlRequest: URLRequest) -> AnyPublisher<NetworkSessionDataResponse, Error>
  
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
  
  func dataRequest(with url: URL, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    self.dataTask(with: url) { data, response, error in
      if let error = error {
        completion(.failure(error))
      } else {
        completion(.success((data ?? Data(), response ?? self.createResponse(url: url))))
      }
    }.resume()
  }
  
  func dataRequest(with urlRequest: URLRequest, _ completion: @escaping (Result<NetworkSessionDataResponse, Error>) -> Void) {
    self.dataTask(with: urlRequest) { data, response, error in
      if let error = error {
        completion(.failure(error))
      } else {
        completion(.success((data ?? Data(), response ?? self.createResponse(url: urlRequest.url))))
      }
    }.resume()
  }
  
  func dataRequest(with url: URL) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    return self.dataTaskPublisher(for: url)
      .map({ ($0, $1) })
      .mapError({ $0 as Error })
      .eraseToAnyPublisher()
  }
  
  func dataRequest(with urlRequest: URLRequest) -> AnyPublisher<NetworkSessionDataResponse, Error> {
    return self.dataTaskPublisher(for: urlRequest)
      .map({ ($0, $1) })
      .mapError({ $0 as Error })
      .receive(on: DispatchQueue.main)
      .eraseToAnyPublisher()
  }

  func dataRequest(with url: URL) async throws -> NetworkSessionDataResponse {
    return try await data(from: url)
  }

  func dataRequest(with urlRequest: URLRequest) async throws -> NetworkSessionDataResponse {
    return try await data(for: urlRequest)
  }
}
