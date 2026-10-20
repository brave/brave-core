// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation
import XCTest

@testable import BraveTransferEngine

final class ShareSendFlowTests: XCTestCase {
  private var scheduler: ManualScheduler!
  private var keys: ChainKeys!
  private var trustSource: SnapshotTrustSource!
  private var settings: InMemorySettingsStore!
  private var advertiser: FakeAdvertiser!
  private var service: TransferService!
  private var flow: ShareSendFlow!
  private var snapshots: [ShareSendFlow.Snapshot] = []
  private var directory: URL!

  private let snapshot = ChainSnapshot(
    isSyncActive: true, localDeviceGUID: "phone-guid", localDeviceName: "iPhone",
    devices: [ChainDevice(guid: "desktop-guid", name: "Mac", type: .desktop)])

  override func setUp() {
    super.setUp()
    directory = temporaryDirectory()
    scheduler = ManualScheduler()
    let rootKey = Bytes((0..<32).map { UInt8($0) })
    keys = try! ChainKeys(rootKey: rootKey)
    trustSource = SnapshotTrustSource(snapshot: snapshot, rootKey: rootKey)
    settings = InMemorySettingsStore()
    advertiser = FakeAdvertiser()
    var configuration = TransferServiceConfiguration(
      role: .shareExtension, localDeviceType: .phone, fallbackDeviceName: "iPhone")
    configuration.maxSendBytes = HandoffPolicy().maxDirectBytes
    service = TransferService(
      configuration: configuration, scheduler: scheduler,
      staticPrivateKey: StaticKey.generatePrivateKey(), trustSource: trustSource,
      trustStore: FileTrustStore(url: directory.appendingPathComponent("trust.json")),
      settingsStore: settings, advertiser: advertiser, fileSink: nil, inbox: nil)
    flow = ShareSendFlow(service: service, scheduler: scheduler)
    snapshots = []
    flow.onChange = { [unowned self] in snapshots.append($0) }
  }

  override func tearDown() {
    flow = nil
    service = nil
    try? FileManager.default.removeItem(at: directory)
    super.tearDown()
  }

  private func connectDesktop() -> FakeDesktop {
    let desktop = FakeDesktop()
    desktop.dial(service, keys: keys, scheduler: scheduler)
    return desktop
  }

  func testStartsListeningOnlyAfterBegin() {
    XCTAssertEqual(flow.phase, .preparing)
    XCTAssertEqual(advertiser.state, .stopped)
    flow.begin(totalBytes: 10, localNetwork: .granted)
    XCTAssertEqual(flow.phase, .searching)
    XCTAssertEqual(advertiser.state, .advertising)
  }

  func testTooLargeHandsOffBeforeListening() {
    flow.begin(totalBytes: HandoffPolicy().maxDirectBytes + 1, localNetwork: .granted)
    XCTAssertEqual(flow.phase, .handoff(.tooLarge))
    XCTAssertEqual(advertiser.startCount, 0)
    let changes = snapshots.count
    flow.setItems([OutgoingItems.text("late")!])
    XCTAssertEqual(flow.phase, .handoff(.tooLarge))
    XCTAssertEqual(snapshots.count, changes)
  }

  func testUndeterminedLocalNetworkHandsOff() {
    flow.begin(totalBytes: 10, localNetwork: .notDetermined)
    XCTAssertEqual(flow.phase, .handoff(.localNetworkUnavailable))
    XCTAssertEqual(advertiser.startCount, 0)
  }

  func testSyncOffHandsOff() {
    trustSource.update(snapshot: .inactive, rootKey: nil)
    flow.begin(totalBytes: 10, localNetwork: .granted)
    XCTAssertEqual(flow.phase, .handoff(.notSetUp))
  }

  func testNoDeviceTimesOut() {
    flow.begin(totalBytes: 10, localNetwork: .granted)
    scheduler.advance(by: HandoffPolicy().noDeviceTimeout - 1)
    XCTAssertEqual(flow.phase, .searching)
    scheduler.advance(by: 1)
    XCTAssertEqual(flow.phase, .handoff(.noDevice))
    XCTAssertEqual(advertiser.state, .stopped)
  }

