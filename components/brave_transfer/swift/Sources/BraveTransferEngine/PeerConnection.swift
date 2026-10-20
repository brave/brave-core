// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

public protocol PeerConnectionDelegate: AnyObject {
  /// The handshake finished and `remoteInfo` and `remoteStaticKey` are set.
  /// Return false to close the connection.
  func peerConnectionDidAuthenticate(_ connection: PeerConnection) -> Bool
  /// The connection can carry transfers.
  func peerConnectionDidBecomeReady(_ connection: PeerConnection)
  func peerConnection(_ connection: PeerConnection, didReceive event: TransferEvent)
  /// `bytes` of chunk data for an outgoing transfer were queued for sending.
  func peerConnection(_ connection: PeerConnection, didSendChunkOf transferID: Bytes, bytes: Int)
  /// Runs once, asynchronously, after the connection closed. No other calls
  /// follow.
  func peerConnection(_ connection: PeerConnection, didCloseWithReason reason: String)
}

/// Drives the handshake and a TransferSession over a ByteStream. The Swift
/// counterpart of the desktop `PeerConnection`
/// (components/brave_transfer/browser/peer_connection.h).
public final class PeerConnection {
  public static let handshakeTimeout: TimeInterval = 10
  public static let pollInterval: TimeInterval = 1
  public static let maxBufferedBytes = 256 * 1024

  public let id: Int
  public let isInitiator: Bool
  public weak var delegate: PeerConnectionDelegate?

  private let scheduler: TransferScheduler
  private var stream: ByteStream?
  private var initiator: InitiatorHandshake?
  private var responder: ResponderHandshake?
  private var reader = MessageReader()
  private var receivedMessage1 = false
  private var channel: SecureChannel?
  private var session: TransferSession?
  private var ready = false
  public private(set) var isClosed = false
  private var pumping = false
  private var dispatching = false
  private var pendingEvents: [TransferEvent] = []
  private var handshakeTimer: TransferTimer?
  private var pollTimer: TransferTimer?

  public static func initiator(
    id: Int, stream: ByteStream, scheduler: TransferScheduler, staticPrivateKey: Bytes,
    psk: Bytes, trustTag: Bytes, localInfo: PeerInfo
  ) throws -> PeerConnection {
    let handshake = try InitiatorHandshake(
      staticPrivateKey: staticPrivateKey, psk: psk, trustTag: trustTag, localInfo: localInfo)
    return PeerConnection(
      id: id, stream: stream, scheduler: scheduler, initiator: handshake, responder: nil)
  }

  public static func responder(
    id: Int, stream: ByteStream, scheduler: TransferScheduler, staticPrivateKey: Bytes,
    localInfo: PeerInfo, pskForTag: @escaping (Bytes) -> Bytes?
  ) throws -> PeerConnection {
    let handshake = try ResponderHandshake(
      staticPrivateKey: staticPrivateKey, localInfo: localInfo, pskForTag: pskForTag)
    return PeerConnection(
      id: id, stream: stream, scheduler: scheduler, initiator: nil, responder: handshake)
  }

  private init(
    id: Int, stream: ByteStream, scheduler: TransferScheduler, initiator: InitiatorHandshake?,
    responder: ResponderHandshake?
  ) {
    self.id = id
    self.stream = stream
    self.scheduler = scheduler
    self.initiator = initiator
    self.responder = responder
    isInitiator = initiator != nil
  }

  public func start() {
    stream?.delegate = self
    stream?.start()
    handshakeTimer = scheduler.schedule(after: Self.handshakeTimeout) { [weak self] in
      self?.fail("handshake timed out")
    }
    if let initiator {
      sendPrefixed { try initiator.message1() }
    }
  }

  public var isReady: Bool { ready && !isClosed }
  /// Valid once the handshake has finished.
  public var remoteInfo: PeerInfo? { channel?.remoteInfo }
  public var remoteStaticKey: Bytes? { channel?.remoteStaticKey }
  public var isIdle: Bool { session?.isIdle ?? true }
  public var hasOutgoingTransfer: Bool { session?.hasOutgoingTransfer ?? false }

  /// Offers `items`. Only one outgoing transfer can be active at a time.
  /// Returns the protocol transfer id.
  public func sendOffer(_ items: [OutgoingItem]) throws -> Bytes {
    guard isReady, let session else { throw ProtocolError("connection is not ready") }
    let transferID = try session.sendOffer(items, now: scheduler.now)
    pump()
    return transferID
  }

  public func respond(transferID: Bytes, decision: Decision, acceptedItemIDs: [UInt32] = []) {
    guard isReady, let session else { return }
    dispatch {
      try session.respond(
        transferID: transferID, decision: decision, acceptedItemIDs: acceptedItemIDs)
    }
    pump()
  }

  public func writeFailed(transferID: Bytes) {
    guard isReady, let session else { return }
    dispatch { try session.writeFailed(transferID: transferID) }
    pump()
  }

