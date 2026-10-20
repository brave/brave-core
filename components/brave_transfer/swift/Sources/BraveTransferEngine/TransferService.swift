// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

public protocol TransferServiceObserver: AnyObject {
  func transferServiceStateChanged(_ service: TransferService)
  func transferServiceDevicesChanged(_ service: TransferService)
  func transferService(_ service: TransferService, didReceiveOffer offer: IncomingOfferInfo)
  /// The offer was answered, expired or was withdrawn by the sender.
  func transferService(_ service: TransferService, didRemoveOffer offerID: String)
  func transferService(_ service: TransferService, didUpdateTransfer transfer: TransferInfo)
  func transferServiceInboxChanged(_ service: TransferService)
  func transferServiceSettingsChanged(_ service: TransferService)
  /// Only sent while "Show notification on receive" is on.
  func transferService(_ service: TransferService, didReceiveItems count: Int, from deviceName: String)
  /// A chain device presented a different key than the pinned one and was
  /// rejected.
  func transferService(_ service: TransferService, didRejectKeyMismatchFrom deviceName: String)
}

extension TransferServiceObserver {
  public func transferServiceStateChanged(_ service: TransferService) {}
  public func transferServiceDevicesChanged(_ service: TransferService) {}
  public func transferService(_ service: TransferService, didReceiveOffer offer: IncomingOfferInfo) {}
  public func transferService(_ service: TransferService, didRemoveOffer offerID: String) {}
  public func transferService(_ service: TransferService, didUpdateTransfer transfer: TransferInfo) {}
  public func transferServiceInboxChanged(_ service: TransferService) {}
  public func transferServiceSettingsChanged(_ service: TransferService) {}
  public func transferService(
    _ service: TransferService, didReceiveItems count: Int, from deviceName: String
  ) {}
  public func transferService(
    _ service: TransferService, didRejectKeyMismatchFrom deviceName: String
  ) {}
}

public struct TransferServiceConfiguration {
  public enum Role {
    /// The main app: receives, saves and records items in the inbox.
    case app
    /// The share extension: only sends. Incoming offers are declined.
    case shareExtension
  }

  public var role: Role
  public var localDeviceType: DeviceType
  /// Used when Sync has no name for this device.
  public var fallbackDeviceName: String
  public var maxSendBytes: UInt64 = TransferLimits.maxItemSize

  public init(role: Role, localDeviceType: DeviceType, fallbackDeviceName: String) {
    self.role = role
    self.localDeviceType = localDeviceType
    self.fallbackDeviceName = fallbackDeviceName
  }
}

/// Brave Transfer on iPhone and iPad: trust, connections, transfers and the
/// inbox. It listens (the desktop dials, design plan §2) and is the Swift
/// counterpart of the desktop `BraveTransferService`. Platform pieces are
/// injected, so it builds and is tested on Linux. Confined to `scheduler`.
public final class TransferService {
  /// Received text up to this size is kept in the inbox; longer text is
  /// saved as a .txt file.
  public static let maxInboxTextBytes: UInt64 = 64 * 1024
  public static let maxFinishedTransfers = 50
  static let progressInterval: TimeInterval = 0.2

  private final class ConnectionInfo {
    let peer: PeerConnection
    var deviceGUID = ""
    var deviceName = ""

    init(peer: PeerConnection) {
      self.peer = peer
    }
  }

  private final class TransferRecord {
    var id = UUID().uuidString.lowercased()
    /// 0 before an outgoing offer is sent and after the transfer ends.
    var connectionID = 0
    var protocolID = Bytes()
    var deviceID = ""
    var deviceName = ""
    var direction = TransferDirection.send
    var state = TransferState.waitingForResponse
    var items: [TransferItemInfo] = []
    var bytesTransferred: UInt64 = 0
    var bytesTotal: UInt64 = 0
    var lastNotified: TimeInterval = 0

    // Outgoing, until the offer is sent.
    var outgoingItems: [OutgoingItem] = []

