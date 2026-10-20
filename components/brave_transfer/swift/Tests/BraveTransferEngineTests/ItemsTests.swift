// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation
import XCTest

@testable import BraveTransferEngine

final class OutgoingItemsTests: XCTestCase {
  func testTextOrLink() throws {
    let link = try XCTUnwrap(OutgoingItems.text(" https://brave.com/a?b=c \n"))
    XCTAssertEqual(link.kind, .url)
    XCTAssertEqual(link.inlineData, Bytes("https://brave.com/a?b=c".utf8))
    XCTAssertEqual(link.mimeType, "text/uri-list")

    XCTAssertEqual(OutgoingItems.text("see https://brave.com")?.kind, .text)
    XCTAssertEqual(OutgoingItems.text("ftp://example.com")?.kind, .text)
    XCTAssertEqual(OutgoingItems.text("https://")?.kind, .text)
    XCTAssertNil(OutgoingItems.text(""))

    let text = try XCTUnwrap(OutgoingItems.text("hello"))
    XCTAssertEqual(text.kind, .text)
    XCTAssertEqual(text.name, "text.txt")
    XCTAssertEqual(text.inlineData, Bytes("hello".utf8))

    let long = try XCTUnwrap(OutgoingItems.text(String(repeating: "x", count: 10_000)))
    XCTAssertTrue(long.inlineData.isEmpty)
    XCTAssertNotNil(long.read)
  }

  func testFileItemHashesAndReads() throws {
    let directory = temporaryDirectory()
    let url = directory.appendingPathComponent("Clip.MOV")
    let data = Bytes((0..<(3 << 20) + 17).map { UInt8(truncatingIfNeeded: $0 &* 31) })
    try Data(data).write(to: url)
    let item = try OutgoingItems.file(at: url)
    XCTAssertEqual(item.kind, .file)
    XCTAssertEqual(item.mimeType, "video/quicktime")
    XCTAssertEqual(item.name, "Clip.MOV")
    XCTAssertEqual(item.size, UInt64(data.count))
    XCTAssertEqual(item.sha256, SHA256Digest.hash(data))

    var read = Bytes()
    var offset: UInt64 = 0
    while offset < item.size {
      let length = Int(min(UInt64(TransferLimits.maxChunkData), item.size - offset))
      read += try item.read!(offset, length)
      offset += UInt64(length)
    }
    XCTAssertEqual(read, data)

    let named = try OutgoingItems.file(at: url, name: "", mimeType: "image/heic")
    XCTAssertEqual(named.name, "file")
    XCTAssertEqual(named.kind, .image)
  }

  func testTruncatesNamesOnCharacterBoundaries() {
    XCTAssertEqual(OutgoingItems.truncateUTF8("héllo", maxBytes: 2), "h")
    XCTAssertEqual(OutgoingItems.truncateUTF8("héllo", maxBytes: 3), "hé")
    XCTAssertEqual(OutgoingItems.truncateUTF8("abc", maxBytes: 10), "abc")
  }

  func testMimeTypes() {
    XCTAssertEqual(MimeTypes.guess("JPG"), "image/jpeg")
    XCTAssertEqual(MimeTypes.guess("weird"), "application/octet-stream")
    XCTAssertTrue(MimeTypes.isImageOrVideo("video/mp4"))
    XCTAssertTrue(MimeTypes.isImageOrVideo("IMAGE/PNG"))
    XCTAssertFalse(MimeTypes.isImageOrVideo("application/pdf"))
  }
}

final class FileSinkTests: XCTestCase {
  func testUniqueNames() throws {
    let directory = temporaryDirectory()
    var names: [String] = []
    for _ in 0..<3 {
      let source = directory.appendingPathComponent(UUID().uuidString)
      try Data([1]).write(to: source)
      let destination = try DirectoryFinalizer.moveToUniqueName(
        source, in: directory, name: "photo.jpg")
      names.append(destination.lastPathComponent)
    }
    XCTAssertEqual(names, ["photo.jpg", "photo (1).jpg", "photo (2).jpg"])

    let source = directory.appendingPathComponent(UUID().uuidString)
    try Data([1]).write(to: source)
    XCTAssertEqual(
      try DirectoryFinalizer.moveToUniqueName(source, in: directory, name: "README")
        .lastPathComponent, "README")
  }

  func testAbortDeletesPartialFile() throws {
    let directory = temporaryDirectory()
    let scheduler = ManualScheduler()
    let sink = TemporaryFileSink(
      temporaryDirectory: directory, finalizer: RecordingFinalizer(directory: directory),
      scheduler: scheduler)
    try sink.begin(
      IncomingFile(key: "k", name: "a", kind: .file, mimeType: "", size: 3, sha256: []))
    try sink.append(key: "k", data: [1, 2])
    XCTAssertEqual(try FileManager.default.contentsOfDirectory(atPath: directory.path).count, 1)
    sink.abort(key: "k")
    XCTAssertEqual(try FileManager.default.contentsOfDirectory(atPath: directory.path), [])
    XCTAssertThrowsError(try sink.append(key: "k", data: [3]))
  }

