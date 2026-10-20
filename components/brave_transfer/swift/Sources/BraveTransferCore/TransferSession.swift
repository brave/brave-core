// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Transfer limits and timers (PROTOCOL.md §8).
public enum TransferLimits {
  public static let transferIDLength = 16
  public static let maxItems = 256
  public static let maxNameBytes = 1024
  public static let maxMimeTypeBytes = 255
  public static let maxInlineBytes = 8192
  public static let maxItemSize: UInt64 = 1 << 40
  public static let maxChunkData = 61440
  public static let offerTimeout: TimeInterval = 60
  public static let idleTimeout: TimeInterval = 30
  public static let pingInterval: TimeInterval = 10
}

public struct ReceiveLimits {
  public var maxItems = TransferLimits.maxItems
  public var maxTotalSize = TransferLimits.maxItemSize

  public init() {}
}

/// Something to send. Small TEXT and URL items travel inline; everything
/// else is read in chunks through `read(offset, length)`.
public struct OutgoingItem {
  public var kind: ItemKind
  public var name: String
  public var mimeType: String
  public var size: UInt64
  public var sha256: Bytes
  public var inlineData: Bytes
  public var read: ((UInt64, Int) throws -> Bytes)?

  public init(
    kind: ItemKind, name: String, mimeType: String, size: UInt64, sha256: Bytes,
    inlineData: Bytes = [], read: ((UInt64, Int) throws -> Bytes)? = nil
  ) {
    self.kind = kind
    self.name = name
    self.mimeType = mimeType
    self.size = size
    self.sha256 = sha256
    self.inlineData = inlineData
    self.read = read
  }

  public static func fromBytes(
    kind: ItemKind, name: String, mimeType: String, data: Bytes
  ) -> OutgoingItem {
    let inline = (kind == .text || kind == .url) && data.count <= TransferLimits.maxInlineBytes
    return OutgoingItem(
      kind: kind, name: name, mimeType: mimeType, size: UInt64(data.count),
      sha256: CryptoPrimitives.sha256(data), inlineData: inline ? data : [],
      read: inline
        ? nil
        : { offset, length in
          Array(data[Int(offset)..<(Int(offset) + length)])
        })
  }
}

public struct TransferResult: Equatable {
  public var transferID: Bytes
  public var isOutgoing: Bool
  /// nil when the transfer was cancelled or refused.
  public var status: DoneStatus?
  public var cancelReason: CancelReason?
  public var decision: Decision?
}

public enum TransferEvent: Equatable {
  case offerReceived(Offer)
  case responseReceived(transferID: Bytes, decision: Decision, acceptedItemIDs: [UInt32])
  case inlineItemReceived(transferID: Bytes, item: OfferItem)
  case chunkReceived(transferID: Bytes, itemID: UInt32, offset: UInt64, data: Bytes)
  case itemReceived(transferID: Bytes, item: OfferItem)
  case transferFinished(TransferResult)
}

private func validate(_ condition: Bool, _ message: String) throws {
  if !condition { throw ProtocolError(message) }
}

/// Throws ProtocolError if the offer breaks PROTOCOL.md §8.2.
public func validateOffer(_ offer: Offer, limits: ReceiveLimits = ReceiveLimits()) throws {
  try validate(offer.transferID.count == TransferLimits.transferIDLength, "bad transfer id")
  try validate((1...limits.maxItems).contains(offer.items.count), "bad item count")
  var ids = Set<UInt32>()
  var total: UInt64 = 0
  for item in offer.items {
    try validate(ids.insert(item.itemID).inserted, "duplicate item id")
    try validate([.file, .image, .text, .url].contains(item.kind), "bad item kind")
    try validate((1...TransferLimits.maxNameBytes).contains(item.name.utf8.count), "bad item name")
    try validate(item.mimeType.utf8.count <= TransferLimits.maxMimeTypeBytes, "bad mime type")
    try validate(item.sha256.count == 32, "bad sha256")
    try validate(item.size <= TransferLimits.maxItemSize, "item too large")
    total += item.size
    let textual = item.kind == .text || item.kind == .url
    try validate(!(textual && item.size == 0), "empty text item")
    let mustInline =
      item.kind == .url || (textual && item.size <= UInt64(TransferLimits.maxInlineBytes))
    if !item.inlineData.isEmpty {
      try validate(mustInline && UInt64(item.inlineData.count) == item.size, "bad inline data")
      try validate(
        CryptoPrimitives.sha256(item.inlineData) == item.sha256, "inline data hash mismatch")
    } else {
      try validate(!mustInline, "missing inline data")
    }
    if item.kind == .url {
      guard let url = decodeUTF8Strictly(item.inlineData) else {
        throw ProtocolError("url is not UTF-8")
      }
      let lower = url.lowercased()
      try validate(
        lower.hasPrefix("http://") || lower.hasPrefix("https://"), "url scheme not allowed")
    } else if item.kind == .text && !item.inlineData.isEmpty {
      try validate(decodeUTF8Strictly(item.inlineData) != nil, "text is not UTF-8")
    }
  }
  try validate(total <= limits.maxTotalSize, "offer too large")
}

