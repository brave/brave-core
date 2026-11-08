// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Darwin
import Foundation

public enum TorBridgeSource: String, Codable, CaseIterable, Hashable, Sendable {
  case builtIn, request, provide
}

public enum TorBuiltInBridge: String, Codable, CaseIterable, Hashable, Sendable {
  case obfs4, snowflake, meekAzure
  public var title: String {
    switch self {
    case .obfs4: return "obfs4"
    case .snowflake: return "Snowflake"
    case .meekAzure: return "meek-azure"
    }
  }
  public var lines: [String] {
    switch self {
    case .obfs4: return [
      "obfs4 193.11.166.194:27020 86AC7B8D430DAC4117E9F42C9EAED18133863AAF cert=0LDeJH4JzMDtkJJrFphJCiPqKx7loozKN7VNfuukMGfHO0Z8OGdzHVkhVAOfo1mUdv9cMg iat-mode=0",
      "obfs4 193.11.166.194:27015 2D82C2E354D531A68469ADF7F878FA6060C6BACA cert=4TLQPJrTSaDffMK7Nbao6LC7G9OW/NHkUwIdjLSS3KYf0Nv4/nQiiI8dY2TcsQx01NniOg iat-mode=0",
      "obfs4 85.31.186.26:443 91A6354697E6B02A386312F68D82CF86824D3606 cert=PBwr+S8JTVZo6MPdHnkTwXJPILWADLqfMGoVvhZClMq/Urndyd42BwX9YFJHZnBB3H0XCw iat-mode=0",
      "obfs4 146.57.248.225:22 10A6CD36A537FCE513A322361547444B393989F0 cert=K1gDtDAIcUfeLqbstggjIw2rtgIKqdIhUlHp82XRqNSq/mtAjp1BIC9vHKJ2FAEpGssTPw iat-mode=0",
      "obfs4 209.148.46.65:443 74FAD13168806246602538555B5521A0383A1875 cert=ssH+9rP8dG2NLDN2XuFw63hIO/9MNNinLmxQDpVa+7kTOa9/m+tGWT1SmSYpQ9uTBGa6Hw iat-mode=0",
      "obfs4 193.11.166.194:27025 1AE2C08904527FEA90C4C4F8C1083EA59FBC6FAF cert=ItvYZzW5tn6v3G4UnQa6Qz04Npro6e81AP70YujmK/KXwDFPTs3aHXcHp4n8Vt6w/bv8cA iat-mode=0",
      "obfs4 45.145.95.6:27015 C5B7CD6946FF10C5B3E89691A7D3F2C122D2117C cert=TD7PbUO0/0k6xYHMPW3vJxICfkMZNdkRrb63Zhl5j9dW3iRGiCx0A7mPhe5T2EDzQ35+Zw iat-mode=0",
      "obfs4 51.222.13.177:80 5EDAC3B810E12B01F6FD8050D2FD3E277B289A08 cert=2uplIpLQ0q9+0qMFrK5pkaYRDOe460LL9WHBvatgkuRr/SL31wBOEupaMMJ6koRE6Ld0ew iat-mode=0",
      "obfs4 37.218.245.14:38224 D9A82D2F9C2F65A18407B1D2B764F130847F8B5D cert=bjRaMrr1BRiAW8IE9U5z27fQaYgOhX1UCmOpg2pFpoMvo6ZgQMzLsaTzzQNTlm7hNcb+Sg iat-mode=0",
      "obfs4 192.95.36.142:443 CDF2E852BF539B82BD10E27E9115A31734E378C2 cert=qUVQ0srL1JI/vO6V6m/24anYXiJD3QP2HgzUKQtQ7GRqqUvs7P+tG43RtAqdhLOALP7DJQ iat-mode=1",
      "obfs4 85.31.186.98:443 011F2599C0E9B27EE74B353155E244813763C3E5 cert=ayq0XzCwhpdysn5o0EyDUbmSOx3X/oTEbzDMvczHOdBJKlvIdHHLJGkZARtT4dcBFArPPg iat-mode=0",
    ]
    case .snowflake: return [
      "snowflake 192.0.2.4:80 8838024498816A039FCBBAB14E6F40A0843051FA fingerprint=8838024498816A039FCBBAB14E6F40A0843051FA url=https://1098762253.rsc.cdn77.org/ fronts=www.cdn77.com,www.phpmyadmin.net ice=stun:stun.l.google.com:19302,stun:stun.antisip.com:3478,stun:stun.bluesip.net:3478,stun:stun.dus.net:3478,stun:stun.epygi.com:3478,stun:stun.sonetel.net:3478,stun:stun.uls.co.za:3478,stun:stun.voipgate.com:3478,stun:stun.voys.nl:3478 utls-imitate=hellorandomizedalpn",
      "snowflake 192.0.2.3:80 2B280B23E1107BB62ABFC40DDCC8824814F80A72 fingerprint=2B280B23E1107BB62ABFC40DDCC8824814F80A72 url=https://1098762253.rsc.cdn77.org/ fronts=www.cdn77.com,www.phpmyadmin.net ice=stun:stun.l.google.com:19302,stun:stun.antisip.com:3478,stun:stun.bluesip.net:3478,stun:stun.dus.net:3478,stun:stun.epygi.com:3478,stun:stun.sonetel.com:3478,stun:stun.uls.co.za:3478,stun:stun.voipgate.com:3478,stun:stun.voys.nl:3478 utls-imitate=hellorandomizedalpn",
    ]
    case .meekAzure: return [
      "meek_lite 192.0.2.18:80 BE776A53492E1E044A26F17306E1BC46A55A1625 url=https://meek.azureedge.net/ front=ajax.aspnetcdn.com",
    ]
    }
  }
}