  public func cancel(transferID: Bytes, reason: CancelReason = .user) {
    guard isReady, let session else { return }
    dispatch { session.cancel(transferID: transferID, reason: reason) }
    pump()
  }

  /// Closes the connection. `didCloseWithReason` follows.
  public func close() {
    fail("closed locally")
  }

  // MARK: Private

  private func handleMessage(_ message: Bytes) {
    guard let channel, let session else {
      handleHandshakeMessage(message)
      return
    }
    let frame: Frame
    do {
      frame = try channel.open(message)
    } catch {
      fail("\(error)")
      return
    }
    dispatch { try session.handle(frame, now: scheduler.now) }
    if !isClosed {
      maybeMarkReady()
    }
  }

  private func handleHandshakeMessage(_ message: Bytes) {
    do {
      if let initiator {
        try initiator.readMessage2(message)
        sendPrefixed { try initiator.message3() }
        if isClosed { return }
        try handshakeComplete(initiator.channel())
        return
      }
      guard let responder else { return }
      if !receivedMessage1 {
        try responder.readMessage1(message)
        receivedMessage1 = true
        sendPrefixed { try responder.message2() }
        return
      }
      try responder.readMessage3(message)
      try handshakeComplete(responder.channel())
    } catch {
      fail("\(error)")
    }
  }

  private func handshakeComplete(_ channel: SecureChannel) {
    handshakeTimer?.cancel()
    handshakeTimer = nil
    self.channel = channel
    initiator = nil
    responder = nil
    session = TransferSession(now: scheduler.now, sendConfirmation: !isInitiator)
    guard delegate?.peerConnectionDidAuthenticate(self) == true else {
      fail("peer is not trusted")
      return
    }
    pollTimer = scheduler.scheduleRepeating(every: Self.pollInterval) { [weak self] in
      self?.poll()
    }
    maybeMarkReady()
  }

  private func maybeMarkReady() {
    guard !ready, !isClosed, let channel, channel.isConfirmed else { return }
    ready = true
    delegate?.peerConnectionDidBecomeReady(self)
  }

  private func sendPrefixed(_ message: () throws -> Bytes) {
    do {
      stream?.write(try TransferProtocol.lengthPrefixed(message()))
    } catch {
      fail("\(error)")
    }
  }

  private func pump() {
    guard !isClosed, !pumping, let session, let channel, channel.isConfirmed, let stream else {
      return
    }
    pumping = true
    defer { pumping = false }
    while stream.pendingWriteBytes < Self.maxBufferedBytes {
      let frame: Frame?
      let wire: Bytes
      do {
        frame = try session.nextFrame(now: scheduler.now)
        guard let frame else { break }
        wire = try channel.seal(frame)
      } catch {
        fail("\(error)")
        return
      }
      stream.write(wire)
      if case .chunk(let chunk) = frame {
        delegate?.peerConnection(self, didSendChunkOf: chunk.transferID, bytes: chunk.data.count)
        if isClosed { return }
      }
    }
  }

  private func poll() {
    guard !isClosed, let session else { return }
    dispatch { try session.poll(now: scheduler.now) }
    pump()
  }

  private func dispatch(_ produce: () throws -> [TransferEvent]) {
    guard !isClosed else { return }
    do {
      pendingEvents += try produce()
    } catch {
      fail("\(error)")
      return
    }
    guard !dispatching else { return }
    dispatching = true
    defer { dispatching = false }
    while !pendingEvents.isEmpty && !isClosed {
      let event = pendingEvents.removeFirst()
      delegate?.peerConnection(self, didReceive: event)
    }
  }

  private func fail(_ reason: String) {
    guard !isClosed else { return }
    isClosed = true
    ready = false
    pendingEvents.removeAll()
    handshakeTimer?.cancel()
    pollTimer?.cancel()
    handshakeTimer = nil
    pollTimer = nil
    scheduler.post { [self] in
      stream?.delegate = nil
      stream?.close()
      stream = nil
      delegate?.peerConnection(self, didCloseWithReason: reason)
    }
  }
}

extension PeerConnection: ByteStreamDelegate {
  public func byteStream(_ stream: ByteStream, didRead data: Bytes) {
    guard !isClosed else { return }
    for message in reader.feed(data) {
      handleMessage(message)
      if isClosed { return }
    }
    pump()
  }

  public func byteStreamDidDrain(_ stream: ByteStream) {
    pump()
  }

  public func byteStream(_ stream: ByteStream, didCloseWithError error: Error?) {
    guard !isClosed else { return }
    if error != nil {
      fail("connection error")
    } else if session == nil {
      fail("closed during the handshake")
    } else if !(session?.isIdle ?? true) || reader.hasPartialMessage {
      fail("closed by the peer during a transfer")
    } else {
      fail("closed by the peer")
    }
  }
}