/// Transfer state machine over an established SecureChannel
/// (PROTOCOL.md §8). Sans-IO: feed decoded frames to `handle(_:now:)`, call
/// `poll(now:)` regularly, and send whatever `nextFrame(now:)` returns until
/// it returns nil. Not thread-safe; use from one queue.
public final class TransferSession {
  private enum OutgoingState { case offered, sending, awaitingDone }
  private enum IncomingState { case pending, receiving }

  private struct Outgoing {
    var transferID: Bytes
    var items: [(UInt32, OutgoingItem)]
    var state = OutgoingState.offered
    var offeredAt: TimeInterval
    var queue: [(UInt32, OutgoingItem)] = []
    var offset: UInt64 = 0
  }

  private struct Incoming {
    var offer: Offer
    var state = IncomingState.pending
    var receivedAt: TimeInterval
    var queue: [OfferItem] = []
    var received: UInt64 = 0
    var hasher: SHA256Digest?
  }

  public let limits: ReceiveLimits
  private let random: (Int) -> Bytes
  private var control: [Frame] = []
  private var outgoing: Outgoing?
  private var incoming: Incoming?
  private var finishedIDs = Set<Bytes>()
  private var lastReceived: TimeInterval
  private var lastSent: TimeInterval
  private var pingSequence: UInt64 = 0

  /// `sendConfirmation` must be set on the responder (PROTOCOL.md §4.3).
  public init(
    now: TimeInterval, limits: ReceiveLimits = ReceiveLimits(), sendConfirmation: Bool = false,
    random: ((Int) -> Bytes)? = nil
  ) {
    self.limits = limits
    self.random = random ?? CryptoPrimitives.randomBytes
    lastReceived = now
    lastSent = now
    if sendConfirmation {
      control.append(.ping(Ping(sequence: 0)))
    }
  }

  public var isIdle: Bool { outgoing == nil && incoming == nil && control.isEmpty }
  public var hasOutgoingTransfer: Bool { outgoing != nil }

  // MARK: Outgoing

  @discardableResult
  public func sendOffer(_ items: [OutgoingItem], now: TimeInterval) throws -> Bytes {
    guard outgoing == nil else { throw ProtocolError("an outgoing transfer is already active") }
    let transferID = random(TransferLimits.transferIDLength)
    let numbered = items.enumerated().map { (UInt32($0.offset + 1), $0.element) }
    let offer = Offer(
      transferID: transferID,
      items: numbered.map { id, item in
        OfferItem(
          itemID: id, kind: item.kind, name: item.name, mimeType: item.mimeType,
          size: item.size, sha256: item.sha256, inlineData: item.inlineData)
      })
    try validateOffer(offer)
    outgoing = Outgoing(transferID: transferID, items: numbered, offeredAt: now)
    control.append(.offer(offer))
    return transferID
  }

  private func startSending(_ accepted: Set<UInt32>) {
    outgoing!.state = .sending
    outgoing!.queue = outgoing!.items.filter { id, item in
      accepted.contains(id) && item.inlineData.isEmpty && item.size > 0
    }
    if outgoing!.queue.isEmpty {
      outgoing!.state = .awaitingDone
    }
  }

  private func nextChunk() throws -> Frame? {
    guard var out = outgoing, out.state == .sending else { return nil }
    let (itemID, item) = out.queue[0]
    let length = Int(min(UInt64(TransferLimits.maxChunkData), item.size - out.offset))
    guard let read = item.read else { throw ProtocolError("item has no data source") }
    let data = try read(out.offset, length)
    guard data.count == length else { throw ProtocolError("item source returned the wrong length") }
    let chunk = Chunk(transferID: out.transferID, itemID: itemID, offset: out.offset, data: data)
    out.offset += UInt64(length)
    if out.offset == item.size {
      out.queue.removeFirst()
      out.offset = 0
      if out.queue.isEmpty {
        out.state = .awaitingDone
      }
    }
    outgoing = out
    return .chunk(chunk)
  }

  // MARK: Incoming