    // Incoming.
    var textBuffers: [UInt32: Bytes] = [:]
    var openFiles = Set<UInt32>()

    var info: TransferInfo {
      TransferInfo(
        id: id, deviceID: deviceID, deviceName: deviceName, direction: direction, state: state,
        items: items, bytesTransferred: bytesTransferred, bytesTotal: bytesTotal)
    }

    func fileKey(_ itemID: UInt32) -> String { "\(id)/\(itemID)" }
  }

  private struct PendingOffer {
    var connectionID: Int
    var offer: Offer
    var info: IncomingOfferInfo
  }

  private struct WeakObserver {
    weak var value: TransferServiceObserver?
  }

  public let configuration: TransferServiceConfiguration
  private let scheduler: TransferScheduler
  private let staticPrivateKey: Bytes
  private let trustSource: TransferTrustSource
  private let trustStore: TrustStore
  private let settingsStore: TransferSettingsStore
  private let advertiser: TransferAdvertiser
  private let fileSink: IncomingFileSink?
  private let inbox: InboxStore?

  private var observers: [WeakObserver] = []
  private var listeningAllowed = false
  private var advertisedKeys: ChainKeys?
  private var tagTimer: TransferTimer?
  private var nextConnectionID = 1
  private var connections: [Int: ConnectionInfo] = [:]
  private var transfers: [String: TransferRecord] = [:]
  private var transferOrder: [String] = []
  private var pendingOffers: [String: PendingOffer] = [:]
  private var pendingOfferOrder: [String] = []

  public init(
    configuration: TransferServiceConfiguration,
    scheduler: TransferScheduler,
    staticPrivateKey: Bytes,
    trustSource: TransferTrustSource,
    trustStore: TrustStore,
    settingsStore: TransferSettingsStore,
    advertiser: TransferAdvertiser,
    fileSink: IncomingFileSink?,
    inbox: InboxStore?
  ) {
    self.configuration = configuration
    self.scheduler = scheduler
    self.staticPrivateKey = staticPrivateKey
    self.trustSource = trustSource
    self.trustStore = trustStore
    self.settingsStore = settingsStore
    self.advertiser = advertiser
    self.fileSink = fileSink
    self.inbox = inbox
    trustSource.onChange = { [weak self] in self?.trustChanged() }
    advertiser.onStateChange = { [weak self] _ in self?.notifyServiceStateChanged() }
    advertiser.onConnection = { [weak self] stream in self?.acceptIncoming(stream) }
  }

  deinit {
    tagTimer?.cancel()
    advertiser.stop()
  }

  // MARK: Observers

  public func addObserver(_ observer: TransferServiceObserver) {
    observers.removeAll { $0.value == nil || $0.value === observer }
    observers.append(WeakObserver(value: observer))
  }

  public func removeObserver(_ observer: TransferServiceObserver) {
    observers.removeAll { $0.value == nil || $0.value === observer }
  }

  private func notify(_ body: (TransferServiceObserver) -> Void) {
    for observer in observers.compactMap(\.value) {
      body(observer)
    }
  }

  // MARK: Lifecycle

  /// Whether the embedder lets the service listen: in the app, while a scene
  /// is in the foreground and the user has opened brave://transfer once
  /// (design plan §5.2); in the extension, while its UI is up.
  public func setListeningAllowed(_ allowed: Bool) {
    guard allowed != listeningAllowed else { return }
    listeningAllowed = allowed
    updateAdvertising()
  }

  public var isListeningAllowed: Bool { listeningAllowed }

  /// Closes every connection. Active transfers end as interrupted (failed)
  /// and partial files are deleted (PROTOCOL.md §8.8).
  public func closeAllConnections() {
    for info in connections.values {
      info.peer.close()
    }
  }

  /// Whether a transfer is in flight, so the app should ask for background
  /// time before closing connections.
  public var hasActiveTransfers: Bool {
    transfers.values.contains { $0.connectionID != 0 && !$0.state.isFinal }
      || !pendingOffers.isEmpty
  }

