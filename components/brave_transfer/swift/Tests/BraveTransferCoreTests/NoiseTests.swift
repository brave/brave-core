// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import BraveTransferCore

final class NoiseTests: XCTestCase {
  func testCacophonyVector() throws {
    let v = try Vectors.load("noise_cacophony_xxpsk3.json")
    let initiator = try NoiseHandshake(
      initiator: true, prologue: hex(v["init_prologue"]), staticPrivateKey: hex(v["init_static"]),
      psk: hex((v["init_psks"] as? [String])?.first),
      ephemeralPrivateKey: hex(v["init_ephemeral"]))
    let responder = try NoiseHandshake(
      initiator: false, prologue: hex(v["resp_prologue"]), staticPrivateKey: hex(v["resp_static"]),
      psk: hex((v["resp_psks"] as? [String])?.first),
      ephemeralPrivateKey: hex(v["resp_ephemeral"]))
    let messages = objects(v["messages"])
    XCTAssertEqual(messages.count, 6)
    for (i, m) in messages.enumerated() {
      let payload = hex(m["payload"])
      let ciphertext = hex(m["ciphertext"])
      let (sender, receiver) = i % 2 == 0 ? (initiator, responder) : (responder, initiator)
      if i < 3 {
        XCTAssertEqual(try sender.writeMessage(payload: payload), ciphertext, "message \(i + 1)")
        XCTAssertEqual(try receiver.readMessage(ciphertext), payload)
      } else {
        XCTAssertEqual(try sender.sendCipher!.encrypt(ad: [], plaintext: payload), ciphertext)
        XCTAssertEqual(try receiver.receiveCipher!.decrypt(ad: [], ciphertext: ciphertext), payload)
      }
    }
    XCTAssertEqual(initiator.handshakeHash?.hexString, v["handshake_hash"] as? String)
    XCTAssertEqual(responder.handshakeHash, initiator.handshakeHash)
  }

  func testBraveHandshakeVector() throws {
    let v = try Vectors.load("noise_handshake.json")
    let tag = hex(v["trust_tag"])
    let psk = hex(v["psk"])
    let messages = objects(v["handshake_messages"])
    let responderInfo = PeerInfo(json: messages[1]["payload_message"] as! JSONObject)
    let initiatorInfo = PeerInfo(json: messages[2]["payload_message"] as! JSONObject)
    XCTAssertEqual(responderInfo.serialized().hexString, messages[1]["payload"] as? String)

    let initiator = try InitiatorHandshake(
      staticPrivateKey: hex(v["initiator_static"]), psk: psk, trustTag: tag,
      localInfo: initiatorInfo, ephemeralPrivateKey: hex(v["initiator_ephemeral"]))
    let responder = try ResponderHandshake(
      staticPrivateKey: hex(v["responder_static"]), localInfo: responderInfo,
      ephemeralPrivateKey: hex(v["responder_ephemeral"])
    ) { $0 == tag ? psk : nil }

    let m1 = try initiator.message1()
    XCTAssertEqual(m1.hexString, messages[0]["message"] as? String)
    XCTAssertEqual(try responder.readMessage1(m1).trustTag, tag)
    let m2 = try responder.message2()
    XCTAssertEqual(m2.hexString, messages[1]["message"] as? String)
    XCTAssertEqual(try initiator.readMessage2(m2), responderInfo)
    let m3 = try initiator.message3()
    XCTAssertEqual(m3.hexString, messages[2]["message"] as? String)
    XCTAssertEqual(try responder.readMessage3(m3), initiatorInfo)

    let ic = try initiator.channel()
    let rc = try responder.channel()
    XCTAssertEqual(ic.handshakeHash.hexString, v["handshake_hash"] as? String)
    XCTAssertEqual(ic.sendKey.hexString, v["initiator_to_responder_key"] as? String)
    XCTAssertEqual(rc.sendKey.hexString, v["responder_to_initiator_key"] as? String)
    XCTAssertEqual(ic.remoteStaticKey.hexString, v["responder_static_public"] as? String)
    XCTAssertEqual(rc.remoteStaticKey.hexString, v["initiator_static_public"] as? String)
    XCTAssertFalse(ic.isConfirmed)
    XCTAssertTrue(rc.isConfirmed)

    for m in objects(v["transport_messages"]) {
      let frame = Frame(json: m["frame"] as! JSONObject)
      XCTAssertEqual(frame.serialized().hexString, m["plaintext"] as? String)
      let (sender, receiver) = m["sender"] as? String == "initiator" ? (ic, rc) : (rc, ic)
      let wire = try sender.seal(frame)
      XCTAssertEqual(wire.hexString, m["wire"] as? String)
      XCTAssertEqual(try receiver.open(Bytes(wire[2...])), frame)
      XCTAssertTrue(ic.isConfirmed)
    }
  }

