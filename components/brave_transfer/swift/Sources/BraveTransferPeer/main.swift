// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Plays one Brave Transfer device over stdin/stdout, for interop tests with
// the Python reference peer (tools/brave_transfer). Not shipped in the app.
//
// The run ends like the reference peer's: once this side's offer has
// finished and --expect-offers incoming transfers have finished, it closes
// stdout and reads until stdin reaches EOF.

import BraveTransferCore
import Foundation

#if canImport(Glibc)
import Glibc
#elseif canImport(Darwin)
import Darwin
#endif

let usage = """
  usage: BraveTransferPeer --role initiator|responder --seed HEX [options]
    --static HEX           static X25519 private key (random by default)
    --name NAME            device name
    --guid GUID            sync device GUID
    --allow-guid GUID      only talk to these remote GUIDs (repeatable)
    --send-text TEXT       offer a text item (repeatable)
    --send-url URL         offer a URL item (repeatable)
    --send-file PATH       offer a file (repeatable)
    --accept all|first|decline
    --expect-offers N      incoming transfers to wait for
    --receive-dir DIR      where received items are written
    --report PATH          write a JSON report here
  """

func fail(_ message: String) -> Never {
  FileHandle.standardError.write(Data("BraveTransferPeer: \(message)\n".utf8))
  exit(1)
}

func log(_ message: String) {
  FileHandle.standardError.write(Data("BraveTransferPeer: \(message)\n".utf8))
}

struct Options {
  var initiator = true
  var seed = Bytes()
  var staticKey: Bytes?
  var name = "Swift peer"
  var guid = ""
  var allowedGUIDs: Set<String> = []
  var texts: [String] = []
  var urls: [String] = []
  var files: [String] = []
  var accept = "all"
  var expectOffers = 0
  var receiveDir: URL?
  var report: URL?

  init(_ arguments: [String]) {
    var args = arguments[...]
    var role: String?
    func value(_ flag: String) -> String {
      guard let v = args.popFirst() else { fail("\(flag) needs a value\n\(usage)") }
      return v
    }
    while let flag = args.popFirst() {
      switch flag {
      case "--role": role = value(flag)
      case "--seed": seed = Bytes(hexString: value(flag)) ?? []
      case "--static": staticKey = Bytes(hexString: value(flag))
      case "--name": name = value(flag)
      case "--guid": guid = value(flag)
      case "--allow-guid": allowedGUIDs.insert(value(flag))
      case "--send-text": texts.append(value(flag))
      case "--send-url": urls.append(value(flag))
      case "--send-file": files.append(value(flag))
      case "--accept": accept = value(flag)
      case "--expect-offers": expectOffers = Int(value(flag)) ?? 0
      case "--receive-dir": receiveDir = URL(fileURLWithPath: value(flag))
      case "--report": report = URL(fileURLWithPath: value(flag))
      default: fail("unknown argument \(flag)\n\(usage)")
      }
    }
    guard role == "initiator" || role == "responder" else { fail(usage) }
    initiator = role == "initiator"
    guard seed.count == 32 else { fail("--seed must be 32 bytes of hex") }
    guard ["all", "first", "decline"].contains(accept) else { fail(usage) }
  }
}

/// Non-blocking stdin/stdout with length-prefixed message framing.
final class StdioTransport {
  private var reader = MessageReader()
  private var inbox: [Bytes] = []
  private(set) var outBuffer = Bytes()
  private(set) var inputClosed = false
  private(set) var outputClosed = false

  init() {
    signal(SIGPIPE, SIG_IGN)
    _ = fcntl(1, F_SETFL, fcntl(1, F_GETFL) | O_NONBLOCK)
  }

  func send(_ bytes: Bytes) {
    outBuffer += bytes
  }

  func closeOutput() {
    close(1)
    outputClosed = true
  }

  func takeMessages() -> [Bytes] {
    defer { inbox.removeAll() }
    return inbox
  }

