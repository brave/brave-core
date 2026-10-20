// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation
import XCTest

@testable import BraveTransferEngine

final class TransferServiceTests: XCTestCase {
  private var scheduler: ManualScheduler!
  private var rootKey: Bytes!
  private var keys: ChainKeys!
  private var trustSource: SnapshotTrustSource!
  private var trustStore: FileTrustStore!
  private var settings: InMemorySettingsStore!
  private var advertiser: FakeAdvertiser!
  private var finalizer: RecordingFinalizer!
  private var sink: TemporaryFileSink!
  private var inbox: FileInboxStore!
  private var service: TransferService!
  private var observer: RecordingObserver!
  private var directory: URL!

  private let snapshot = ChainSnapshot(
    isSyncActive: true, localDeviceGUID: "phone-guid", localDeviceName: "iPhone",
    devices: [
      ChainDevice(guid: "desktop-guid", name: "Mac", type: .desktop),
      ChainDevice(guid: "other-guid", name: "Linux box", type: .desktop),
    ])

  override func setUp() {
    super.setUp()
    directory = temporaryDirectory()
    scheduler = ManualScheduler()
    rootKey = Bytes((0..<32).map { UInt8($0) })
    keys = try! ChainKeys(rootKey: rootKey)
    trustSource = SnapshotTrustSource(snapshot: snapshot, rootKey: rootKey)
    trustStore = FileTrustStore(url: directory.appendingPathComponent("trust.json"))
    settings = InMemorySettingsStore()
    makeService(role: .app)
  }

  override func tearDown() {
    service = nil
    try? FileManager.default.removeItem(at: directory)
    super.tearDown()
  }

  private func makeService(role: TransferServiceConfiguration.Role) {
    advertiser = FakeAdvertiser()
    finalizer = RecordingFinalizer(directory: directory.appendingPathComponent("Downloads"))
    sink = TemporaryFileSink(
      temporaryDirectory: directory.appendingPathComponent("tmp"), finalizer: finalizer,
      scheduler: scheduler)
    inbox = FileInboxStore(url: nil)
    service = TransferService(
      configuration: TransferServiceConfiguration(
        role: role, localDeviceType: .phone, fallbackDeviceName: "iPhone"),
      scheduler: scheduler, staticPrivateKey: StaticKey.generatePrivateKey(),
      trustSource: trustSource, trustStore: trustStore, settingsStore: settings,
      advertiser: advertiser, fileSink: role == .app ? sink : nil,
      inbox: role == .app ? inbox : nil)
    observer = RecordingObserver()
    service.addObserver(observer)
  }

  private func connectDesktop(_ desktop: FakeDesktop = FakeDesktop()) -> FakeDesktop {
    service.setListeningAllowed(true)
    desktop.dial(service, keys: keys, scheduler: scheduler)
    return desktop
  }

  private func offer(
    from desktop: FakeDesktop, _ items: [OutgoingItem]
  ) -> Bytes {
    let id = try! desktop.connection!.sendOffer(items)
    scheduler.runUntilIdle()
    return id
  }

  // MARK: Advertising

  func testAdvertisesOnlyWhileAllowedAndSyncActive() {
    XCTAssertEqual(advertiser.state, .stopped)
    service.setListeningAllowed(true)
    XCTAssertEqual(advertiser.state, .advertising)
    XCTAssertEqual(advertiser.txtRecord, keys.txtRecord(unixTime: scheduler.now))
    XCTAssertTrue(service.serviceState.discoveryActive)
    XCTAssertTrue(service.serviceState.syncEnabled)

    trustSource.update(snapshot: snapshot, rootKey: nil)
    XCTAssertEqual(advertiser.state, .stopped)
    XCTAssertFalse(service.serviceState.syncEnabled)

    trustSource.update(snapshot: snapshot, rootKey: rootKey)
    XCTAssertEqual(advertiser.state, .advertising)

    var newSettings = settings.settings
    newSettings.autoTrustSyncChain = false
    service.settings = newSettings
    XCTAssertEqual(advertiser.state, .stopped)

    newSettings.autoTrustSyncChain = true
    newSettings.receiveMode = .off
    service.settings = newSettings
    XCTAssertEqual(advertiser.state, .stopped)

    newSettings.receiveMode = .trustedDevices
    service.settings = newSettings
    XCTAssertEqual(advertiser.state, .advertising)
    service.setListeningAllowed(false)
    XCTAssertEqual(advertiser.state, .stopped)
  }

