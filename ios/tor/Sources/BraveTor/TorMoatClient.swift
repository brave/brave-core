// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Foundation

public struct TorBridgeChallenge: Sendable {
  public let image: Data
  fileprivate let token: String
}

/// Explicit bootstrap requests only. This session never loads browsing content.
public actor TorMoatClient {
  private let session: URLSession
  public init() {
    let configuration = URLSessionConfiguration.ephemeral
    configuration.httpCookieStorage = nil
    configuration.httpShouldSetCookies = false
    configuration.urlCache = nil
    configuration.requestCachePolicy = .reloadIgnoringLocalCacheData
    configuration.timeoutIntervalForRequest = 30
    configuration.timeoutIntervalForResource = 45
    session = URLSession(configuration: configuration, delegate: MoatRedirectPolicy(), delegateQueue: nil)
  }

  public func challenge() async throws -> TorBridgeChallenge {
    try Self.decodeChallenge(await post("fetch", item: [
      "type": "client-transports", "version": "0.1.0", "supported": ["obfs4"]
    ]))
  }

  public func bridges(challenge: TorBridgeChallenge, solution: String) async throws -> [String] {
    guard !solution.isEmpty, solution.utf8.count <= 256 else { throw TorBridgeError.invalidResponse }
    return try Self.decodeBridges(await post("check", item: [
      "id": "2", "version": "0.1.0", "qrcode": "false",
      "type": "moat-solution", "transport": "obfs4",
      "challenge": challenge.token, "solution": solution
    ]))
  }

  private func post(_ endpoint: String, item: [String: Any]) async throws -> Data {
    var request = URLRequest(url: URL(string: "https://bridges.torproject.org/moat/\(endpoint)")!)
    request.httpMethod = "POST"
    request.setValue("application/vnd.api+json", forHTTPHeaderField: "Content-Type")
    request.setValue("LVOippNS8UiKLH6kXf1D8pI1clLc", forHTTPHeaderField: "shim-token")
    request.httpBody = try JSONSerialization.data(withJSONObject: ["data": [item]])
    let (bytes, response) = try await session.bytes(for: request)
    guard let response = response as? HTTPURLResponse, (200..<300).contains(response.statusCode),
      response.url?.host == "bridges.torproject.org",
      response.expectedContentLength <= 262_144
    else { throw TorBridgeError.invalidResponse }
    var data = Data()
    for try await byte in bytes {
      try Task.checkCancellation()
      guard data.count < 262_144 else { throw TorBridgeError.invalidResponse }
      data.append(byte)
    }
    return data
  }

  private static func item(_ data: Data) throws -> [String: Any] {
    guard data.count <= 262_144,
      let root = try JSONSerialization.jsonObject(with: data) as? [String: Any],
      root["errors"] == nil,
      let items = root["data"] as? [[String: Any]], let item = items.first
    else { throw TorBridgeError.invalidResponse }
    return item
  }

  static func decodeChallenge(_ data: Data) throws -> TorBridgeChallenge {
    let item = try item(data)
    guard let encoded = item["image"] as? String, let image = Data(base64Encoded: encoded),
      !image.isEmpty, image.count <= 196_608,
      let token = item["challenge"] as? String, !token.isEmpty, token.utf8.count <= 8192
    else { throw TorBridgeError.invalidResponse }
    return TorBridgeChallenge(image: image, token: token)
  }

  static func decodeBridges(_ data: Data) throws -> [String] {
    let item = try item(data)
    guard let lines = item["bridges"] as? [String], !lines.isEmpty, lines.count <= 64 else {
      throw TorBridgeError.invalidResponse
    }
    try lines.forEach {
      try TorBridgeLine.validate($0)
      guard TorBridgeLine.transport($0) == "obfs4" else { throw TorBridgeError.unsupportedTransport }
    }
    return lines
  }
}

private final class MoatRedirectPolicy: NSObject, URLSessionTaskDelegate {
  func urlSession(_ session: URLSession, task: URLSessionTask,
                  willPerformHTTPRedirection response: HTTPURLResponse, newRequest request: URLRequest,
                  completionHandler: @escaping (URLRequest?) -> Void) {
    completionHandler(nil)
  }
}
