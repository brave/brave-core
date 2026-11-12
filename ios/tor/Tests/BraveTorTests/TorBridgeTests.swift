// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Foundation
import XCTest
@testable import BraveTor

final class TorBridgeTests: XCTestCase {
  func testDesktopBuiltInsValidateAndRequireTheirTransport() throws {
    for builtIn in TorBuiltInBridge.allCases {
      var config = TorBridgeConfiguration()
      config.useBridges = true
      config.builtIn = builtIn
      let lines = try config.bridgeLines()
      XCTAssertFalse(lines.isEmpty)
      XCTAssertThrowsError(try TorBridgeLine.controlCommand(lines: lines, ports: [:]))
      let ports = Dictionary(uniqueKeysWithValues: Set(lines.compactMap(TorBridgeLine.transport))
        .map { ($0, 12345) })
      let command = try TorBridgeLine.controlCommand(lines: lines, ports: ports)
      XCTAssertTrue(command.contains("UseBridges=1"))
      XCTAssertTrue(command.contains("socks5 127.0.0.1:12345"))
    }
  }

  func testBridgeGrammarAndControlInjectionAreRejected() throws {
    for line in [
      "192.0.2.1:443", "[2001:db8::1]:9001",
      "obfs4 192.0.2.1:443 " + String(repeating: "A", count: 40) + " cert=abc/+def iat-mode=0"
    ] { try TorBridgeLine.validate(line) }
    for line in [
      "", "host.example:443", "192.0.2.1:0", "192.0.2.1:65536",
      "192.0.2.1:443\nDisableNetwork 0", "192.0.2.1:443\" Bridge=\"127.0.0.1:80",
      "obfs4 192.0.2.1:443 cert=abc\\def", "unknown 192.0.2.1:443",
      "192.0.2.1:443 NOT_A_FINGERPRINT", "obfs4 192.0.2.1:443 cert="
    ] { XCTAssertThrowsError(try TorBridgeLine.validate(line), line) }
    XCTAssertEqual(TorBridgeLine.lines(from: " Bridge 192.0.2.1:443\n\n192.0.2.2:443 "),
                   ["192.0.2.1:443", "192.0.2.2:443"])
  }

  func testDisabledAndCorruptConfigurationsDoNotSilentlyUseBridgesOrPublicRelays() throws {
    let config = TorBridgeConfiguration()
    XCTAssertEqual(try config.bridgeLines(), [])
    XCTAssertEqual(try TorBridgeLine.controlCommand(lines: [], ports: [:]),
                   "RESETCONF UseBridges Bridge ClientTransportPlugin")
    XCTAssertEqual(TorBridgeConfiguration.decode(config.encoded), config)
    let corrupt = TorBridgeConfiguration.decode("not json")
    XCTAssertTrue(corrupt.useBridges)
    XCTAssertThrowsError(try corrupt.bridgeLines())
    var empty = config
    empty.useBridges = true
    empty.source = .request
    XCTAssertThrowsError(try empty.bridgeLines())
    empty.source = .provide
    XCTAssertThrowsError(try empty.bridgeLines())
  }

  func testMoatResponseParsingAndBounds() throws {
    func payload(_ item: [String: Any]) throws -> Data {
      try JSONSerialization.data(withJSONObject: ["data": [item]])
    }
    let challenge = try TorMoatClient.decodeChallenge(payload([
      "image": Data([1, 2, 3]).base64EncodedString(), "challenge": "fixture"
    ]))
    XCTAssertEqual(challenge.image, Data([1, 2, 3]))
    XCTAssertThrowsError(try TorMoatClient.decodeChallenge(payload(["image": "invalid"])))
    XCTAssertThrowsError(try TorMoatClient.decodeChallenge(Data(repeating: 32, count: 262145)))
    let bridge = "obfs4 192.0.2.1:443 cert=fixture iat-mode=0"
    XCTAssertEqual(try TorMoatClient.decodeBridges(payload(["bridges": [bridge]])), [bridge])
    XCTAssertThrowsError(try TorMoatClient.decodeBridges(payload(["bridges": []])))
    XCTAssertThrowsError(try TorMoatClient.decodeBridges(payload(["bridges": ["192.0.2.1:443"]])))
    XCTAssertThrowsError(try TorMoatClient.decodeBridges(payload(["bridges": [bridge + "\r\nSETCONF"]])))
  }
}