  /// Waits up to `timeout` ms for stdin to be readable or stdout writable.
  func pump(timeout: Int32) throws {
    var fds: [pollfd] = []
    if !inputClosed {
      fds.append(pollfd(fd: 0, events: Int16(POLLIN), revents: 0))
    }
    if !outBuffer.isEmpty && !outputClosed {
      fds.append(pollfd(fd: 1, events: Int16(POLLOUT), revents: 0))
    }
    guard !fds.isEmpty else { return }
    if poll(&fds, nfds_t(fds.count), timeout) < 0 {
      if errno == EINTR { return }
      throw ProtocolError("poll failed: \(errno)")
    }
    for fd in fds where fd.revents != 0 {
      if fd.fd == 0 {
        var buffer = Bytes(repeating: 0, count: 1 << 16)
        let n = read(0, &buffer, buffer.count)
        if n > 0 {
          inbox += reader.feed(Array(buffer[..<n]))
        } else if n == 0 {
          inputClosed = true
          if reader.hasPartialMessage { throw ProtocolError("connection closed mid-message") }
        } else if errno != EAGAIN && errno != EINTR {
          throw ProtocolError("read failed: \(errno)")
        }
      } else {
        let n = outBuffer.withUnsafeBytes { write(1, $0.baseAddress, $0.count) }
        if n > 0 {
          outBuffer.removeFirst(n)
        } else if n < 0 && errno != EAGAIN && errno != EINTR {
          throw ProtocolError("write failed: \(errno)")
        }
      }
    }
  }

  func flush() throws {
    while !outBuffer.isEmpty {
      try pump(timeout: -1)
    }
  }

  func readMessage() throws -> Bytes {
    while inbox.isEmpty {
      if inputClosed { throw ProtocolError("connection closed during handshake") }
      try pump(timeout: -1)
    }
    return inbox.removeFirst()
  }
}

func fileItem(_ path: String) throws -> OutgoingItem {
  let url = URL(fileURLWithPath: path)
  let handle = try FileHandle(forReadingFrom: url)
  var digest = SHA256Digest()
  var size: UInt64 = 0
  while let block = try handle.read(upToCount: 1 << 20), !block.isEmpty {
    digest.update(Bytes(block))
    size += UInt64(block.count)
  }
  return OutgoingItem(
    kind: .file, name: url.lastPathComponent, mimeType: "application/octet-stream", size: size,
    sha256: digest.finalize()
  ) { offset, length in
    try handle.seek(toOffset: offset)
    return Bytes(try handle.read(upToCount: length) ?? Data())
  }
}

/// Writes received items and records what happened, in the same JSON shape
/// as the Python reference peer's report.
final class Receiver {
  private let directory: URL?
  private var openFiles: [String: (final: URL, part: URL, handle: FileHandle)] = [:]
  private var names: [String: String] = [:]
  var received: [[String: Any]] = []
  var finished: [[String: Any]] = []

  init(directory: URL?) {
    self.directory = directory
    if let directory {
      try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    }
  }

  private func key(_ transferID: Bytes, _ itemID: UInt32) -> String {
    "\(transferID.hexString)/\(itemID)"
  }

  private func uniquePath(_ name: String) -> URL {
    let directory = directory!
    let dot = name.firstIndex(of: ".")
    let stem = dot.map { String(name[..<$0]) } ?? name
    let ext = dot.map { String(name[$0...]) } ?? ""
    var candidate = directory.appendingPathComponent(name)
    var n = 1
    let fm = FileManager.default
    while fm.fileExists(atPath: candidate.path)
      || fm.fileExists(atPath: candidate.path + ".part")
    {
      candidate = directory.appendingPathComponent("\(stem) (\(n))\(ext)")
      n += 1
    }
    return candidate
  }