  func testCommitUnknownKeyFails() {
    let scheduler = ManualScheduler()
    let directory = temporaryDirectory()
    let sink = TemporaryFileSink(
      temporaryDirectory: directory, finalizer: RecordingFinalizer(directory: directory),
      scheduler: scheduler)
    var result: SavedFile?? = .none
    sink.commit(key: "missing") { result = .some($0) }
    scheduler.runUntilIdle()
    XCTAssertEqual(result, .some(nil))
  }
}

final class OutboxTests: XCTestCase {
  func testWritesAndReadsAnEntry() throws {
    let container = temporaryDirectory()
    let outbox = Outbox(appGroupContainer: container)
    let source = container.appendingPathComponent("IMG_0001.HEIC")
    let data = Bytes(repeating: 7, count: 100_000)
    try Data(data).write(to: source)

    let writer = try outbox.create(now: 1000)
    writer.addText("https://brave.com", isLink: true)
    writer.addText("a note", isLink: false)
    try writer.addFile(at: source, name: "IMG_0001.HEIC", mimeType: "image/heic")
    writer.setDevice("mac-guid")
    XCTAssertNil(outbox.manifest(for: writer.id))
    try writer.commit()

    let manifest = try XCTUnwrap(outbox.manifest(for: writer.id))
    XCTAssertEqual(manifest.deviceID, "mac-guid")
    XCTAssertEqual(manifest.items.map(\.kind), [.link, .text, .image])
    XCTAssertTrue(FileManager.default.fileExists(atPath: source.path))

    let items = try outbox.outgoingItems(for: writer.id)
    XCTAssertEqual(items.map(\.kind), [.url, .text, .image])
    XCTAssertEqual(items[2].sha256, SHA256Digest.hash(data))
    XCTAssertEqual(items[2].name, "IMG_0001.HEIC")

    outbox.remove(writer.id)
    XCTAssertNil(outbox.manifest(for: writer.id))
  }

  func testRejectsPathsOutsideTheEntry() throws {
    let container = temporaryDirectory()
    let outbox = Outbox(appGroupContainer: container)
    let writer = try outbox.create()
    try writer.commit()
    var manifest = try XCTUnwrap(outbox.manifest(for: writer.id))
    manifest.items = [
      OutboxManifest.Item(kind: .file, name: "x", mimeType: "", fileName: "../../secret")
    ]
    try JSONEncoder().encode(manifest).write(
      to: outbox.directory(for: writer.id).appendingPathComponent(Outbox.manifestName))
    XCTAssertEqual(try outbox.outgoingItems(for: writer.id).count, 0)
    XCTAssertNil(Outbox.contained("..", in: container))
    XCTAssertNil(Outbox.contained("", in: container))
    XCTAssertNotNil(Outbox.contained("0-a.jpg", in: container))
  }

  func testPurgesOldAndIncompleteEntries() throws {
    let container = temporaryDirectory()
    let outbox = Outbox(appGroupContainer: container)
    let old = try outbox.create(now: 0)
    try old.commit()
    let fresh = try outbox.create(now: Outbox.maxAge)
    try fresh.commit()
    let removed = outbox.purge(now: Outbox.maxAge + 10)
    XCTAssertEqual(removed, [old.id.uuidString])
    XCTAssertNotNil(outbox.manifest(for: fresh.id))
  }

  func testHandoffURL() throws {
    let id = UUID()
    let url = try XCTUnwrap(Outbox.handoffURL(scheme: "brave", id: id))
    XCTAssertEqual(url.absoluteString, "brave://transfer/send?outbox=\(id.uuidString)")
    XCTAssertEqual(Outbox.outboxID(fromHandoffURL: url), id)
    XCTAssertNil(Outbox.outboxID(fromHandoffURL: URL(string: "brave://transfer/x?outbox=1")!))
  }

  func testHandoffPolicy() {
    let policy = HandoffPolicy()
    XCTAssertNil(policy.reasonBeforeSending(totalBytes: 500 * 1024 * 1024))
    XCTAssertEqual(policy.reasonBeforeSending(totalBytes: 500 * 1024 * 1024 + 1), .tooLarge)
    XCTAssertEqual(policy.noDeviceTimeout, 10)
  }
}