public struct TorBridgeConfiguration: Codable, Equatable, Sendable {
  public var useBridges = false
  public var source: TorBridgeSource = .builtIn
  public var builtIn: TorBuiltInBridge = .obfs4
  public var requested: [String] = []
  public var provided: [String] = []
  public init() {}

  public func bridgeLines() throws -> [String] {
    guard useBridges else { return [] }
    let lines: [String]
    switch source {
    case .builtIn: lines = builtIn.lines
    case .request: lines = requested
    case .provide: lines = provided
    }
    guard !lines.isEmpty, lines.count <= 64 else { throw TorBridgeError.invalidLines }
    try lines.forEach { try TorBridgeLine.validate($0) }
    return lines
  }

  public var encoded: String {
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.sortedKeys]
    return String(data: (try? encoder.encode(self)) ?? Data(), encoding: .utf8) ?? ""
  }

  public static func decode(_ value: String) -> Self {
    if let data = value.data(using: .utf8),
       let config = try? JSONDecoder().decode(Self.self, from: data) { return config }
    // Corrupt settings must not silently fall back to a direct Tor connection.
    var invalid = Self()
    invalid.useBridges = true
    invalid.source = .provide
    return invalid
  }
}

public enum TorBridgeError: Error {
  case invalidLines, unsupportedTransport, invalidResponse
}

public enum TorBridgeLine {
  public static func lines(from text: String) -> [String] {
    text.components(separatedBy: .newlines)
      .map { $0.trimmingCharacters(in: .whitespaces) }
      .filter { !$0.isEmpty }
      .map { $0.hasPrefix("Bridge ") ? String($0.dropFirst(7)) : $0 }
  }

  public static func transport(_ line: String) -> String? {
    let first = line.split(separator: " ").first.map(String.init) ?? ""
    return ["obfs4", "snowflake", "meek_lite"].contains(first) ? first : nil
  }

  public static func validate(_ line: String) throws {
    guard !line.isEmpty, line.utf8.count <= 4096,
      line.utf8.allSatisfy({ $0 >= 32 && $0 < 127 && $0 != 34 && $0 != 92 })
    else { throw TorBridgeError.invalidLines }
    var fields = line.split(separator: " ").map(String.init)
    if let first = fields.first, !first.contains(":") {
      guard transport(line) != nil else { throw TorBridgeError.unsupportedTransport }
      fields.removeFirst()
    }
    guard let address = fields.first, let colon = address.lastIndex(of: ":"),
      let port = Int(address[address.index(after: colon)...]), (1...65535).contains(port)
    else { throw TorBridgeError.invalidLines }
    var host = String(address[..<colon])
    let family: Int32
    if host.hasPrefix("["), host.hasSuffix("]") {
      host = String(host.dropFirst().dropLast())
      family = AF_INET6
    } else { family = AF_INET }
    var bytes = [UInt8](repeating: 0, count: 16)
    let parsed = host.withCString { pointer in
      bytes.withUnsafeMutableBytes { inet_pton(family, pointer, $0.baseAddress) }
    }
    guard parsed == 1 else { throw TorBridgeError.invalidLines }
    fields.removeFirst()
    if let fingerprint = fields.first, !fingerprint.contains("=") {
      guard fingerprint.count == 40, fingerprint.allSatisfy({ $0.isHexDigit })
      else { throw TorBridgeError.invalidLines }
      fields.removeFirst()
    }
    for field in fields {
      guard let equals = field.firstIndex(of: "="), equals != field.startIndex,
        field[..<equals].allSatisfy({ $0.isASCII && ($0.isLetter || $0.isNumber || $0 == "-" || $0 == "_") }),
        field.index(after: equals) < field.endIndex
      else { throw TorBridgeError.invalidLines }
    }
  }

  public static func controlCommand(lines: [String], ports: [String: Int]) throws -> String {
    guard !lines.isEmpty else { return "RESETCONF UseBridges Bridge ClientTransportPlugin" }
    guard lines.count <= 64 else { throw TorBridgeError.invalidLines }
    try lines.forEach { try validate($0) }
    var parts = ["SETCONF", "UseBridges=1"]
    for name in Set(lines.compactMap(transport)).sorted() {
      guard let port = ports[name], (1...65535).contains(port) else {
        throw TorBridgeError.unsupportedTransport
      }
      parts.append("ClientTransportPlugin=\"\(name) socks5 127.0.0.1:\(port)\"")
    }
    parts += lines.map { "Bridge=\"\($0)\"" }
    return parts.joined(separator: " ")
  }
}
