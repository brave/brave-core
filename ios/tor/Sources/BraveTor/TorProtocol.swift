// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Foundation

public enum TorError: Error, Equatable {
  case unavailable
  case controlClosed
  case controlTimeout
  case malformedReply
  case rejectedCommand
  case invalidEndpoint
  case invalidDestination
}

public struct TorEndpoint: Equatable, Sendable {
  public let port: UInt16

  public init(listener: String) throws {
    // Accept only one numeric loopback endpoint; never resolve a proxy hostname.
    let value = listener.trimmingCharacters(in: .whitespacesAndNewlines)
    let address = value.hasPrefix("\"") && value.hasSuffix("\"")
      ? String(value.dropFirst().dropLast()) : value
    let parts = address.split(separator: ":", omittingEmptySubsequences: false)
    guard parts.count == 2, parts[0] == "127.0.0.1",
      let port = UInt16(parts[1]), port != 0
    else {
      throw TorError.invalidEndpoint
    }
    self.port = port
  }
}

struct TorReply: Equatable {
  let code: Int
  let lines: [String]

  func value(for key: String) -> String? {
    let prefix = key + "="
    return lines.first(where: { $0.hasPrefix(prefix) }).map {
      String($0.dropFirst(prefix.count))
    }
  }
}

struct TorReplyParser {
  private var code: Int?
  private var lines: [String] = []
  private var inDataBlock = false
  private var size = 0

  mutating func consume(_ line: String) throws -> TorReply? {
    size += line.utf8.count
    guard size <= 1_048_576 else { throw TorError.malformedReply }
    if inDataBlock {
      if line == "." {
        inDataBlock = false
      } else {
        lines.append(line.hasPrefix("..") ? String(line.dropFirst()) : line)
      }
      return nil
    }
    let bytes = Array(line.utf8)
    guard bytes.count >= 4,
      bytes.prefix(3).allSatisfy({ (48...57).contains($0) }),
      let nextCode = Int(String(line.prefix(3))),
      [UInt8(32), 43, 45].contains(bytes[3]),
      code == nil || code == nextCode
    else {
      throw TorError.malformedReply
    }
    code = nextCode
    lines.append(String(line.dropFirst(4)))
    if bytes[3] == 43 {
      inDataBlock = true
    }
    if bytes[3] == 32 {
      let result = TorReply(code: nextCode, lines: lines)
      self = TorReplyParser()
      return result
    }
    return nil
  }
}

public enum TorDestination {
  /// Recognize onion input without a public-DNS URL classifier or HTTPS upgrade.
  public static func onionURL(from text: String) -> URL? {
    let trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
    guard trimmed.rangeOfCharacter(from: .whitespacesAndNewlines) == nil else { return nil }
    let input = trimmed.contains("://") ? trimmed : "http://" + trimmed
    guard let url = URL(string: input),
      url.host?.lowercased().hasSuffix(".onion") == true, isAllowed(url)
    else { return nil }
    return url
  }

  public static func url(from text: String) throws -> URL {
    if let onion = onionURL(from: text) { return onion }
    let trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
    let input: String
    if trimmed.contains("://") {
      input = trimmed
    } else {
      // Onion service transport is authenticated/encrypted by Tor itself.
      let host = trimmed.split(separator: "/").first?.split(separator: ":").first ?? ""
      input = (host.lowercased().hasSuffix(".onion") ? "http://" : "https://") + trimmed
    }
    guard let url = URL(string: input), isAllowed(url) else {
      throw TorError.invalidDestination
    }
    return url
  }

  public static func isAllowed(_ url: URL) -> Bool {
    guard let scheme = url.scheme?.lowercased(), ["http", "https"].contains(scheme),
      let host = url.host?.lowercased(), !host.isEmpty,
      url.user == nil, url.password == nil,
      !host.hasSuffix("."), !host.contains(":"), !host.contains("%"),
      host != "localhost", !host.hasSuffix(".localhost"), !host.hasSuffix(".local"),
      host.contains("."), !host.allSatisfy({ $0.isNumber || $0 == "." })
    else {
      return false
    }
    return true
  }
}