  private func record(_ transferID: Bytes, _ item: OfferItem, savedAs: URL?, inline: String?) {
    received.append([
      "transfer_id": transferID.hexString, "item_id": item.itemID, "kind": item.kind.rawValue,
      "name": item.name, "size": item.size, "sha256": item.sha256.hexString,
      "saved_as": savedAs?.path ?? NSNull(), "inline_data": inline ?? NSNull(),
    ])
  }

  func saveWhole(_ transferID: Bytes, _ item: OfferItem, _ data: Bytes) throws {
    let inline =
      item.kind == .text || item.kind == .url ? String(decoding: data, as: UTF8.self) : nil
    var saved: URL?
    if directory != nil {
      saved = uniquePath(FilenameSanitizer.sanitize(item.name))
      try Data(data).write(to: saved!)
    }
    record(transferID, item, savedAs: saved, inline: inline)
  }

  func offerReceived(_ offer: Offer) {
    for item in offer.items {
      names[key(offer.transferID, item.itemID)] = item.name
    }
  }

  func writeChunk(_ transferID: Bytes, _ itemID: UInt32, _ data: Bytes) throws {
    guard directory != nil else { return }
    let k = key(transferID, itemID)
    if openFiles[k] == nil {
      let final = uniquePath(FilenameSanitizer.sanitize(names[k] ?? ""))
      let part = URL(fileURLWithPath: final.path + ".part")
      guard FileManager.default.createFile(atPath: part.path, contents: nil) else {
        throw ProtocolError("cannot create \(part.path)")
      }
      openFiles[k] = (final, part, try FileHandle(forWritingTo: part))
    }
    try openFiles[k]!.handle.write(contentsOf: Data(data))
  }

  func finishItem(_ transferID: Bytes, _ item: OfferItem) throws {
    if let file = openFiles.removeValue(forKey: key(transferID, item.itemID)) {
      try file.handle.close()
      try FileManager.default.moveItem(at: file.part, to: file.final)
      record(transferID, item, savedAs: file.final, inline: nil)
    } else if item.size == 0 {
      try saveWhole(transferID, item, [])
    } else {
      record(transferID, item, savedAs: nil, inline: nil)
    }
  }

  func transferFinished(_ result: TransferResult) {
    for (k, file) in openFiles where k.hasPrefix(result.transferID.hexString + "/") {
      try? file.handle.close()
      try? FileManager.default.removeItem(at: file.part)
      openFiles.removeValue(forKey: k)
    }
    finished.append([
      "transfer_id": result.transferID.hexString, "outgoing": result.isOutgoing,
      "status": result.status?.rawValue ?? NSNull(),
      "cancel_reason": result.cancelReason?.rawValue ?? NSNull(),
      "decision": result.decision?.rawValue ?? NSNull(),
    ])
  }
}

