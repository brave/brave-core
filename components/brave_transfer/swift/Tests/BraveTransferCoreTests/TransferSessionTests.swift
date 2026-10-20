// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import XCTest

final class TransferSessionTests: XCTestCase {
  private func vectorItems(_ data: JSONObject) -> [OutgoingItem] {
    objects(data["items"]).map { j in
      let content: Bytes
      if let text = j["text"] as? String {
        content = Bytes(text.utf8)
      } else {
        content = pattern(Int(uint(j["pattern_length"])))
      }
      return OutgoingItem.fromBytes(
        kind: ItemKind(rawValue: uint32(j["kind"])), name: j["name"] as! String,
        mimeType: j["mime_type"] as! String, data: content)
    }
  }

  /// Same delivery order as run_scenario() in tools/brave_transfer/gen_vectors.py.
  private func runScenario(
    items: [OutgoingItem], transferID: Bytes, decision: Decision, acceptedIDs: [UInt32]
  ) throws -> (frames: [(String, Frame)], results: [String: TransferResult]) {
    let now: TimeInterval = 1000
    let initiator = TransferSession(now: now, random: { Array(transferID.prefix($0)) })
    let responder = TransferSession(now: now, sendConfirmation: true)
    try initiator.sendOffer(items, now: now)
    var frames: [(String, Frame)] = []
    var results: [String: TransferResult] = [:]

    func deliver(_ frame: Frame, to receiver: TransferSession, named name: String) throws {
      var events = try receiver.handle(Frame(serialized: frame.serialized()), now: now)
      while !events.isEmpty {
        switch events.removeFirst() {
        case .offerReceived(let offer):
          events += try receiver.respond(
            transferID: offer.transferID, decision: decision, acceptedItemIDs: acceptedIDs)
        case .transferFinished(let result):
          results[name] = result
        default:
          break
        }
      }
    }

    var progressed = true
    while progressed {
      progressed = false
      for (sender, receiver, name, other) in [
        (responder, initiator, "responder", "initiator"),
        (initiator, responder, "initiator", "responder"),
      ] {
        while let frame = try sender.nextFrame(now: now) {
          frames.append((name, frame))
          try deliver(frame, to: receiver, named: other)
          progressed = true
        }
      }
    }
    XCTAssertTrue(initiator.isIdle)
    XCTAssertTrue(responder.isIdle)
    return (frames, results)
  }

  func testTranscriptVectors() throws {
    let data = try Vectors.load("session_transcript.json")
    let transferID = hex(data["transfer_id"])
    let scenarios = objects(data["scenarios"])
    XCTAssertEqual(scenarios.count, 4)
    for scenario in scenarios {
      let name = scenario["name"] as! String
      let (frames, results) = try runScenario(
        items: vectorItems(data), transferID: transferID,
        decision: Decision(rawValue: uint32(scenario["decision"])),
        acceptedIDs: (scenario["accepted_item_ids"] as? [Any] ?? []).map(uint32))
      let expected = objects(scenario["frames"])
      XCTAssertEqual(frames.count, expected.count, name)
      for ((sender, frame), want) in zip(frames, expected) {
        let raw = frame.serialized()
        XCTAssertEqual(sender, want["sender"] as? String, name)
        XCTAssertEqual(raw.count, Int(uint(want["length"])), name)
        XCTAssertEqual(SHA256Digest.hash(raw).hexString, want["sha256"] as? String, name)
        if let plaintext = want["plaintext"] as? String {
          XCTAssertEqual(raw.hexString, plaintext, name)
        }
      }
      for side in ["initiator", "responder"] {
        let want = scenario["\(side)_result"] as! JSONObject
        let got = try XCTUnwrap(results[side], name)
        XCTAssertEqual(got.status?.rawValue, (want["status"] as Any?).flatMap { $0 as? NSNumber }?.uint32Value, name)
        XCTAssertEqual(
          got.decision?.rawValue, (want["decision"] as Any?).flatMap { $0 as? NSNumber }?.uint32Value, name)
      }
    }
  }