  public var advertiserState: AdvertiserState { advertiser.state }

  public var serviceState: TransferServiceState {
    TransferServiceState(
      syncEnabled: trustSource.isSyncActive, discoveryActive: advertiser.state == .advertising)
  }

  // MARK: Devices

  public var devices: [TransferDevice] {
    guard isChainTrustActive else { return [] }
    let now = scheduler.now * 1000
    let records = trustStore.records
    return trustSource.chainDevices.compactMap { chainDevice in
      let record = records[chainDevice.guid]
      if record?.removed == true {
        return nil
      }
      var device = TransferDevice(
        id: chainDevice.guid, name: chainDevice.name, type: chainDevice.type,
        source: .syncChain, policy: record?.policy ?? .default)
      device.isConnected = readyConnection(for: chainDevice.guid) != nil
      device.lastSeenTime = device.isConnected ? now : record?.lastSeen ?? 0
      if let key = record?.staticKey {
        device.keyFingerprint = StaticKey.fingerprint(publicKey: key)
      }
      return device
    }
  }

  @discardableResult
  public func setDevicePolicy(_ policy: DevicePolicy, for deviceID: String) -> Bool {
    guard !deviceID.isEmpty else { return false }
    trustStore.update(deviceID) { $0.policy = policy }
    notify { $0.transferServiceDevicesChanged(self) }
    return true
  }

  /// Stops trusting a device and drops its pinned key.
  @discardableResult
  public func removeDevice(_ deviceID: String) -> Bool {
    guard !deviceID.isEmpty else { return false }
    trustStore.update(deviceID) {
      $0.staticKey = nil
      $0.removed = true
    }
    for info in connections.values where info.deviceGUID == deviceID {
      info.peer.close()
    }
    notify { $0.transferServiceDevicesChanged(self) }
    return true
  }

  // MARK: Sending

  public func sendText(_ text: String, to deviceID: String) -> SendResult {
    guard UInt64(text.utf8.count) <= configuration.maxSendBytes else { return .tooLarge }
    guard let item = OutgoingItems.text(text) else { return .failed }
    return send([item], to: deviceID)
  }

  /// Queues `items` for `deviceID`. File items must already be hashed (see
  /// `OutgoingItems.file`).
  public func send(_ items: [OutgoingItem], to deviceID: String) -> SendResult {
    guard !items.isEmpty, items.count <= TransferLimits.maxItems else { return .failed }
    guard items.reduce(0, { $0 + $1.size }) <= configuration.maxSendBytes else {
      return .tooLarge
    }
    guard let peer = readyConnection(for: deviceID) else { return .deviceUnavailable }
    let record = TransferRecord()
    record.deviceID = deviceID
    record.deviceName = connections[peer.id]?.deviceName ?? ""
    record.direction = .send
    record.state = .waitingForResponse
    for (index, item) in items.enumerated() {
      record.items.append(
        TransferItemInfo(
          itemID: UInt32(index + 1), kind: item.kind, name: item.name, mimeType: item.mimeType,
          size: item.size, inlineData: item.inlineData))
      record.bytesTotal += item.size
    }
    record.outgoingItems = items
    pruneFinishedTransfers()
    transfers[record.id] = record
    transferOrder.append(record.id)
    notifyTransfer(record)
    startNextSend(deviceID)
    return record.state == .failed ? .failed : .success
  }

  /// The id of the most recent transfer, for callers that need to follow the
  /// one they just queued.
  public var lastTransferID: String? { transferOrder.last }

  public func cancelTransfer(_ transferID: String) {
    guard let record = transfers[transferID], !record.state.isFinal else { return }
    if record.connectionID == 0 {
      record.outgoingItems = []
      setState(record, .cancelled)
      return
    }
    connections[record.connectionID]?.peer.cancel(transferID: record.protocolID)
  }

  public var transferList: [TransferInfo] {
    transferOrder.compactMap { transfers[$0]?.info }
  }