  private func info(_ name: String) -> PeerInfo {
    PeerInfo(deviceName: name, deviceType: .desktop, syncDeviceGUID: "")
  }

  private func handshake(
    initiatorKeys: ChainKeys, responderKeys: ChainKeys, tag: Bytes
  ) throws -> (SecureChannel, SecureChannel) {
    let initiator = try InitiatorHandshake(
      staticPrivateKey: StaticKey.generatePrivateKey(), psk: initiatorKeys.psk, trustTag: tag,
      localInfo: info("Mac"))
    let responder = try ResponderHandshake(
      staticPrivateKey: StaticKey.generatePrivateKey(), localInfo: info("iPhone"),
      keys: responderKeys, unixTime: 900_000)
    try responder.readMessage1(initiator.message1())
    try initiator.readMessage2(responder.message2())
    try responder.readMessage3(initiator.message3())
    return (try initiator.channel(), try responder.channel())
  }

  func testRoundTripAndAdjacentEpochs() throws {
    let keys = try ChainKeys(seed: Bytes(repeating: 1, count: 32))
    for epoch: UInt64 in [999, 1000, 1001] {
      let (ic, rc) = try handshake(
        initiatorKeys: keys, responderKeys: keys, tag: keys.tag(epoch: epoch))
      let wire = try rc.seal(.ping(Ping(sequence: 0)))
      XCTAssertEqual(try ic.open(Bytes(wire[2...])), .ping(Ping(sequence: 0)))
      XCTAssertTrue(ic.isConfirmed)
    }
    XCTAssertThrowsError(
      try handshake(initiatorKeys: keys, responderKeys: keys, tag: keys.tag(epoch: 1002)))
  }

  func testOtherChainIsRejected() throws {
    let a = try ChainKeys(seed: Bytes(repeating: 1, count: 32))
    let b = try ChainKeys(seed: Bytes(repeating: 2, count: 32))
    XCTAssertThrowsError(try handshake(initiatorKeys: a, responderKeys: b, tag: a.tag(epoch: 1000)))
  }

  func testWrongPSKFailsAtMessage3() throws {
    let a = try ChainKeys(seed: Bytes(repeating: 1, count: 32))
    let b = try ChainKeys(seed: Bytes(repeating: 2, count: 32))
    XCTAssertThrowsError(try handshake(initiatorKeys: a, responderKeys: b, tag: b.tag(epoch: 1000)))
    { error in
      XCTAssertTrue("\(error)".contains("message 3"), "\(error)")
    }
  }

  func testLowOrderPublicKeysAreRejected() {
    // 0 and 1 are low-order points on Curve25519.
    for point in [Bytes(repeating: 0, count: 32), [1] + Bytes(repeating: 0, count: 31)] {
      XCTAssertThrowsError(
        try CryptoPrimitives.x25519(privateKey: StaticKey.generatePrivateKey(), publicKey: point))
    }
  }

  func testTamperedTransportMessageIsRejected() throws {
    let keys = try ChainKeys(seed: Bytes(repeating: 1, count: 32))
    let (ic, rc) = try handshake(initiatorKeys: keys, responderKeys: keys, tag: keys.tag(epoch: 1000))
    var wire = try ic.seal(.ping(Ping(sequence: 1)))
    wire[wire.count - 1] ^= 1
    XCTAssertThrowsError(try rc.open(Bytes(wire[2...])))
  }

  func testOversizedControlFrameIsRejected() throws {
    let keys = try ChainKeys(seed: Bytes(repeating: 1, count: 32))
    let (ic, _) = try handshake(initiatorKeys: keys, responderKeys: keys, tag: keys.tag(epoch: 1000))
    var item = OfferItem()
    item.name = String(repeating: "a", count: 17000)
    XCTAssertThrowsError(try ic.seal(.offer(Offer(transferID: Bytes(repeating: 0, count: 16), items: [item]))))
  }

  func testMessageReader() throws {
    var reader = MessageReader()
    let stream = try TransferProtocol.lengthPrefixed([1, 2, 3]) + TransferProtocol.lengthPrefixed([])
      + TransferProtocol.lengthPrefixed(Bytes(repeating: 9, count: 300))
    var out: [Bytes] = []
    for byte in stream {
      out += reader.feed([byte])
    }
    XCTAssertEqual(out, [[1, 2, 3], [], Bytes(repeating: 9, count: 300)])
    XCTAssertFalse(reader.hasPartialMessage)
    XCTAssertThrowsError(try TransferProtocol.lengthPrefixed(Bytes(repeating: 0, count: 65536)))
  }
}