  // MARK: Edge cases, mirroring tools/brave_transfer/tests/test_session.py.

  private func items() throws -> [OutgoingItem] {
    vectorItems(try Vectors.load("session_transcript.json"))
  }

  private func drain(_ session: TransferSession, now: TimeInterval = 0) throws -> [Frame] {
    var frames: [Frame] = []
    while let frame = try session.nextFrame(now: now) {
      frames.append(frame)
    }
    return frames
  }

  @discardableResult
  private func deliver(_ frames: [Frame], to session: TransferSession) throws -> [TransferEvent] {
    try frames.flatMap { try session.handle(Frame(serialized: $0.serialized()), now: 0) }
  }

  private func finished(_ events: [TransferEvent]) -> [TransferResult] {
    events.compactMap {
      if case .transferFinished(let r) = $0 { return r }
      return nil
    }
  }

  private func offer(
    from sender: TransferSession, to receiver: TransferSession, items: [OutgoingItem]? = nil
  ) throws -> Bytes {
    let id = try sender.sendOffer(items ?? self.items(), now: 0)
    try deliver(drain(sender), to: receiver)
    return id
  }

  func testChunksReassemble() throws {
    let sender = TransferSession(now: 0)
    let receiver = TransferSession(now: 0)
    let id = try offer(from: sender, to: receiver)
    _ = try receiver.respond(transferID: id, decision: .acceptAll)
    try deliver(drain(receiver), to: sender)
    var data: [UInt32: Bytes] = [:]
    var received = Set<UInt32>()
    for event in try deliver(drain(sender), to: receiver) {
      if case .chunkReceived(_, let itemID, _, let bytes) = event {
        data[itemID, default: []] += bytes
      }
      if case .itemReceived(_, let item) = event {
        received.insert(item.itemID)
      }
    }
    XCTAssertEqual(data[3], pattern(62440))
    XCTAssertEqual(received, [3, 5])
    XCTAssertEqual(finished(try deliver(drain(receiver), to: sender)).first?.status, .ok)
  }

  func testSecondIncomingOfferIsBusy() throws {
    let a = TransferSession(now: 0)
    let b = TransferSession(now: 0)
    let c = TransferSession(now: 0)
    _ = try offer(from: a, to: b)
    let id = try c.sendOffer(items(), now: 0)
    XCTAssertEqual(try deliver(drain(c), to: b), [])
    let frames = try drain(b)
    XCTAssertEqual(frames, [.response(Response(transferID: id, decision: .busy))])
    XCTAssertEqual(finished(try deliver(frames, to: c)).first?.decision, .busy)
  }

  func testHashMismatch() throws {
    let data = Bytes(repeating: 0x78, count: 100_000)
    let bad = OutgoingItem(
      kind: .file, name: "f", mimeType: "", size: UInt64(data.count),
      sha256: SHA256Digest.hash([1]),
      read: { offset, length in Array(data[Int(offset)..<Int(offset) + length]) })
    let s = TransferSession(now: 0)
    let r = TransferSession(now: 0)
    let id = try offer(from: s, to: r, items: [bad])
    _ = try r.respond(transferID: id, decision: .acceptAll)
    try deliver(drain(r), to: s)
    XCTAssertEqual(finished(try deliver(drain(s), to: r)).first?.status, .hashMismatch)
    XCTAssertEqual(finished(try deliver(drain(r), to: s)).first?.status, .hashMismatch)
  }

  func testWriteFailure() throws {
    let s = TransferSession(now: 0)
    let r = TransferSession(now: 0)
    let id = try offer(from: s, to: r)
    _ = try r.respond(transferID: id, decision: .acceptAll)
    try deliver(drain(r), to: s)
    try deliver([XCTUnwrap(try s.nextFrame(now: 0))], to: r)
    XCTAssertEqual(finished(try r.writeFailed(transferID: id)).first?.status, .writeFailed)
    XCTAssertEqual(finished(try deliver(drain(r), to: s)).first?.status, .writeFailed)
    XCTAssertTrue(s.isIdle && r.isIdle)
  }