  public func transfer(_ id: String) -> TransferInfo? {
    transfers[id]?.info
  }

  // MARK: Receiving

  public var pendingOfferList: [IncomingOfferInfo] {
    pendingOfferOrder.compactMap { pendingOffers[$0]?.info }
  }

  @discardableResult
  public func respondToOffer(_ offerID: String, accept: Bool) -> Bool {
    guard let pending = pendingOffers[offerID] else { return false }
    removePendingOffer(offerID)
    guard let info = connections[pending.connectionID], info.peer.isReady else { return false }
    if accept {
      acceptOffer(connectionID: pending.connectionID, offer: pending.offer)
    } else {
      info.peer.respond(transferID: pending.offer.transferID, decision: .decline)
    }
    return true
  }

  // MARK: Inbox

  public func inboxPage(offset: Int, count: Int) -> InboxPage {
    inbox?.page(offset: offset, count: count) ?? InboxPage(entries: [], hasMore: false)
  }

  public func inboxEntry(_ id: String) -> InboxEntry? {
    Int64(id).flatMap { inbox?.entry(id: $0) }
  }

  public func clearHistory() -> Bool {
    guard let inbox, inbox.clear() else { return false }
    notify { $0.transferServiceInboxChanged(self) }
    return true
  }

  // MARK: Settings

  public var settings: TransferSettings {
    get { settingsStore.settings }
    set {
      settingsStore.settings = newValue
      settingsChanged()
    }
  }

  /// Call when the settings changed outside the service (for example from
  /// the other process).
  public func settingsChanged() {
    notify { $0.transferServiceSettingsChanged(self) }
    if !isChainTrustActive {
      closeAllConnections()
    }
    updateAdvertising()
    notify { $0.transferServiceDevicesChanged(self) }
  }

  // MARK: Connections

  public func acceptIncoming(_ stream: ByteStream) {
    guard isChainTrustActive, let keys = trustSource.chainKeys else {
      stream.close()
      return
    }
    let tags = keys.acceptedTags(unixTime: scheduler.now)
    let peer: PeerConnection
    do {
      peer = try PeerConnection.responder(
        id: nextConnectionID, stream: stream, scheduler: scheduler,
        staticPrivateKey: staticPrivateKey, localInfo: localPeerInfo,
        pskForTag: { tags.contains($0) ? keys.psk : nil })
    } catch {
      stream.close()
      return
    }
    nextConnectionID += 1
    peer.delegate = self
    connections[peer.id] = ConnectionInfo(peer: peer)
    peer.start()
  }

  private var isChainTrustActive: Bool {
    settingsStore.settings.autoTrustSyncChain && trustSource.isSyncActive
  }

  private var shouldAdvertise: Bool {
    guard listeningAllowed, isChainTrustActive, trustSource.chainKeys != nil else { return false }
    switch configuration.role {
    case .app: return settingsStore.settings.receiveMode != .off
    case .shareExtension: return true
    }
  }

  private var localPeerInfo: PeerInfo {
    var name = OutgoingItems.truncateUTF8(
      trustSource.localDeviceName, maxBytes: TransferProtocol.maxDeviceNameBytes)
    if name.isEmpty {
      name = OutgoingItems.truncateUTF8(
        configuration.fallbackDeviceName, maxBytes: TransferProtocol.maxDeviceNameBytes)
    }
    let capabilities: Capabilities = configuration.role == .app ? [.receive, .send] : [.send]
    return PeerInfo(
      deviceName: name.isEmpty ? "Brave" : name, deviceType: configuration.localDeviceType,
      syncDeviceGUID: trustSource.localDeviceGUID, capabilities: capabilities)
  }

  private func trustChanged() {
    if !isChainTrustActive {
      closeAllConnections()
    } else {
      pruneTrustedDevices()
    }
    updateAdvertising()
    notifyServiceStateChanged()
    notify { $0.transferServiceDevicesChanged(self) }
  }

