// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation
import XCTest

@testable import BraveTransferEngine

/// A scheduler with a fake clock. Posted work runs in `runUntilIdle`.
final class ManualScheduler: TransferScheduler {
  private final class Timer: TransferTimer {
    let fireAt: TimeInterval
    let sequence: Int
    let work: () -> Void
    var isCancelled = false

    init(fireAt: TimeInterval, sequence: Int, work: @escaping () -> Void) {
      self.fireAt = fireAt
      self.sequence = sequence
      self.work = work
    }

    func cancel() { isCancelled = true }
  }

  var now: TimeInterval
  private let lock = NSLock()
  private var posted: [() -> Void] = []
  private var timers: [Timer] = []
  private var sequence = 0

  init(now: TimeInterval = 1_800_000_000) {
    self.now = now
  }

  /// Safe to call from any thread: the file sink posts from its I/O queue.
  func post(_ work: @escaping () -> Void) {
    lock.lock()
    posted.append(work)
    lock.unlock()
  }

  private func takePosted() -> (() -> Void)? {
    lock.lock()
    defer { lock.unlock() }
    return posted.isEmpty ? nil : posted.removeFirst()
  }

  @discardableResult
  func schedule(after delay: TimeInterval, _ work: @escaping () -> Void) -> TransferTimer {
    sequence += 1
    let timer = Timer(fireAt: now + delay, sequence: sequence, work: work)
    timers.append(timer)
    return timer
  }

  func runUntilIdle() {
    var guardCount = 0
    while let work = takePosted() {
      work()
      guardCount += 1
      precondition(guardCount < 1_000_000, "runaway scheduler")
    }
  }

  /// Runs a single posted task, if any.
  func runOne() {
    takePosted()?()
  }

  /// Runs posted work until `condition` holds, waiting for work posted from
  /// other threads.
  func runUntil(timeout: TimeInterval = 5, _ condition: () -> Bool) -> Bool {
    let deadline = Date().addingTimeInterval(timeout)
    while Date() < deadline {
      runUntilIdle()
      if condition() { return true }
      Thread.sleep(forTimeInterval: 0.005)
    }
    runUntilIdle()
    return condition()
  }

  /// Moves the clock forward, firing timers in order and draining posted
  /// work after each.
  func advance(by seconds: TimeInterval) {
    let end = now + seconds
    runUntilIdle()
    while true {
      timers.removeAll { $0.isCancelled }
      guard
        let next = timers.filter({ $0.fireAt <= end }).min(by: {
          ($0.fireAt, $0.sequence) < ($1.fireAt, $1.sequence)
        })
      else { break }
      timers.removeAll { $0 === next }
      now = max(now, next.fireAt)
      next.work()
      runUntilIdle()
    }
    now = end
  }
}

/// One end of an in-memory connection. Writes are delivered to the other end
/// through the scheduler, never synchronously.
final class MemoryByteStream: ByteStream {
  weak var delegate: ByteStreamDelegate?
  weak var peer: MemoryByteStream?
  private let scheduler: ManualScheduler
  private var started = false
  private var inbound: [Bytes] = []
  private var closed = false
  private(set) var bytesWritten = 0

  init(scheduler: ManualScheduler) {
    self.scheduler = scheduler
  }

  static func pair(scheduler: ManualScheduler) -> (MemoryByteStream, MemoryByteStream) {
    let a = MemoryByteStream(scheduler: scheduler)
    let b = MemoryByteStream(scheduler: scheduler)
    a.peer = b
    b.peer = a
    return (a, b)
  }

  func start() {
    started = true
    deliver()
  }

  func write(_ data: Bytes) {
    guard !closed, let peer else { return }
    bytesWritten += data.count
    scheduler.post { [weak peer] in
      peer?.receive(data)
    }
  }

  var pendingWriteBytes: Int { 0 }

  func close() {
    guard !closed else { return }
    closed = true
    scheduler.post { [weak peer] in
      peer?.remoteClosed()
    }
  }

  private func receive(_ data: Bytes) {
    guard !closed else { return }
    inbound.append(data)
    deliver()
  }

  private func deliver() {
    guard started else { return }
    while !inbound.isEmpty && !closed {
      let data = inbound.removeFirst()
      delegate?.byteStream(self, didRead: data)
    }
  }

  private func remoteClosed() {
    guard !closed else { return }
    closed = true
    delegate?.byteStream(self, didCloseWithError: nil)
  }
}

final class FakeAdvertiser: TransferAdvertiser {
  private(set) var state: AdvertiserState = .stopped {
    didSet { onStateChange?(state) }
  }
  var onStateChange: ((AdvertiserState) -> Void)?
  var onConnection: ((ByteStream) -> Void)?
  private(set) var txtRecord: [String: String] = [:]
  private(set) var startCount = 0

  func start(txtRecord: [String: String]) {
    self.txtRecord = txtRecord
    startCount += 1
    state = .advertising
  }

  func updateTXTRecord(_ txtRecord: [String: String]) {
    self.txtRecord = txtRecord
  }

  func stop() {
    state = .stopped
  }

  func simulate(_ state: AdvertiserState) {
    self.state = state
  }
}