  func testCancelMidTransferIgnoresFramesInFlight() throws {
    let s = TransferSession(now: 0)
    let r = TransferSession(now: 0)
    let id = try offer(from: s, to: r)
    _ = try r.respond(transferID: id, decision: .acceptAll)
    try deliver(drain(r), to: s)
    let firstChunk = try XCTUnwrap(try s.nextFrame(now: 0))
    _ = s.cancel(transferID: id)
    let rest = try drain(s)
    XCTAssertEqual(rest, [.cancel(Cancel(transferID: id, reason: .user))])
    XCTAssertEqual(finished(try deliver([firstChunk] + rest, to: r)).first?.cancelReason, .user)
    XCTAssertTrue(r.isIdle && s.isIdle)
    XCTAssertEqual(try deliver([firstChunk], to: r), [])
  }

  func testTimeouts() throws {
    let s = TransferSession(now: 0)
    let r = TransferSession(now: 0)
    let id = try offer(from: s, to: r)
    let t = TransferLimits.offerTimeout + 1
    try deliver([.ping(Ping(sequence: 1))], to: s)
    _ = try s.handle(.ping(Ping()), now: t)
    _ = try r.handle(.ping(Ping()), now: t)
    XCTAssertEqual(finished(try s.poll(now: t)).first?.cancelReason, .timeout)
    XCTAssertEqual(try drain(s, now: t).first, .cancel(Cancel(transferID: id, reason: .timeout)))
    XCTAssertEqual(finished(try r.poll(now: t)).first?.decision, .decline)

    let idle = TransferSession(now: 0)
    XCTAssertEqual(try idle.poll(now: TransferLimits.pingInterval - 1), [])
    XCTAssertNil(try idle.nextFrame(now: 0))
    _ = try idle.poll(now: TransferLimits.pingInterval)
    XCTAssertEqual(try idle.nextFrame(now: TransferLimits.pingInterval), .ping(Ping(sequence: 1)))
    XCTAssertThrowsError(try idle.poll(now: TransferLimits.idleTimeout + 0.5))
  }

  func testBadChunksAreProtocolErrors() throws {
    let mutations: [(inout Chunk) -> Void] = [
      { $0.offset += 1 },
      { $0.itemID = 99 },
      { $0.data = [] },
      { $0.transferID = Bytes(repeating: 0, count: 16) },
    ]
    for mutate in mutations {
      let s = TransferSession(now: 0)
      let r = TransferSession(now: 0)
      let id = try offer(from: s, to: r)
      _ = try r.respond(transferID: id, decision: .acceptAll)
      try deliver(drain(r), to: s)
      guard case .chunk(var chunk) = try s.nextFrame(now: 0) else { return XCTFail() }
      mutate(&chunk)
      XCTAssertThrowsError(try deliver([.chunk(chunk)], to: r))
    }
  }

  func testBadResponsesAreProtocolErrors() throws {
    let responses: [(Decision, [UInt32])] = [
      (.acceptAll, [1]), (.acceptSome, []), (.acceptSome, [1, 1]), (.acceptSome, [42]),
      (.decline, [1]), (.unspecified, []),
    ]
    for (decision, ids) in responses {
      let s = TransferSession(now: 0)
      let r = TransferSession(now: 0)
      let id = try offer(from: s, to: r)
      XCTAssertThrowsError(
        try s.handle(
          .response(Response(transferID: id, decision: decision, acceptedItemIDs: ids)), now: 0))
    }
  }

  func testReusedTransferIDIsAnError() throws {
    let a = TransferSession(now: 0, random: { Bytes(repeating: 1, count: $0) })
    let b = TransferSession(now: 0)
    let id = try offer(from: a, to: b)
    _ = try b.respond(transferID: id, decision: .decline)
    try deliver(drain(b), to: a)
    try a.sendOffer(items(), now: 0)
    XCTAssertThrowsError(try deliver(drain(a), to: b))
  }
}
