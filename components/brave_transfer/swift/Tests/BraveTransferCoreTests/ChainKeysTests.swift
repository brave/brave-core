// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import XCTest

final class ChainKeysTests: XCTestCase {
  func testKeyDerivationVectors() throws {
    let data = try Vectors.load("key_derivation.json")
    let valid = objects(data["valid"])
    XCTAssertFalse(valid.isEmpty)
    for v in valid {
      let keys = try ChainKeys(seed: hex(v["seed"]))
      XCTAssertEqual(keys.rootKey.hexString, v["root_key"] as? String, "\(v["name"]!)")
      XCTAssertEqual(keys.psk.hexString, v["psk"] as? String)
      XCTAssertEqual(keys.discoveryKey.hexString, v["discovery_key"] as? String)
    }
  }

  func testBadSeedLength() {
    XCTAssertThrowsError(try ChainKeys(seed: Bytes(repeating: 0, count: 31)))
  }

  func testDiscoveryTagVectors() throws {
    let data = try Vectors.load("discovery_tags.json")
    let keys = try ChainKeys(seed: hex(data["seed"]))
    XCTAssertEqual(keys.discoveryKey.hexString, data["discovery_key"] as? String)
    for v in objects(data["tags"]) {
      let time = TimeInterval(uint(v["unix_time"]))
      XCTAssertEqual(ChainKeys.epoch(unixTime: time), uint(v["epoch"]))
      XCTAssertEqual(keys.tag(epoch: uint(v["epoch"])).hexString, v["tag"] as? String)
    }
    for v in objects(data["accepted"]) {
      XCTAssertEqual(
        keys.acceptedTags(unixTime: TimeInterval(uint(v["unix_time"]))).map(\.hexString),
        v["tags"] as? [String])
    }
    let txt = try XCTUnwrap(data["txt_record"] as? JSONObject)
    let record = keys.txtRecord(unixTime: TimeInterval(uint(txt["unix_time"])))
    XCTAssertEqual(
      record.map { "\($0.key)=\($0.value)" }.sorted(), (txt["entries"] as? [String])?.sorted())
  }

  func testTXTRecordMatching() throws {
    let keys = try ChainKeys(seed: Bytes(repeating: 7, count: 32))
    let other = try ChainKeys(seed: Bytes(repeating: 8, count: 32))
    let now: TimeInterval = 1_792_054_400
    XCTAssertTrue(keys.txtRecordMatches(keys.txtRecord(unixTime: now), unixTime: now))
    XCTAssertTrue(keys.txtRecordMatches(keys.txtRecord(unixTime: now - 900), unixTime: now))
    XCTAssertFalse(keys.txtRecordMatches(keys.txtRecord(unixTime: now - 1800), unixTime: now))
    XCTAssertFalse(keys.txtRecordMatches(other.txtRecord(unixTime: now), unixTime: now))
    XCTAssertFalse(keys.txtRecordMatches(["v": "2", "c": keys.tag(epoch: 0).hexString], unixTime: 0))
    XCTAssertFalse(keys.txtRecordMatches(["v": "1", "c": "zz"], unixTime: now))
  }

  func testFingerprints() throws {
    let data = try Vectors.load("noise_handshake.json")
    for side in ["initiator", "responder"] {
      let publicKey = try StaticKey.publicKey(privateKey: hex(data["\(side)_static"]))
      XCTAssertEqual(publicKey.hexString, data["\(side)_static_public"] as? String)
      XCTAssertEqual(
        StaticKey.fingerprint(publicKey: publicKey), data["\(side)_fingerprint"] as? String)
    }
  }
}
