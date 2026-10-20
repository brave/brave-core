// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransfer
import BraveTransferUI
import SwiftUI
import UIKit
import os

/// The "Brave Transfer" share extension (design plan §5.3). Runs the Swift
/// engine only: no BraveCore or Chromium, to stay within the extension's
/// memory limit. Keeps its UI up until the transfer is done, since the
/// extension may be killed as soon as the request completes.
final class TransferShareViewController: UIViewController {
  private static let log = Logger(subsystem: "com.brave.transfer", category: "share")

  private var engine: TransferEngine?
  private var model: TransferShareModel?

  override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = .clear

    let attachments = (extensionContext?.inputItems as? [NSExtensionItem] ?? [])
      .flatMap { $0.attachments ?? [] }

    do {
      let engine = try TransferEngine(
        role: .shareExtension,
        appGroup: try TransferAppGroup.current()
      )
      let model = TransferShareModel(
        engine: engine,
        appScheme: Bundle.main.infoDictionary?["BRAVE_URL_SCHEME"] as? String ?? "brave",
        workDirectory: FileManager.default.temporaryDirectory
          .appendingPathComponent("BraveTransferShare-\(UUID().uuidString)", isDirectory: true),
        openURL: { [weak self] url in self?.openApp(url) },
        finish: { [weak self] in self?.finish() }
      )
      self.engine = engine
      self.model = model
      embed(TransferShareView(model: model))
      model.start(providers: attachments)
    } catch {
      Self.log.error("Brave Transfer is unavailable: \(String(describing: error))")
      embed(TransferShareUnavailableView(onClose: { [weak self] in self?.finish() }))
    }
  }

  private func embed(_ content: some View) {
    let host = UIHostingController(rootView: content)
    addChild(host)
    host.view.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(host.view)
    NSLayoutConstraint.activate([
      host.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      host.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      host.view.topAnchor.constraint(equalTo: view.topAnchor),
      host.view.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])
    host.didMove(toParent: self)
  }

  /// Extensions cannot use `UIApplication.shared`; the application is
  /// reached through the responder chain, as `OpenInBraveViewController`
  /// does.
  private func openApp(_ url: URL) {
    var responder: UIResponder? = self
    while let current = responder {
      if let application = current as? UIApplication {
        application.open(url, options: [:], completionHandler: nil)
        return
      }
      responder = current.next
    }
    Self.log.error("Could not open Brave for the hand-off")
  }

  private func finish() {
    engine?.perform { service in
      service.setListeningAllowed(false)
      service.closeAllConnections()
    }
    // Completing in the same turn as `openApp` can cancel the open.
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) { [weak self] in
      self?.extensionContext?.completeRequest(returningItems: [], completionHandler: nil)
    }
  }
}