  func testSnapshotWithoutKeyIsNotActive() {
    trustSource = SnapshotTrustSource(snapshot: snapshot, rootKey: nil)
    makeService(role: .app)
    service.setListeningAllowed(true)
    XCTAssertEqual(advertiser.state, .stopped)
    XCTAssertTrue(service.devices.isEmpty)
  }

  func testRotatesTagEachEpoch() {
    service.setListeningAllowed(true)
    let first = advertiser.txtRecord["c"]
    scheduler.advance(by: TimeInterval(ChainKeys.epochSeconds) + 2)
    XCTAssertNotEqual(advertiser.txtRecord["c"], first)
    XCTAssertEqual(advertiser.txtRecord, keys.txtRecord(unixTime: scheduler.now))
    XCTAssertEqual(advertiser.startCount, 1)
  }

  func testDeniedLocalNetworkIsReported() {
    service.setListeningAllowed(true)
    advertiser.simulate(.waiting(localNetworkDenied: true))
    XCTAssertEqual(service.advertiserState, .waiting(localNetworkDenied: true))
    XCTAssertFalse(service.serviceState.discoveryActive)
    XCTAssertGreaterThan(observer.stateChanges, 0)
  }

  // MARK: Trust

  func testChainDeviceConnectsAndIsPinned() {
    let desktop = connectDesktop()
    XCTAssertTrue(desktop.ready)
    let device = service.devices.first { $0.id == "desktop-guid" }
    XCTAssertEqual(device?.isConnected, true)
    XCTAssertEqual(device?.name, "Mac")
    XCTAssertEqual(device?.keyFingerprint, StaticKey.fingerprint(publicKey: desktop.publicKey))
    XCTAssertEqual(trustStore.record(for: "desktop-guid")?.staticKey, desktop.publicKey)
    XCTAssertEqual(service.devices.first { $0.id == "other-guid" }?.isConnected, false)

    let remote = desktop.connection!.remoteInfo!
    XCTAssertEqual(remote.syncDeviceGUID, "phone-guid")
    XCTAssertEqual(remote.deviceName, "iPhone")
    XCTAssertEqual(remote.deviceType, .phone)
    XCTAssertEqual(remote.capabilities, [.receive, .send])
  }

  func testPinsAreSharedThroughTheFile() {
    let desktop = connectDesktop()
    let other = FileTrustStore(url: directory.appendingPathComponent("trust.json"))
    XCTAssertEqual(other.record(for: "desktop-guid")?.staticKey, desktop.publicKey)
  }

  func testRejectsDeviceOutsideTheChain() {
    let desktop = connectDesktop(FakeDesktop(guid: "stranger"))
    XCTAssertFalse(desktop.ready)
    XCTAssertNotNil(desktop.closeReason)
    XCTAssertNil(trustStore.record(for: "stranger"))
  }

  func testRejectsOwnGUID() {
    let desktop = connectDesktop(FakeDesktop(guid: "phone-guid"))
    XCTAssertFalse(desktop.ready)
  }

  func testRejectsWrongChain() {
    service.setListeningAllowed(true)
    let desktop = FakeDesktop()
    let otherKeys = try! ChainKeys(rootKey: Bytes(repeating: 9, count: 32))
    desktop.dial(service, keys: otherKeys, scheduler: scheduler)
    XCTAssertFalse(desktop.ready)
    XCTAssertNotNil(desktop.closeReason)
    XCTAssertNil(trustStore.record(for: "desktop-guid"))
  }

  func testRejectsTagWithWrongPSK() {
    service.setListeningAllowed(true)
    let desktop = FakeDesktop()
    desktop.dial(
      service, keys: keys, scheduler: scheduler, psk: Bytes(repeating: 1, count: 32))
    XCTAssertFalse(desktop.ready)
  }