  private func updateAdvertising() {
    guard shouldAdvertise, let keys = trustSource.chainKeys else {
      stopAdvertising()
      return
    }
    if advertisedKeys == keys, advertiser.state != .stopped {
      return
    }
    stopAdvertising()
    advertisedKeys = keys
    advertiser.start(txtRecord: keys.txtRecord(unixTime: scheduler.now))
    scheduleTagRotation()
    notifyServiceStateChanged()
  }

  private func stopAdvertising() {
    tagTimer?.cancel()
    tagTimer = nil
    advertisedKeys = nil
    if advertiser.state != .stopped {
      advertiser.stop()
      notifyServiceStateChanged()
    }
  }

  private func scheduleTagRotation() {
    tagTimer?.cancel()
    tagTimer = scheduler.schedule(
      after: TagSchedule.secondsUntilNextEpoch(unixTime: scheduler.now)
    ) { [weak self] in
      guard let self, let keys = self.advertisedKeys else { return }
      self.advertiser.updateTXTRecord(keys.txtRecord(unixTime: self.scheduler.now))
      self.scheduleTagRotation()
    }
  }

  private func pruneTrustedDevices() {
    guard trustSource.isSyncActive else { return }
    let chainGUIDs = Set(trustSource.chainDevices.map(\.guid))
    trustStore.remove(trustStore.records.keys.filter { !chainGUIDs.contains($0) })
    for info in connections.values where !info.deviceGUID.isEmpty {
      if !chainGUIDs.contains(info.deviceGUID) {
        info.peer.close()
      }
    }
  }

  private func readyConnection(for deviceGUID: String) -> PeerConnection? {
    guard !deviceGUID.isEmpty else { return nil }
    return connections.values.first { $0.deviceGUID == deviceGUID && $0.peer.isReady }?.peer
  }

  private func findTransfer(connectionID: Int, protocolID: Bytes) -> TransferRecord? {
    transfers.values.first {
      $0.connectionID == connectionID && $0.protocolID == protocolID && !$0.state.isFinal
    }
  }

  private func startNextSend(_ deviceGUID: String) {
    guard let peer = readyConnection(for: deviceGUID), !peer.hasOutgoingTransfer else { return }
    for id in transferOrder {
      guard let record = transfers[id], record.direction == .send,
        record.deviceID == deviceGUID, record.connectionID == 0,
        record.state == .waitingForResponse, !record.outgoingItems.isEmpty
      else { continue }
      let items = record.outgoingItems
      record.outgoingItems = []
      do {
        record.protocolID = try peer.sendOffer(items)
        record.connectionID = peer.id
        notifyTransfer(record)
        return
      } catch {
        setState(record, .failed)
      }
    }
  }

  // MARK: Incoming transfers

  private func handleOffer(_ info: ConnectionInfo, _ offer: Offer) {
    let settings = settingsStore.settings
    let policy = trustStore.record(for: info.deviceGUID)?.policy ?? .default
    if configuration.role == .shareExtension || fileSink == nil || inbox == nil
      || settings.receiveMode == .off || policy == .block
    {
      info.peer.respond(transferID: offer.transferID, decision: .decline)
      return
    }
    let autoAccept =
      policy == .alwaysAccept || (policy == .default && settings.autoAcceptTrusted)
    if autoAccept {
      acceptOffer(connectionID: info.peer.id, offer: offer)
      return
    }
    let offerInfo = IncomingOfferInfo(
      id: UUID().uuidString.lowercased(), deviceID: info.deviceGUID, deviceName: info.deviceName,
      items: offer.items.map(TransferItemInfo.init),
      expiryTime: (scheduler.now + TransferLimits.offerTimeout) * 1000)
    pendingOffers[offerInfo.id] = PendingOffer(
      connectionID: info.peer.id, offer: offer, info: offerInfo)
    pendingOfferOrder.append(offerInfo.id)
    notify { $0.transferService(self, didReceiveOffer: offerInfo) }
  }

