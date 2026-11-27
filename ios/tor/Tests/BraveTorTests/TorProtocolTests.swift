// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import XCTest
@testable import BraveTor

final class TorProtocolTests: XCTestCase {
  func testBootstrapSummaryUsesCompleteQuotedStageDescription() {
    XCTAssertEqual(TorSnapshot.summary(from:
      "NOTICE BOOTSTRAP PROGRESS=80 TAG=ap_conn SUMMARY=\"Connecting to the Tor network\""),
      "Connecting to the Tor network")
    XCTAssertEqual(TorSnapshot.summary(from:
      "NOTICE BOOTSTRAP PROGRESS=14 SUMMARY=\"Handshaking with a relay\" COUNT=1"),
      "Handshaking with a relay")
    XCTAssertEqual(TorSnapshot.summary(from: "NOTICE BOOTSTRAP PROGRESS=0"), "")
    XCTAssertEqual(TorSnapshot.summary(from: "SUMMARY=\"unterminated"), "")
  }

  func testMultilineControlReply() throws {
    var parser = TorReplyParser()
    XCTAssertNil(try parser.consume("250-status/circuit-established=1"))
    XCTAssertNil(try parser.consume("250-net/listeners/socks=\"127.0.0.1:9123\""))
    let reply = try XCTUnwrap(parser.consume("250 OK"))
    XCTAssertEqual(reply.code, 250)
    XCTAssertEqual(reply.value(for: "status/circuit-established"), "1")
    XCTAssertEqual(try TorEndpoint(listener: reply.value(for: "net/listeners/socks")!).port, 9123)
    XCTAssertEqual(try parser.consume("250 OK")?.lines, ["OK"])
  }

  func testDotEscapedDataAndMalformedReplies() throws {
    var parser = TorReplyParser()
    XCTAssertNil(try parser.consume("250+info="))
    XCTAssertNil(try parser.consume("..escaped"))
    XCTAssertNil(try parser.consume("."))
    XCTAssertEqual(try parser.consume("250 OK")?.lines, ["info=", ".escaped", "OK"])
    for line in ["", "250", "250?bad", "abc OK", "２５０ OK"] {
      var parser = TorReplyParser()
      XCTAssertThrowsError(try parser.consume(line), line)
    }
    var mismatched = TorReplyParser()
    XCTAssertNil(try mismatched.consume("250-first"))
    XCTAssertThrowsError(try mismatched.consume("650 event"))
  }

  func testOnlyOneNumericLoopbackProxyIsAccepted() throws {
    XCTAssertEqual(try TorEndpoint(listener: "\"127.0.0.1:65535\"").port, 65535)
    for value in [
      "", "localhost:9050", "0.0.0.0:9050", "192.168.1.1:9050",
      "127.0.0.1:0", "127.0.0.1:65536", "127.0.0.1:-1",
      "\"127.0.0.1:9050\" \"127.0.0.1:9051\"", "unix:/tmp/tor",
      "127.0.0.1:9050/path",
    ] {
      XCTAssertThrowsError(try TorEndpoint(listener: value), value)
    }
  }

  func testOnionAddressBarInputPreservesTransportAndURLComponents() throws {
    let host = String(repeating: "a", count: 56) + ".onion"
    for input in [host, host + "?q=test", host + "#section", host + "/path?q=test#section"] {
      let url = try XCTUnwrap(TorDestination.onionURL(from: input))
      XCTAssertEqual(url.scheme, "http")
      XCTAssertEqual(url.host, host)
      XCTAssertEqual(try TorDestination.url(from: input), url)
    }
    let explicit = "https://" + host + ":8443/path?q=test#section"
    XCTAssertEqual(TorDestination.onionURL(from: explicit)?.absoluteString, explicit)
    XCTAssertEqual(TorDestination.onionURL(from: "  HTTP://" + host + "/  ")?.scheme?.lowercased(), "http")
    XCTAssertEqual(TorDestination.onionURL(from: "sub." + host + ":8080/path")?.port, 8080)
    for input in [
      "example.com", "example.onion.evil.com", "https://example.com/path.onion",
      "file://" + host, "http://user:password@" + host, "http://" + host + ".",
      "search for " + host, host + "/hello world",
    ] {
      XCTAssertNil(TorDestination.onionURL(from: input), input)
    }
  }

  func testDestinationsCannotEscapeIntoOtherAppsOrLocalNetworking() throws {
    XCTAssertEqual(try TorDestination.url(from: "example.com/path").scheme, "https")
    XCTAssertEqual(try TorDestination.url(from: "example.onion/path").scheme, "http")
    XCTAssertTrue(TorDestination.isAllowed(URL(string: "https://example.com/")!))
    for value in [
      "file:///tmp/secret", "javascript:alert(1)", "data:text/html,hello",
      "brave://settings", "mailto:user@example.com", "https://user:pass@example.com",
      "http://localhost", "http://x.localhost", "http://printer.local",
      "http://127.0.0.1", "http://192.168.1.1", "http://[::1]",
      "http://2130706433", "http://0x7f000001", "https://example.com.",
    ] {
      if let url = URL(string: value) {
        XCTAssertFalse(TorDestination.isAllowed(url), value)
      }
    }
  }
}