  func testAcceptsAdjacentEpochTag() {
    service.setListeningAllowed(true)
    let desktop = FakeDesktop()
    let previous = keys.tag(epoch: ChainKeys.epoch(unixTime: scheduler.now) - 1)
    desktop.dial(service, keys: keys, scheduler: scheduler, tag: previous)
    XCTAssertTrue(desktop.ready)
  }

  func testKeyMismatchIsRejected() {
    let first = connectDesktop()
    first.connection!.close()
    scheduler.runUntilIdle()

    let impostor = connectDesktop(FakeDesktop())
    XCTAssertFalse(impostor.ready)
    XCTAssertEqual(observer.keyMismatches, ["Mac"])
    XCTAssertEqual(trustStore.record(for: "desktop-guid")?.staticKey, first.publicKey)

    let again = connectDesktop(FakeDesktop(staticPrivateKey: first.staticPrivateKey))
    XCTAssertTrue(again.ready)
  }

  func testOnlyOneConnectionPerDevice() {
    let desktop = FakeDesktop()
    _ = connectDesktop(desktop)
    let second = connectDesktop(FakeDesktop(staticPrivateKey: desktop.staticPrivateKey))
    XCTAssertFalse(second.ready)
    XCTAssertTrue(desktop.ready)
  }

  func testRemovedDeviceIsHiddenAndRejected() {
    let desktop = connectDesktop()
    XCTAssertTrue(service.removeDevice("desktop-guid"))
    scheduler.runUntilIdle()
    XCTAssertNotNil(desktop.closeReason)
    XCTAssertNil(service.devices.first { $0.id == "desktop-guid" })
    XCTAssertNil(trustStore.record(for: "desktop-guid")?.staticKey)

    let again = connectDesktop(FakeDesktop(staticPrivateKey: desktop.staticPrivateKey))
    XCTAssertFalse(again.ready)
  }

  func testSyncOffClosesConnections() {
    let desktop = connectDesktop()
    trustSource.update(snapshot: .inactive, rootKey: nil)
    scheduler.runUntilIdle()
    XCTAssertNotNil(desktop.closeReason)
    XCTAssertTrue(service.devices.isEmpty)
    XCTAssertEqual(advertiser.state, .stopped)
    // Pins are kept, so trust resumes when Sync is back.
    XCTAssertNotNil(trustStore.record(for: "desktop-guid")?.staticKey)
  }

  func testDeviceLeavingTheChainDropsItsPin() {
    let desktop = connectDesktop()
    var smaller = snapshot
    smaller.devices.removeAll { $0.guid == "desktop-guid" }
    trustSource.update(snapshot: smaller, rootKey: rootKey)
    scheduler.runUntilIdle()
    XCTAssertNotNil(desktop.closeReason)
    XCTAssertNil(trustStore.record(for: "desktop-guid"))
  }

  func testDisconnectUpdatesLastSeen() {
    let desktop = connectDesktop()
    scheduler.advance(by: 5)
    desktop.connection!.close()
    scheduler.runUntilIdle()
    let device = service.devices.first { $0.id == "desktop-guid" }
    XCTAssertEqual(device?.isConnected, false)
    XCTAssertEqual(device?.lastSeenTime, scheduler.now * 1000)
  }

  // MARK: Receiving