  func testDeviceStopsTheTimerAndSends() {
    flow.begin(totalBytes: 5, localNetwork: .granted)
    let desktop = connectDesktop()
    XCTAssertEqual(flow.phase, .choosing)
    XCTAssertEqual(flow.snapshot.devices.map(\.id), ["desktop-guid"])
    scheduler.advance(by: 60)
    XCTAssertEqual(flow.phase, .choosing)

    XCTAssertEqual(flow.send(to: "desktop-guid"), .failed, "items are not ready yet")
    flow.setItems([OutgoingItems.text("hello")!])
    XCTAssertTrue(flow.snapshot.isReadyToSend)

    XCTAssertEqual(flow.send(to: "desktop-guid"), .success)
    guard case .sending(let id) = flow.phase else {
      return XCTFail("expected sending, got \(flow.phase)")
    }
    XCTAssertEqual(flow.snapshot.transfer?.id, id)
    scheduler.runUntilIdle()
    XCTAssertEqual(flow.phase, .sending(transferID: id), "waiting for the computer to accept")

    let offer = desktop.events.compactMap { event -> Offer? in
      if case .offerReceived(let offer) = event { return offer }
      return nil
    }.first
    desktop.connection!.respond(transferID: offer!.transferID, decision: .acceptAll)
    scheduler.runUntilIdle()
    XCTAssertEqual(flow.phase, .finished(.completed))
    XCTAssertEqual(desktop.received[1], Bytes("hello".utf8))
    XCTAssertEqual(flow.snapshot.transfer?.state, .completed)
    XCTAssertEqual(snapshots.last?.phase, .finished(.completed))
    XCTAssertEqual(advertiser.state, .stopped)
  }

  func testDeclined() {
    flow.begin(totalBytes: 5, localNetwork: .granted)
    let desktop = connectDesktop()
    desktop.autoRespond = .decline
    flow.setItems([OutgoingItems.text("hello")!])
    XCTAssertEqual(flow.send(to: "desktop-guid"), .success)
    scheduler.runUntilIdle()
    XCTAssertEqual(flow.phase, .finished(.declined))
  }

  func testDisconnectGoesBackToSearching() {
    flow.begin(totalBytes: 5, localNetwork: .granted)
    let desktop = connectDesktop()
    XCTAssertEqual(flow.phase, .choosing)
    desktop.connection!.close()
    scheduler.runUntilIdle()
    XCTAssertEqual(flow.phase, .searching)
    XCTAssertTrue(flow.snapshot.devices.isEmpty)
  }

  func testUnavailableDeviceIsReported() {
    flow.begin(totalBytes: 5, localNetwork: .granted)
    _ = connectDesktop()
    flow.setItems([OutgoingItems.text("hello")!])
    XCTAssertEqual(flow.send(to: "someone-else"), .deviceUnavailable)
    XCTAssertEqual(flow.phase, .choosing)
    XCTAssertEqual(flow.snapshot.lastSendResult, .deviceUnavailable)
  }

  func testLocalNetworkDenialWhileSearchingHandsOff() {
    flow.begin(totalBytes: 5, localNetwork: .granted)
    advertiser.simulate(.waiting(localNetworkDenied: true))
    XCTAssertEqual(flow.phase, .handoff(.localNetworkUnavailable))
  }

  func testContinueInApp() {
    flow.begin(totalBytes: 5, localNetwork: .granted)
    flow.continueInApp()
    XCTAssertEqual(flow.phase, .handoff(.userRequested))
    XCTAssertEqual(advertiser.state, .stopped)
  }

  func testCancelStopsTheTransfer() {
    flow.begin(totalBytes: 5, localNetwork: .granted)
    let desktop = connectDesktop()
    flow.setItems([OutgoingItems.text("hello")!])
    flow.send(to: "desktop-guid")
    scheduler.runUntilIdle()
    flow.cancel()
    scheduler.runUntilIdle()
    XCTAssertEqual(service.transferList.last?.state, .cancelled)
    XCTAssertEqual(desktop.finishedResults.last?.cancelReason, .user)
    XCTAssertEqual(flow.phase, .finished(.cancelled))
  }

  func testEmptyItemsFail() {
    flow.begin(totalBytes: 0, localNetwork: .granted)
    flow.setItems([])
    XCTAssertEqual(flow.phase, .finished(.failed))
  }
}