  public func respond(
    transferID: Bytes, decision: Decision, acceptedItemIDs: [UInt32] = []
  ) throws -> [TransferEvent] {
    guard let inc = incoming, inc.offer.transferID == transferID, inc.state == .pending else {
      throw ProtocolError("no pending offer with that id")
    }
    let offered = Set(inc.offer.items.map(\.itemID))
    var ids: [UInt32] = []
    let accepted: Set<UInt32>
    switch decision {
    case .acceptSome:
      guard !acceptedItemIDs.isEmpty, Set(acceptedItemIDs).isSubset(of: offered) else {
        throw ProtocolError("bad accepted ids")
      }
      ids = acceptedItemIDs.sorted()
      accepted = Set(acceptedItemIDs)
    case .acceptAll:
      accepted = offered
    default:
      accepted = []
    }
    control.append(
      .response(Response(transferID: transferID, decision: decision, acceptedItemIDs: ids)))
    var events: [TransferEvent] = []
    if accepted.isEmpty {
      finishIncoming(&events, status: nil, decision: decision)
      return events
    }
    incoming!.state = .receiving
    for item in inc.offer.items where accepted.contains(item.itemID) {
      if !item.inlineData.isEmpty {
        events.append(.inlineItemReceived(transferID: transferID, item: item))
      } else if item.size == 0 {
        events.append(.itemReceived(transferID: transferID, item: item))
      } else {
        incoming!.queue.append(item)
      }
    }
    completeIncomingIfDone(&events)
    return events
  }

  private func completeIncomingIfDone(_ events: inout [TransferEvent]) {
    guard let inc = incoming, inc.queue.isEmpty else { return }
    control.append(.done(Done(transferID: inc.offer.transferID, status: .ok)))
    finishIncoming(&events, status: .ok)
  }

  private func finishIncoming(
    _ events: inout [TransferEvent], status: DoneStatus?, reason: CancelReason? = nil,
    decision: Decision? = nil
  ) {
    let id = incoming!.offer.transferID
    finishedIDs.insert(id)
    incoming = nil
    events.append(
      .transferFinished(
        TransferResult(
          transferID: id, isOutgoing: false, status: status, cancelReason: reason,
          decision: decision)))
  }

  private func finishOutgoing(
    _ events: inout [TransferEvent], status: DoneStatus?, reason: CancelReason? = nil,
    decision: Decision? = nil
  ) {
    let id = outgoing!.transferID
    finishedIDs.insert(id)
    outgoing = nil
    events.append(
      .transferFinished(
        TransferResult(
          transferID: id, isOutgoing: true, status: status, cancelReason: reason,
          decision: decision)))
  }

  /// Ends the incoming transfer with Done{WRITE_FAILED}.
  public func writeFailed(transferID: Bytes) throws -> [TransferEvent] {
    guard let inc = incoming, inc.offer.transferID == transferID, inc.state == .receiving else {
      throw ProtocolError("no incoming transfer with that id")
    }
    control.append(.done(Done(transferID: transferID, status: .writeFailed)))
    var events: [TransferEvent] = []
    finishIncoming(&events, status: .writeFailed)
    return events
  }

  public func cancel(transferID: Bytes, reason: CancelReason = .user) -> [TransferEvent] {
    var events: [TransferEvent] = []
    control.append(.cancel(Cancel(transferID: transferID, reason: reason)))
    if outgoing?.transferID == transferID {
      finishOutgoing(&events, status: nil, reason: reason)
    } else if incoming?.offer.transferID == transferID {
      finishIncoming(&events, status: nil, reason: reason)
    }
    return events
  }

  // MARK: Frames

  public func nextFrame(now: TimeInterval) throws -> Frame? {
    let frame: Frame?
    if !control.isEmpty {
      frame = control.removeFirst()
    } else {
      frame = try nextChunk()
    }
    if frame != nil {
      lastSent = now
    }
    return frame
  }

  public func handle(_ frame: Frame, now: TimeInterval) throws -> [TransferEvent] {
    lastReceived = now
    var events: [TransferEvent] = []
    switch frame {
    case .ping: break
    case .offer(let offer): try onOffer(offer, now: now, events: &events)
    case .response(let response): try onResponse(response, events: &events)
    case .chunk(let chunk): try onChunk(chunk, events: &events)
    case .done(let done): try onDone(done, events: &events)
    case .cancel(let cancel): onCancel(cancel, events: &events)
    }
    return events
  }

  private func onOffer(_ offer: Offer, now: TimeInterval, events: inout [TransferEvent]) throws {
    try validateOffer(offer, limits: limits)
    if finishedIDs.contains(offer.transferID) || outgoing?.transferID == offer.transferID {
      throw ProtocolError("reused transfer id")
    }
    if let inc = incoming {
      if inc.offer.transferID == offer.transferID {
        throw ProtocolError("reused transfer id")
      }
      control.append(.response(Response(transferID: offer.transferID, decision: .busy)))
      finishedIDs.insert(offer.transferID)
      return
    }
    incoming = Incoming(offer: offer, receivedAt: now)
    events.append(.offerReceived(offer))
  }