  func testOfferIsPromptedAndAcceptedItemsAreSaved() throws {
    let desktop = connectDesktop()
    let fileData = Bytes((0..<150_000).map { UInt8(truncatingIfNeeded: $0 &* 7) })
    _ = offer(
      from: desktop,
      [
        .fromBytes(kind: .url, name: "https://brave.com", mimeType: "text/uri-list",
                   data: Bytes("https://brave.com".utf8)),
        .fromBytes(kind: .file, name: "report.pdf", mimeType: "application/pdf", data: fileData),
      ])
    XCTAssertEqual(observer.offers.count, 1)
    let pending = observer.offers[0]
    XCTAssertEqual(pending.deviceName, "Mac")
    XCTAssertEqual(pending.deviceID, "desktop-guid")
    XCTAssertEqual(pending.items.map(\.kind), [.link, .file])
    XCTAssertEqual(pending.items[0].text, "https://brave.com")
    XCTAssertEqual(pending.items[1].name, "report.pdf")
    XCTAssertEqual(pending.expiryTime, (scheduler.now + 60) * 1000)
    XCTAssertEqual(service.pendingOfferList, [pending])
    XCTAssertTrue(service.hasActiveTransfers)

    XCTAssertTrue(service.respondToOffer(pending.id, accept: true))
    XCTAssertTrue(scheduler.runUntil { self.inbox.page(offset: 0, count: 10).entries.count == 2 })

    XCTAssertEqual(observer.removedOffers, [pending.id])
    XCTAssertEqual(desktop.finishedResults.last?.status, .ok)
    let transfer = try XCTUnwrap(service.transferList.last)
    XCTAssertEqual(transfer.direction, .receive)
    XCTAssertEqual(transfer.state, .completed)
    XCTAssertEqual(transfer.bytesTransferred, transfer.bytesTotal)
    XCTAssertEqual(observer.receivedNotifications.first?.0, 2)
    XCTAssertEqual(observer.receivedNotifications.first?.1, "Mac")

    let entries = inbox.page(offset: 0, count: 10).entries
    let link = try XCTUnwrap(entries.first { $0.kind == .link })
    XCTAssertEqual(link.text, "https://brave.com")
    XCTAssertEqual(link.deviceName, "Mac")
    let file = try XCTUnwrap(entries.first { $0.kind == .file })
    XCTAssertEqual(file.fileName, "report.pdf")
    XCTAssertEqual(file.size, 150_000)
    XCTAssertEqual(file.sha256, SHA256Digest.hash(fileData).hexString)
    let saved = try Data(contentsOf: URL(fileURLWithPath: try XCTUnwrap(file.filePath)))
    XCTAssertEqual(Bytes(saved), fileData)
    XCTAssertEqual(finalizer.finalized.map(\.name), ["report.pdf"])
    XCTAssertEqual(sink.openFileCount, 0)
    XCTAssertFalse(service.hasActiveTransfers)
  }

  func testLongTextIsSavedAsAFile() throws {
    settings.settings.autoAcceptTrusted = true
    let desktop = connectDesktop()
    let text = String(repeating: "a", count: 70 * 1024)
    _ = offer(from: desktop, [.fromBytes(kind: .text, name: "notes", mimeType: "text/plain",
                                         data: Bytes(text.utf8))])
    XCTAssertTrue(scheduler.runUntil { self.inbox.page(offset: 0, count: 10).entries.count == 1 })
    let entry = try XCTUnwrap(inbox.page(offset: 0, count: 1).entries.first)
    XCTAssertEqual(entry.kind, .file)
    XCTAssertEqual(entry.fileName, "notes.txt")
  }

  func testMediumTextStaysInTheInbox() throws {
    settings.settings.autoAcceptTrusted = true
    let desktop = connectDesktop()
    let text = String(repeating: "b", count: 20 * 1024)
    _ = offer(from: desktop, [.fromBytes(kind: .text, name: "text.txt", mimeType: "text/plain",
                                         data: Bytes(text.utf8))])
    XCTAssertTrue(scheduler.runUntil { self.inbox.page(offset: 0, count: 10).entries.count == 1 })
    XCTAssertEqual(inbox.page(offset: 0, count: 1).entries.first?.text, text)
    XCTAssertTrue(finalizer.finalized.isEmpty)
  }

  func testDeclinedOffer() {
    let desktop = connectDesktop()
    _ = offer(from: desktop, [OutgoingItems.text("hello")!])
    XCTAssertTrue(service.respondToOffer(observer.offers[0].id, accept: false))
    scheduler.runUntilIdle()
    XCTAssertEqual(desktop.finishedResults.last?.decision, .decline)
    XCTAssertEqual(inbox.page(offset: 0, count: 10).entries.count, 0)
    XCTAssertFalse(service.respondToOffer(observer.offers[0].id, accept: true))
  }

  func testUnansweredOfferExpires() {
    let desktop = connectDesktop()
    _ = offer(from: desktop, [OutgoingItems.text("hello")!])
    scheduler.advance(by: 62)
    XCTAssertEqual(observer.removedOffers, [observer.offers[0].id])
    XCTAssertTrue(service.pendingOfferList.isEmpty)
    XCTAssertFalse(desktop.finishedResults.isEmpty)
    XCTAssertTrue(desktop.ready)
  }