  private func acceptOffer(connectionID: Int, offer: Offer) {
    guard let info = connections[connectionID], info.peer.isReady, let fileSink else { return }
    let record = TransferRecord()
    record.connectionID = connectionID
    record.protocolID = offer.transferID
    record.deviceID = info.deviceGUID
    record.deviceName = info.deviceName
    record.direction = .receive
    record.state = .inProgress
    for item in offer.items {
      record.items.append(TransferItemInfo(item))
      record.bytesTotal += item.size
    }
    pruneFinishedTransfers()
    transfers[record.id] = record
    transferOrder.append(record.id)

    for item in offer.items where item.inlineData.isEmpty {
      let textual = item.kind == .text || item.kind == .url
      if textual && item.size <= Self.maxInboxTextBytes {
        record.textBuffers[item.itemID] = []
        continue
      }
      var name = item.name
      if textual && !name.lowercased().hasSuffix(".txt") {
        name += ".txt"
      }
      let file = IncomingFile(
        key: record.fileKey(item.itemID), name: FilenameSanitizer.sanitize(name),
        kind: textual ? .file : TransferItemKind(item.kind), mimeType: item.mimeType,
        size: item.size, sha256: item.sha256)
      do {
        try fileSink.begin(file)
        record.openFiles.insert(item.itemID)
      } catch {
        abortFiles(record)
        record.connectionID = 0
        setState(record, .failed)
        info.peer.respond(transferID: offer.transferID, decision: .noSpace)
        return
      }
    }
    notifyTransfer(record)
    info.peer.respond(transferID: offer.transferID, decision: .acceptAll)
  }

  private func handleResponse(
    _ connection: PeerConnection, transferID: Bytes, decision: Decision, accepted: [UInt32]
  ) {
    guard let record = findTransfer(connectionID: connection.id, protocolID: transferID) else {
      return
    }
    if decision == .acceptSome {
      let acceptedSet = Set(accepted)
      record.bytesTotal = record.items.enumerated().reduce(0) { total, entry in
        acceptedSet.contains(UInt32(entry.offset + 1)) ? total + entry.element.size : total
      }
    }
    if decision == .acceptAll || decision == .acceptSome {
      setState(record, .inProgress)
    }
  }

  private func newInboxEntry(_ record: TransferRecord, _ item: OfferItem) -> InboxEntry {
    InboxEntry(
      receivedTime: scheduler.now * 1000, deviceGUID: record.deviceID,
      deviceName: record.deviceName, kind: TransferItemKind(item.kind), mimeType: item.mimeType,
      size: item.size, sha256: item.sha256.hexString)
  }

  private func handleInlineItem(_ connection: PeerConnection, transferID: Bytes, item: OfferItem) {
    guard let record = findTransfer(connectionID: connection.id, protocolID: transferID) else {
      return
    }
    var entry = newInboxEntry(record, item)
    entry.text = String(decoding: item.inlineData, as: UTF8.self)
    addInboxEntry(entry)
  }

  private func handleChunk(
    _ connection: PeerConnection, transferID: Bytes, itemID: UInt32, data: Bytes
  ) {
    guard let record = findTransfer(connectionID: connection.id, protocolID: transferID) else {
      return
    }
    record.bytesTransferred += UInt64(data.count)
    if record.textBuffers[itemID] != nil {
      record.textBuffers[itemID]! += data
    } else if record.openFiles.contains(itemID) {
      do {
        try fileSink?.append(key: record.fileKey(itemID), data: data)
      } catch {
        fileWriteFailed(record)
        return
      }
    }
    notifyTransfer(record, force: false)
  }

  private func handleItem(_ connection: PeerConnection, transferID: Bytes, item: OfferItem) {
    guard let record = findTransfer(connectionID: connection.id, protocolID: transferID) else {
      return
    }
    var entry = newInboxEntry(record, item)
    if let text = record.textBuffers.removeValue(forKey: item.itemID) {
      entry.text = String(decoding: text, as: UTF8.self)
      addInboxEntry(entry)
      return
    }
    guard record.openFiles.remove(item.itemID) != nil, let fileSink else { return }
    if item.kind == .text || item.kind == .url {
      entry.kind = .file
    }
    let recordID = record.id
    fileSink.commit(key: record.fileKey(item.itemID)) { [weak self] saved in
      self?.fileCommitted(recordID: recordID, entry: entry, saved: saved)
    }
  }