final class DiscoveryTests: XCTestCase {
  func testSecondsUntilNextEpoch() {
    XCTAssertEqual(TagSchedule.secondsUntilNextEpoch(unixTime: 900), 901)
    XCTAssertEqual(TagSchedule.secondsUntilNextEpoch(unixTime: 1799), 2)
    let keys = try! ChainKeys(rootKey: Bytes(repeating: 4, count: 32))
    let now: TimeInterval = 1_800_000_123
    let next = now + TagSchedule.secondsUntilNextEpoch(unixTime: now)
    XCTAssertEqual(ChainKeys.epoch(unixTime: next), ChainKeys.epoch(unixTime: now) + 1)
    XCTAssertNotEqual(keys.txtRecord(unixTime: now), keys.txtRecord(unixTime: next))
  }

  func testInstanceNamesAreRandom() {
    let a = DiscoveryConstants.randomInstanceName()
    XCTAssertTrue(a.hasPrefix("brave-"))
    XCTAssertEqual(a.count, 18)
    XCTAssertNotEqual(a, DiscoveryConstants.randomInstanceName())
  }
}

final class PeerConnectionTests: XCTestCase {
  private final class Delegate: PeerConnectionDelegate {
    var closed: String?
    func peerConnectionDidAuthenticate(_ connection: PeerConnection) -> Bool { true }
    func peerConnectionDidBecomeReady(_ connection: PeerConnection) {}
    func peerConnection(_ connection: PeerConnection, didReceive event: TransferEvent) {}
    func peerConnection(
      _ connection: PeerConnection, didSendChunkOf transferID: Bytes, bytes: Int
    ) {}
    func peerConnection(_ connection: PeerConnection, didCloseWithReason reason: String) {
      closed = reason
    }
  }

  func testHandshakeTimesOut() throws {
    let scheduler = ManualScheduler()
    let (stream, _) = MemoryByteStream.pair(scheduler: scheduler)
    let delegate = Delegate()
    let connection = try PeerConnection.responder(
      id: 1, stream: stream, scheduler: scheduler,
      staticPrivateKey: StaticKey.generatePrivateKey(),
      localInfo: PeerInfo(deviceName: "x", deviceType: .phone, syncDeviceGUID: "g"),
      pskForTag: { _ in nil })
    connection.delegate = delegate
    connection.start()
    scheduler.advance(by: 9)
    XCTAssertNil(delegate.closed)
    scheduler.advance(by: 2)
    XCTAssertEqual(delegate.closed, "handshake timed out")
    XCTAssertTrue(connection.isClosed)
  }

  func testGarbageClosesTheConnection() throws {
    let scheduler = ManualScheduler()
    let (stream, remote) = MemoryByteStream.pair(scheduler: scheduler)
    let delegate = Delegate()
    let connection = try PeerConnection.responder(
      id: 1, stream: stream, scheduler: scheduler,
      staticPrivateKey: StaticKey.generatePrivateKey(),
      localInfo: PeerInfo(deviceName: "x", deviceType: .phone, syncDeviceGUID: "g"),
      pskForTag: { _ in nil })
    connection.delegate = delegate
    connection.start()
    remote.start()
    remote.write([0, 4, 1, 2, 3, 4])
    scheduler.runUntilIdle()
    XCTAssertNotNil(delegate.closed)
  }

  func testIdleConnectionTimesOut() {
    let scheduler = ManualScheduler()
    let keys = try! ChainKeys(rootKey: Bytes(repeating: 5, count: 32))
    let (a, b) = MemoryByteStream.pair(scheduler: scheduler)
    let initiatorDelegate = Delegate()
    let responderDelegate = Delegate()
    let initiator = try! PeerConnection.initiator(
      id: 1, stream: a, scheduler: scheduler, staticPrivateKey: StaticKey.generatePrivateKey(),
      psk: keys.psk, trustTag: keys.tag(epoch: ChainKeys.epoch(unixTime: scheduler.now)),
      localInfo: PeerInfo(deviceName: "Mac", deviceType: .desktop, syncDeviceGUID: "m"))
    let responder = try! PeerConnection.responder(
      id: 2, stream: b, scheduler: scheduler, staticPrivateKey: StaticKey.generatePrivateKey(),
      localInfo: PeerInfo(deviceName: "Phone", deviceType: .phone, syncDeviceGUID: "p"),
      pskForTag: { $0 == keys.tag(epoch: ChainKeys.epoch(unixTime: scheduler.now)) ? keys.psk : nil })
    initiator.delegate = initiatorDelegate
    responder.delegate = responderDelegate
    responder.start()
    initiator.start()
    scheduler.runUntilIdle()
    XCTAssertTrue(initiator.isReady)
    XCTAssertTrue(responder.isReady)
    // Pings keep both sides alive.
    scheduler.advance(by: 120)
    XCTAssertTrue(initiator.isReady)
    XCTAssertTrue(responder.isReady)
    // A side that stops hearing from its peer gives up after 30 s.
    a.delegate = nil
    scheduler.advance(by: 40)
    XCTAssertNotNil(initiatorDelegate.closed)
  }
}