  func testAutoAcceptSetting() {
    settings.settings.autoAcceptTrusted = true
    let desktop = connectDesktop()
    _ = offer(from: desktop, [OutgoingItems.text("hi")!])
    XCTAssertTrue(observer.offers.isEmpty)
    XCTAssertEqual(desktop.finishedResults.last?.status, .ok)
    XCTAssertEqual(inbox.page(offset: 0, count: 10).entries.first?.text, "hi")
  }

  func testDevicePolicies() {
    let desktop = connectDesktop()
    service.setDevicePolicy(.alwaysAccept, for: "desktop-guid")
    _ = offer(from: desktop, [OutgoingItems.text("one")!])
    XCTAssertTrue(observer.offers.isEmpty)
    XCTAssertEqual(desktop.finishedResults.last?.status, .ok)

    service.setDevicePolicy(.block, for: "desktop-guid")
    _ = offer(from: desktop, [OutgoingItems.text("two")!])
    XCTAssertEqual(desktop.finishedResults.last?.decision, .decline)

    settings.settings.autoAcceptTrusted = true
    service.setDevicePolicy(.alwaysAsk, for: "desktop-guid")
    _ = offer(from: desktop, [OutgoingItems.text("three")!])
    XCTAssertEqual(observer.offers.count, 1)
    XCTAssertEqual(service.devices.first { $0.id == "desktop-guid" }?.policy, .alwaysAsk)
  }

  func testReceiveOffDeclines() {
    let desktop = connectDesktop()
    settings.settings.receiveMode = .off
    _ = offer(from: desktop, [OutgoingItems.text("hi")!])
    XCTAssertEqual(desktop.finishedResults.last?.decision, .decline)
  }

  func testWriteFailureFailsTheTransfer() {
    settings.settings.autoAcceptTrusted = true
    finalizer.fail = true
    let desktop = connectDesktop()
    _ = offer(from: desktop, [.fromBytes(kind: .file, name: "a.bin", mimeType: "",
                                         data: Bytes(repeating: 1, count: 100_000))])
    XCTAssertTrue(scheduler.runUntil { self.service.transferList.last?.state == .failed })
    XCTAssertEqual(inbox.page(offset: 0, count: 10).entries.count, 0)
  }

  func testClosingInterruptsTransfersAndDeletesPartialFiles() {
    settings.settings.autoAcceptTrusted = true
    let desktop = connectDesktop()
    _ = try! desktop.connection!.sendOffer([
      .fromBytes(kind: .file, name: "big.bin", mimeType: "",
                 data: Bytes(repeating: 2, count: 1_000_000))
    ])
    // Deliver the offer and a few chunks, then close mid-transfer.
    for _ in 0..<6 { scheduler.runOne() }
    XCTAssertTrue(service.hasActiveTransfers)
    service.closeAllConnections()
    scheduler.runUntilIdle()
    XCTAssertEqual(service.transferList.last?.state, .failed)
    XCTAssertEqual(sink.openFileCount, 0)
    XCTAssertTrue(finalizer.finalized.isEmpty)
    let leftovers = try? FileManager.default.contentsOfDirectory(
      atPath: directory.appendingPathComponent("tmp").path)
    XCTAssertEqual(leftovers ?? [], [])
    XCTAssertNotNil(desktop.closeReason)
  }

  // MARK: Sending