func run(_ options: Options) throws {
  let now = { Date().timeIntervalSince1970 }
  let keys = try ChainKeys(seed: options.seed)
  let staticKey = options.staticKey ?? StaticKey.generatePrivateKey()
  log("local fingerprint: \(StaticKey.fingerprint(publicKey: try StaticKey.publicKey(privateKey: staticKey)))")
  let info = PeerInfo(
    deviceName: options.name, deviceType: options.initiator ? .desktop : .phone,
    syncDeviceGUID: options.guid)
  let items =
    options.texts.map {
      OutgoingItem.fromBytes(kind: .text, name: "text.txt", mimeType: "text/plain", data: Bytes($0.utf8))
    }
    + options.urls.map {
      OutgoingItem.fromBytes(kind: .url, name: $0, mimeType: "text/uri-list", data: Bytes($0.utf8))
    } + (try options.files.map(fileItem))

  let io = StdioTransport()
  let channel: SecureChannel
  if options.initiator {
    let handshake = try InitiatorHandshake(
      staticPrivateKey: staticKey, psk: keys.psk,
      trustTag: keys.tag(epoch: ChainKeys.epoch(unixTime: now())), localInfo: info)
    io.send(try TransferProtocol.lengthPrefixed(handshake.message1()))
    try io.flush()
    try handshake.readMessage2(io.readMessage())
    io.send(try TransferProtocol.lengthPrefixed(handshake.message3()))
    try io.flush()
    channel = try handshake.channel()
  } else {
    let handshake = try ResponderHandshake(
      staticPrivateKey: staticKey, localInfo: info, keys: keys, unixTime: now())
    try handshake.readMessage1(io.readMessage())
    io.send(try TransferProtocol.lengthPrefixed(handshake.message2()))
    try io.flush()
    try handshake.readMessage3(io.readMessage())
    channel = try handshake.channel()
  }
  if !options.allowedGUIDs.isEmpty
    && !options.allowedGUIDs.contains(channel.remoteInfo.syncDeviceGUID)
  {
    throw ProtocolError("remote device is not in the sync chain")
  }
  log("remote fingerprint: \(StaticKey.fingerprint(publicKey: channel.remoteStaticKey))")

  let session = TransferSession(now: now(), sendConfirmation: !options.initiator)
  let receiver = Receiver(directory: options.receiveDir)
  var offerSent = false
  var outgoingDone = items.isEmpty
  var incomingFinished = 0
  var workDone: Bool { outgoingDone && incomingFinished >= options.expectOffers }

  func handle(_ events: [TransferEvent]) throws {
    for event in events {
      switch event {
      case .offerReceived(let offer):
        receiver.offerReceived(offer)
        let decision: Decision
        var ids: [UInt32] = []
        switch options.accept {
        case "all": decision = .acceptAll
        case "first":
          decision = .acceptSome
          ids = [offer.items[0].itemID]
        default: decision = .decline
        }
        try handle(session.respond(transferID: offer.transferID, decision: decision, acceptedItemIDs: ids))
      case .inlineItemReceived(let transferID, let item):
        try receiver.saveWhole(transferID, item, item.inlineData)
      case .chunkReceived(let transferID, let itemID, _, let data):
        try receiver.writeChunk(transferID, itemID, data)
      case .itemReceived(let transferID, let item):
        try receiver.finishItem(transferID, item)
      case .responseReceived:
        break
      case .transferFinished(let result):
        receiver.transferFinished(result)
        if result.isOutgoing {
          outgoingDone = true
        } else {
          incomingFinished += 1
        }
      }
    }
  }

  while true {
    if channel.isConfirmed && !io.outputClosed {
      if !offerSent && !items.isEmpty {
        try session.sendOffer(items, now: now())
        offerSent = true
      }
      while io.outBuffer.count < 1 << 18, let frame = try session.nextFrame(now: now()) {
        io.send(try channel.seal(frame))
      }
      if workDone && io.outBuffer.isEmpty {
        io.closeOutput()
      }
    }
    if io.inputClosed {
      if io.outputClosed { break }
      if !workDone { throw ProtocolError("remote closed before we finished") }
    }
    try io.pump(timeout: 1000)
    for message in io.takeMessages() {
      try handle(session.handle(channel.open(message), now: now()))
    }
    if !io.outputClosed {
      try handle(session.poll(now: now()))
    }
  }

  if let report = options.report {
    let json: [String: Any] = [
      "remote_info": [
        "protocol_version": channel.remoteInfo.protocolVersion,
        "device_name": channel.remoteInfo.deviceName,
        "device_type": channel.remoteInfo.deviceType.rawValue,
        "sync_device_guid": channel.remoteInfo.syncDeviceGUID,
        "capabilities": channel.remoteInfo.capabilities.rawValue,
      ],
      "remote_fingerprint": StaticKey.fingerprint(publicKey: channel.remoteStaticKey),
      "handshake_hash": channel.handshakeHash.hexString,
      "received": receiver.received,
      "finished": receiver.finished,
    ]
    try JSONSerialization.data(withJSONObject: json, options: [.prettyPrinted, .sortedKeys])
      .write(to: report)
  }
}

do {
  try run(Options(Array(CommandLine.arguments.dropFirst())))
} catch {
  fail("\(error)")
}