  private func onResponse(_ response: Response, events: inout [TransferEvent]) throws {
    guard let out = outgoing, out.transferID == response.transferID else {
      if finishedIDs.contains(response.transferID) { return }  // raced with a cancel or timeout
      throw ProtocolError("response for unknown transfer")
    }
    guard out.state == .offered else { throw ProtocolError("duplicate response") }
    let offered = Set(out.items.map(\.0))
    let accepted: Set<UInt32>
    switch response.decision {
    case .acceptAll:
      guard response.acceptedItemIDs.isEmpty else { throw ProtocolError("ACCEPT_ALL with item ids") }
      accepted = offered
    case .acceptSome:
      accepted = Set(response.acceptedItemIDs)
      guard !accepted.isEmpty, accepted.count == response.acceptedItemIDs.count,
        accepted.isSubset(of: offered)
      else { throw ProtocolError("bad accepted item ids") }
    case .decline, .busy, .noSpace:
      guard response.acceptedItemIDs.isEmpty else { throw ProtocolError("item ids on a refusal") }
      events.append(
        .responseReceived(
          transferID: response.transferID, decision: response.decision, acceptedItemIDs: []))
      finishOutgoing(&events, status: nil, decision: response.decision)
      return
    default:
      throw ProtocolError("unknown decision")
    }
    events.append(
      .responseReceived(
        transferID: response.transferID, decision: response.decision,
        acceptedItemIDs: accepted.sorted()))
    startSending(accepted)
  }

  private func onChunk(_ chunk: Chunk, events: inout [TransferEvent]) throws {
    guard var inc = incoming, inc.offer.transferID == chunk.transferID else {
      if finishedIDs.contains(chunk.transferID) { return }  // in flight when we cancelled
      throw ProtocolError("chunk for unknown transfer")
    }
    guard inc.state == .receiving else { throw ProtocolError("chunk before response") }
    let item = inc.queue[0]
    guard chunk.itemID == item.itemID, chunk.offset == inc.received else {
      throw ProtocolError("out-of-order chunk")
    }
    guard (1...TransferLimits.maxChunkData).contains(chunk.data.count),
      chunk.offset + UInt64(chunk.data.count) <= item.size
    else { throw ProtocolError("bad chunk length") }
    if inc.hasher == nil {
      inc.hasher = SHA256Digest()
    }
    inc.hasher!.update(chunk.data)
    inc.received += UInt64(chunk.data.count)
    events.append(
      .chunkReceived(
        transferID: chunk.transferID, itemID: chunk.itemID, offset: chunk.offset,
        data: chunk.data))
    guard inc.received == item.size else {
      incoming = inc
      return
    }
    let ok = inc.hasher!.finalize() == item.sha256
    inc.queue.removeFirst()
    inc.received = 0
    inc.hasher = nil
    incoming = inc
    if !ok {
      control.append(.done(Done(transferID: chunk.transferID, status: .hashMismatch)))
      finishIncoming(&events, status: .hashMismatch)
      return
    }
    events.append(.itemReceived(transferID: chunk.transferID, item: item))
    completeIncomingIfDone(&events)
  }

  private func onDone(_ done: Done, events: inout [TransferEvent]) throws {
    guard let out = outgoing, out.transferID == done.transferID else {
      if finishedIDs.contains(done.transferID) { return }
      throw ProtocolError("done for unknown transfer")
    }
    guard out.state != .offered else { throw ProtocolError("done before response") }
    if done.status == .ok && out.state != .awaitingDone {
      throw ProtocolError("done OK before all chunks were sent")
    }
    finishOutgoing(&events, status: done.status)
  }

  private func onCancel(_ cancel: Cancel, events: inout [TransferEvent]) {
    if outgoing?.transferID == cancel.transferID {
      finishOutgoing(&events, status: nil, reason: cancel.reason)
    } else if incoming?.offer.transferID == cancel.transferID {
      finishIncoming(&events, status: nil, reason: cancel.reason)
    }
    // Unknown ids are ignored: the transfer may already have ended.
  }

  // MARK: Timers

  /// Runs timers. Throws ProtocolError on idle timeout.
  public func poll(now: TimeInterval) throws -> [TransferEvent] {
    var events: [TransferEvent] = []
    if now - lastReceived > TransferLimits.idleTimeout {
      throw ProtocolError("idle timeout")
    }
    if let out = outgoing, out.state == .offered,
      now - out.offeredAt > TransferLimits.offerTimeout
    {
      events += cancel(transferID: out.transferID, reason: .timeout)
    }
    if let inc = incoming, inc.state == .pending,
      now - inc.receivedAt > TransferLimits.offerTimeout
    {
      events += try respond(transferID: inc.offer.transferID, decision: .decline)
    }
    if now - lastSent >= TransferLimits.pingInterval && control.isEmpty {
      pingSequence += 1
      control.append(.ping(Ping(sequence: pingSequence)))
    }
    return events
  }
}