  func testSendTextAsLink() throws {
    let desktop = connectDesktop()
    desktop.autoRespond = .acceptAll
    XCTAssertEqual(service.sendText("  https://brave.com/x  ", to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    let offerItems = desktop.events.compactMap { event -> [OfferItem]? in
      if case .offerReceived(let offer) = event { return offer.items }
      return nil
    }.first
    XCTAssertEqual(offerItems?.first?.kind, .url)
    XCTAssertEqual(desktop.received[1], Bytes("https://brave.com/x".utf8))
    XCTAssertEqual(service.transferList.last?.state, .completed)
    XCTAssertEqual(service.transferList.last?.direction, .send)
  }

  func testSendPlainText() {
    let desktop = connectDesktop()
    desktop.autoRespond = .acceptAll
    XCTAssertEqual(service.sendText("hello there", to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    XCTAssertEqual(desktop.received[1], Bytes("hello there".utf8))
  }

  func testSendFileStreamsFromDisk() throws {
    let desktop = connectDesktop()
    desktop.autoRespond = .acceptAll
    let data = Bytes((0..<300_000).map { UInt8(truncatingIfNeeded: $0) })
    let url = directory.appendingPathComponent("photo.jpg")
    try Data(data).write(to: url)
    let item = try OutgoingItems.file(at: url)
    XCTAssertEqual(item.kind, .image)
    XCTAssertEqual(item.mimeType, "image/jpeg")
    XCTAssertEqual(service.send([item], to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    XCTAssertEqual(desktop.received[1], data)
    let transfer = try XCTUnwrap(service.transferList.last)
    XCTAssertEqual(transfer.state, .completed)
    XCTAssertEqual(transfer.bytesTransferred, 300_000)
  }

  func testSendQueuesBehindActiveTransfer() {
    let desktop = connectDesktop()
    XCTAssertEqual(service.sendText("first", to: "desktop-guid"), .success)
    XCTAssertEqual(service.sendText("second", to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    let offers = desktop.events.filter {
      if case .offerReceived = $0 { return true }
      return false
    }
    XCTAssertEqual(offers.count, 1)
    if case .offerReceived(let offer) = offers[0] {
      desktop.connection!.respond(transferID: offer.transferID, decision: .acceptAll)
    }
    scheduler.runUntilIdle()
    XCTAssertEqual(
      desktop.events.filter {
        if case .offerReceived = $0 { return true }
        return false
      }.count, 2)
  }

  func testSendDeclinedAndUnavailable() {
    XCTAssertEqual(service.sendText("hi", to: "desktop-guid"), .deviceUnavailable)
    let desktop = connectDesktop()
    desktop.autoRespond = .decline
    XCTAssertEqual(service.sendText("hi", to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    XCTAssertEqual(service.transferList.last?.state, .declined)
    XCTAssertEqual(service.sendText("", to: "desktop-guid"), .failed)
  }

  func testCancelOutgoing() {
    let desktop = connectDesktop()
    XCTAssertEqual(service.sendText("hi", to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    service.cancelTransfer(service.lastTransferID!)
    scheduler.runUntilIdle()
    XCTAssertEqual(service.transferList.last?.state, .cancelled)
    XCTAssertEqual(desktop.finishedResults.last?.cancelReason, .user)
  }

  // MARK: Share extension

  func testShareExtensionOnlySends() {
    makeService(role: .shareExtension)
    var noReceive = settings.settings
    noReceive.receiveMode = .off
    settings.settings = noReceive
    let desktop = connectDesktop()
    XCTAssertEqual(advertiser.state, .advertising)
    XCTAssertTrue(desktop.ready)
    XCTAssertEqual(desktop.connection!.remoteInfo!.capabilities, [.send])

    _ = offer(from: desktop, [OutgoingItems.text("to the phone")!])
    XCTAssertTrue(observer.offers.isEmpty)
    XCTAssertEqual(desktop.finishedResults.last?.decision, .decline)

    desktop.autoRespond = .acceptAll
    XCTAssertEqual(service.sendText("from the share sheet", to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    XCTAssertEqual(service.transferList.last?.state, .completed)
    XCTAssertFalse(service.clearHistory())
  }

  // MARK: Inbox

  func testClearHistory() {
    settings.settings.autoAcceptTrusted = true
    let desktop = connectDesktop()
    _ = offer(from: desktop, [OutgoingItems.text("keep me")!])
    XCTAssertEqual(service.inboxPage(offset: 0, count: 10).entries.count, 1)
    let id = String(service.inboxPage(offset: 0, count: 1).entries[0].id)
    XCTAssertEqual(service.inboxEntry(id)?.text, "keep me")
    let changes = observer.inboxChanges
    XCTAssertTrue(service.clearHistory())
    XCTAssertEqual(observer.inboxChanges, changes + 1)
    XCTAssertTrue(service.inboxPage(offset: 0, count: 10).entries.isEmpty)
  }
}