/// Records what a finalizer was asked to save and moves files into a folder.
final class RecordingFinalizer: ReceivedFileFinalizer {
  let directory: URL
  private(set) var finalized: [IncomingFile] = []
  var fail = false

  init(directory: URL) {
    self.directory = directory
  }

  func finalize(
    _ file: IncomingFile, temporaryURL: URL, completion: @escaping (SavedFile?) -> Void
  ) {
    finalized.append(file)
    if fail {
      completion(nil)
      return
    }
    DirectoryFinalizer(directory: { self.directory }).finalize(
      file, temporaryURL: temporaryURL, completion: completion)
  }
}

final class RecordingObserver: TransferServiceObserver {
  var offers: [IncomingOfferInfo] = []
  var removedOffers: [String] = []
  var transfers: [TransferInfo] = []
  var inboxChanges = 0
  var receivedNotifications: [(Int, String)] = []
  var keyMismatches: [String] = []
  var deviceChanges = 0
  var stateChanges = 0

  func transferServiceStateChanged(_ service: TransferService) { stateChanges += 1 }
  func transferServiceDevicesChanged(_ service: TransferService) { deviceChanges += 1 }
  func transferService(_ service: TransferService, didReceiveOffer offer: IncomingOfferInfo) {
    offers.append(offer)
  }
  func transferService(_ service: TransferService, didRemoveOffer offerID: String) {
    removedOffers.append(offerID)
  }
  func transferService(_ service: TransferService, didUpdateTransfer transfer: TransferInfo) {
    transfers.append(transfer)
  }
  func transferServiceInboxChanged(_ service: TransferService) { inboxChanges += 1 }
  func transferService(
    _ service: TransferService, didReceiveItems count: Int, from deviceName: String
  ) {
    receivedNotifications.append((count, deviceName))
  }
  func transferService(
    _ service: TransferService, didRejectKeyMismatchFrom deviceName: String
  ) {
    keyMismatches.append(deviceName)
  }

  func lastState(of id: String) -> TransferState? {
    transfers.last { $0.id == id }?.state
  }
}

/// A desktop peer: dials with InitiatorHandshake and records events.
final class FakeDesktop: PeerConnectionDelegate {
  let guid: String
  let name: String
  let staticPrivateKey: Bytes
  private(set) var connection: PeerConnection?
  private(set) var events: [TransferEvent] = []
  private(set) var ready = false
  private(set) var closeReason: String?
  private(set) var chunksSent = 0
  var autoRespond: Decision?
  var received: [UInt32: Bytes] = [:]

  init(guid: String = "desktop-guid", name: String = "Mac", staticPrivateKey: Bytes? = nil) {
    self.guid = guid
    self.name = name
    self.staticPrivateKey = staticPrivateKey ?? StaticKey.generatePrivateKey()
  }

  var publicKey: Bytes { try! StaticKey.publicKey(privateKey: staticPrivateKey) }

  /// Connects to `service` over an in-memory stream with a tag for `keys`.
  func dial(
    _ service: TransferService, keys: ChainKeys, scheduler: ManualScheduler,
    tag: Bytes? = nil, psk: Bytes? = nil
  ) {
    let (local, remote) = MemoryByteStream.pair(scheduler: scheduler)
    let connection = try! PeerConnection.initiator(
      id: 1, stream: local, scheduler: scheduler, staticPrivateKey: staticPrivateKey,
      psk: psk ?? keys.psk,
      trustTag: tag ?? keys.tag(epoch: ChainKeys.epoch(unixTime: scheduler.now)),
      localInfo: PeerInfo(deviceName: name, deviceType: .desktop, syncDeviceGUID: guid))
    connection.delegate = self
    self.connection = connection
    service.acceptIncoming(remote)
    connection.start()
    scheduler.runUntilIdle()
  }

  func peerConnectionDidAuthenticate(_ connection: PeerConnection) -> Bool { true }

  func peerConnectionDidBecomeReady(_ connection: PeerConnection) {
    ready = true
  }

  func peerConnection(_ connection: PeerConnection, didReceive event: TransferEvent) {
    events.append(event)
    switch event {
    case .offerReceived(let offer):
      if let autoRespond {
        connection.respond(transferID: offer.transferID, decision: autoRespond)
      }
    case .inlineItemReceived(_, let item):
      received[item.itemID] = item.inlineData
    case .chunkReceived(_, let itemID, _, let data):
      received[itemID, default: []] += data
    default:
      break
    }
  }

  func peerConnection(_ connection: PeerConnection, didSendChunkOf transferID: Bytes, bytes: Int) {
    chunksSent += 1
  }

  func peerConnection(_ connection: PeerConnection, didCloseWithReason reason: String) {
    closeReason = reason
    ready = false
  }

  var finishedResults: [TransferResult] {
    events.compactMap {
      if case .transferFinished(let result) = $0 { return result }
      return nil
    }
  }
}

func temporaryDirectory(_ name: String = #function) -> URL {
  let url = FileManager.default.temporaryDirectory
    .appendingPathComponent("BraveTransferEngineTests", isDirectory: true)
    .appendingPathComponent(UUID().uuidString, isDirectory: true)
  try! FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
  return url
}