  private func handleFinished(_ connection: PeerConnection, _ result: TransferResult) {
    if !result.isOutgoing,
      let offerID = pendingOffers.first(where: {
        $0.value.connectionID == connection.id && $0.value.offer.transferID == result.transferID
      })?.key
    {
      removePendingOffer(offerID)
    }
    guard let record = findTransfer(connectionID: connection.id, protocolID: result.transferID)
    else { return }
    var state = TransferState.failed
    if result.status == .ok {
      state = .completed
    } else if [.decline, .busy, .noSpace].contains(result.decision) {
      state = .declined
    } else if result.cancelReason == .timeout {
      state = .expired
    } else if result.cancelReason == .user {
      state = .cancelled
    }
    if state == .completed {
      record.bytesTransferred = record.bytesTotal
    }
    if !result.isOutgoing {
      abortFiles(record)
      record.textBuffers = [:]
      if state == .completed && settingsStore.settings.notifyOnReceive {
        let count = record.items.count
        let name = record.deviceName
        notify { $0.transferService(self, didReceiveItems: count, from: name) }
      }
    }
    record.connectionID = 0
    setState(record, state)
    if result.isOutgoing {
      startNextSend(record.deviceID)
    }
  }

  private func fileWriteFailed(_ record: TransferRecord) {
    guard !record.state.isFinal else { return }
    connections[record.connectionID]?.peer.writeFailed(transferID: record.protocolID)
    if !record.state.isFinal {
      abortFiles(record)
      record.connectionID = 0
      setState(record, .failed)
    }
  }

  private func fileCommitted(recordID: String, entry: InboxEntry, saved: SavedFile?) {
    let record = transfers[recordID]
    guard let saved else {
      if let record, record.state != .failed {
        record.connectionID = 0
        setState(record, .failed)
      }
      return
    }
    var entry = entry
    entry.filePath = saved.url?.path
    entry.fileName = saved.fileName
    entry.savedToPhotos = saved.savedToPhotos
    entry.savedToFallback = saved.usedFallback
    entry.thumbnailURL = saved.thumbnailURL
    addInboxEntry(entry)
  }

  private func addInboxEntry(_ entry: InboxEntry) {
    guard inbox?.add(entry) != nil else { return }
    notify { $0.transferServiceInboxChanged(self) }
  }

  private func abortFiles(_ record: TransferRecord) {
    for itemID in record.openFiles {
      fileSink?.abort(key: record.fileKey(itemID))
    }
    record.openFiles = []
  }

  private func failTransfers(connectionID: Int) {
    for id in transferOrder {
      guard let record = transfers[id], record.connectionID == connectionID,
        !record.state.isFinal
      else { continue }
      abortFiles(record)
      record.textBuffers = [:]
      record.connectionID = 0
      setState(record, .failed)
    }
  }

  // MARK: Notifications

  private func setState(_ record: TransferRecord, _ state: TransferState) {
    record.state = state
    notifyTransfer(record)
  }

  private func notifyTransfer(_ record: TransferRecord, force: Bool = true) {
    let now = scheduler.now
    if !force && now - record.lastNotified < Self.progressInterval {
      return
    }
    record.lastNotified = now
    let info = record.info
    notify { $0.transferService(self, didUpdateTransfer: info) }
  }

  private func pruneFinishedTransfers() {
    var finished = transferOrder.filter { transfers[$0]?.state.isFinal == true }.count
    transferOrder.removeAll { id in
      guard finished > Self.maxFinishedTransfers, transfers[id]?.state.isFinal == true else {
        return false
      }
      transfers.removeValue(forKey: id)
      finished -= 1
      return true
    }
  }

  private func removePendingOffer(_ offerID: String) {
    guard pendingOffers.removeValue(forKey: offerID) != nil else { return }
    pendingOfferOrder.removeAll { $0 == offerID }
    notify { $0.transferService(self, didRemoveOffer: offerID) }
  }

  private func notifyServiceStateChanged() {
    notify { $0.transferServiceStateChanged(self) }
  }
}

extension TransferService: PeerConnectionDelegate {
  public func peerConnectionDidAuthenticate(_ connection: PeerConnection) -> Bool {
    guard let info = connections[connection.id], isChainTrustActive,
      let remoteInfo = connection.remoteInfo, let remoteKey = connection.remoteStaticKey
    else { return false }
    let guid = remoteInfo.syncDeviceGUID
    guard !guid.isEmpty, guid != trustSource.localDeviceGUID,
      let device = trustSource.chainDevices.first(where: { $0.guid == guid })
    else { return false }
    if connections.contains(where: {
      $0.key != connection.id && $0.value.deviceGUID == guid && !$0.value.peer.isClosed
    }) {
      return false
    }
    if let record = trustStore.record(for: guid) {
      if record.removed {
        return false
      }
      if let pinned = record.staticKey, pinned != remoteKey {
        notify { $0.transferService(self, didRejectKeyMismatchFrom: device.name) }
        return false
      }
    }
    info.deviceGUID = guid
    info.deviceName = device.name
    return true
  }

  public func peerConnectionDidBecomeReady(_ connection: PeerConnection) {
    guard let info = connections[connection.id], let key = connection.remoteStaticKey else {
      return
    }
    // Pinned only once both sides accepted the session, so a peer that
    // rejects this device is not pinned.
    let now = scheduler.now * 1000
    trustStore.update(info.deviceGUID) {
      $0.staticKey = key
      $0.name = info.deviceName
      $0.lastSeen = now
    }
    notify { $0.transferServiceDevicesChanged(self) }
    startNextSend(info.deviceGUID)
  }

  public func peerConnection(_ connection: PeerConnection, didReceive event: TransferEvent) {
    guard let info = connections[connection.id] else { return }
    switch event {
    case .offerReceived(let offer):
      handleOffer(info, offer)
    case .responseReceived(let transferID, let decision, let accepted):
      handleResponse(connection, transferID: transferID, decision: decision, accepted: accepted)
    case .inlineItemReceived(let transferID, let item):
      handleInlineItem(connection, transferID: transferID, item: item)
    case .chunkReceived(let transferID, let itemID, _, let data):
      handleChunk(connection, transferID: transferID, itemID: itemID, data: data)
    case .itemReceived(let transferID, let item):
      handleItem(connection, transferID: transferID, item: item)
    case .transferFinished(let result):
      handleFinished(connection, result)
    }
  }

  public func peerConnection(
    _ connection: PeerConnection, didSendChunkOf transferID: Bytes, bytes: Int
  ) {
    guard let record = findTransfer(connectionID: connection.id, protocolID: transferID) else {
      return
    }
    record.bytesTransferred += UInt64(bytes)
    notifyTransfer(record, force: false)
  }

  public func peerConnection(_ connection: PeerConnection, didCloseWithReason reason: String) {
    guard let info = connections.removeValue(forKey: connection.id) else { return }
    failTransfers(connectionID: connection.id)
    for (id, pending) in pendingOffers where pending.connectionID == connection.id {
      removePendingOffer(id)
    }
    guard !info.deviceGUID.isEmpty else { return }
    for id in transferOrder {
      guard let record = transfers[id], record.deviceID == info.deviceGUID,
        record.connectionID == 0, !record.state.isFinal
      else { continue }
      record.outgoingItems = []
      setState(record, .failed)
    }
    if trustStore.record(for: info.deviceGUID) != nil {
      let now = scheduler.now * 1000
      trustStore.update(info.deviceGUID) { $0.lastSeen = now }
    }
    notify { $0.transferServiceDevicesChanged(self) }
  }
}
