// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Shared
import Static
import UIKit

private class RewardsInternalsSharableCell: UITableViewCell, TableViewReusable {
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)

    textLabel?.numberOfLines = 0
    detailTextLabel?.numberOfLines = 0
    selectedBackgroundView = UIView()
    selectedBackgroundView?.backgroundColor = .init {
      $0.userInterfaceStyle == .dark
        ? UIColor(white: 0.2, alpha: 1.0) : UIColor.braveBlurpleTint.withAlphaComponent(0.06)
    }
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

/// Bundles up a set of Rewards Internals information to share with support
class RewardsInternalsShareController: UITableViewController {
  private(set) var initiallySelectedSharables: [Int]

  private let rewardsAPI: BraveRewardsAPI
  private let sharables: [RewardsInternalsSharable]

  init(
    rewardsAPI: BraveRewardsAPI,
    initiallySelectedSharables: [RewardsInternalsSharable],
    sharables: [RewardsInternalsSharable] = RewardsInternalsSharable.all
  ) {
    self.rewardsAPI = rewardsAPI
    self.sharables = sharables
    self.initiallySelectedSharables =
      initiallySelectedSharables
      .compactMap { sharable in
        initiallySelectedSharables.firstIndex(where: { $0.id == sharable.id })
      }
    // Ensure basic info is always selected
    if !initiallySelectedSharables.contains(.basic),
      let indexOfBasic = sharables.firstIndex(of: .basic)
    {
      self.initiallySelectedSharables.insert(indexOfBasic, at: 0)
    }

    let tempDirectory = NSTemporaryDirectory()
    let uuid = UUID()
    dropDirectory = URL(fileURLWithPath: tempDirectory).appendingPathComponent(uuid.uuidString)
    zipPath = URL(fileURLWithPath: tempDirectory).appendingPathComponent("rewards-internals.zip")

    super.init(style: .insetGrouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.RewardsInternals.shareInternalsTitle

    tableView.register(RewardsInternalsSharableCell.self)
    tableView.register(ButtonCell.self, forCellReuseIdentifier: "button")
    tableView.estimatedRowHeight = UITableView.automaticDimension

    tableView.allowsMultipleSelectionDuringEditing = true
    tableView.setEditing(true, animated: false)
    for index in initiallySelectedSharables {
      tableView.selectRow(
        at: IndexPath(row: index, section: 0),
        animated: false,
        scrollPosition: .none
      )
    }

    tableView.tableFooterView = progressIndiciator
    progressIndiciator.observedProgress = shareProgress

    progressIndiciator.sizeToFit()

    navigationItem.leftBarButtonItem = UIBarButtonItem(
      barButtonSystemItem: .cancel,
      target: self,
      action: #selector(tappedCancel)
    )
  }

  @objc private func tappedCancel() {
    shareProgress.cancel()
    dismiss(animated: true)
  }

  deinit {
    cleanup(callingFromDeinit: true)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    progressIndiciator.frame = CGRect(
      x: 16,
      y: progressIndiciator.frame.origin.y,
      width: view.bounds.width - 32,
      height: 16
    )
  }

  private let shareProgress = Progress()
  private var shareProgressObservation: NSKeyValueObservation?

  private let progressIndiciator = UIProgressView(progressViewStyle: .default)

  private func cleanup(callingFromDeinit: Bool = false) {
    if !callingFromDeinit {
      DispatchQueue.main.async {
        // Reset
        self.isSharing = false
        self.progressIndiciator.progress = 0
      }
    }
    Task { [dropDirectory, zipPath] in
      do {
        if await AsyncFileManager.default.fileExists(atPath: dropDirectory.path) {
          try await AsyncFileManager.default.removeItem(at: dropDirectory)
        }
        if await AsyncFileManager.default.fileExists(atPath: zipPath.path) {
          try await AsyncFileManager.default.removeItem(at: zipPath)
        }
      } catch {
        adsRewardsLog.warning(
          "Failed to cleanup sharing Rewards Internals files: \(error.localizedDescription)"
        )
      }
    }
  }

  private let dropDirectory: URL
  private let zipPath: URL

  private func share(_ senderIndexPath: IndexPath) async {
    // create temp folder, zip, share
    guard let selectedIndexPaths = tableView.indexPathsForSelectedRows else { return }
    let sharables = selectedIndexPaths.map { self.sharables[$0.row] }

    let dateFormatter = DateFormatter().then {
      $0.dateStyle = .long
    }
    let dateAndTimeFormatter = DateFormatter().then {
      $0.dateStyle = .long
      $0.timeStyle = .long
    }
    let builder = RewardsInternalsSharableBuilder(
      rewardsAPI: self.rewardsAPI,
      dateFormatter: dateFormatter,
      dateAndTimeFormatter: dateAndTimeFormatter
    )
    do {
      if await AsyncFileManager.default.fileExists(atPath: dropDirectory.path) {
        try await AsyncFileManager.default.removeItem(at: dropDirectory)
      }
      try await AsyncFileManager.default.createDirectory(
        at: dropDirectory,
        withIntermediateDirectories: true,
        attributes: nil
      )

      try await withThrowingTaskGroup(of: Void.self) { group in
        for sharable in sharables {
          let sharableFolder = dropDirectory.appendingPathComponent(sharable.id)
          try await AsyncFileManager.default.createDirectory(
            at: sharableFolder,
            withIntermediateDirectories: true,
            attributes: nil
          )
          try await sharable.generator.generateFiles(at: sharableFolder.path, using: builder)
        }
      }
      if await AsyncFileManager.default.fileExists(atPath: self.zipPath.path) {
        try await AsyncFileManager.default.removeItem(at: self.zipPath)
      }
      let readingIntent: NSFileAccessIntent =
        .readingIntent(with: self.dropDirectory, options: [.forUploading])
      let _: Void = try await withCheckedThrowingContinuation { c in
        NSFileCoordinator().coordinate(
          with: [readingIntent],
          queue: .init()
        ) { error in
          if let error {
            c.resume(throwing: error)
          } else {
            c.resume()
          }
        }
      }
      try await AsyncFileManager.default.moveItem(at: readingIntent.url, to: self.zipPath)
      await MainActor.run {
        let controller = UIActivityViewController(
          activityItems: [self.zipPath],
          applicationActivities: nil
        )
        controller.popoverPresentationController?.sourceView =
          self.tableView.cellForRow(at: senderIndexPath) ?? self.tableView
        controller.popoverPresentationController?.permittedArrowDirections = [.up, .down]
        controller.completionWithItemsHandler = { _, _, _, _ in
          self.cleanup()
        }
        self.present(controller, animated: true)
      }
    } catch {
      adsRewardsLog.error(
        "Failed to make temporary directory for rewards internals sharing: \(error.localizedDescription)"
      )
      self.cleanup()
    }
  }

  // MARK: - UITableViewDataSource

  override func numberOfSections(in tableView: UITableView) -> Int {
    return 2
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    if section == 0 {
      return sharables.count
    }
    return 1  // Share button
  }

  override func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    if indexPath.section == 0 {
      let sharable = sharables[indexPath.row]
      let cell = tableView.dequeueReusableCell(for: indexPath) as RewardsInternalsSharableCell
      cell.textLabel?.text = sharable.title
      cell.detailTextLabel?.text = sharable.description
      return cell
    }
    let cell = tableView.dequeueReusableCell(withIdentifier: "button", for: indexPath)
    cell.textLabel?.textAlignment = .center
    cell.textLabel?.text = Strings.RewardsInternals.share
    return cell
  }

  override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    return indexPath.section == 0
  }

  // MARK: - UITableViewDelegate

  override func tableView(
    _ tableView: UITableView,
    willDeselectRowAt indexPath: IndexPath
  ) -> IndexPath? {
    if indexPath.row == 0 && indexPath.section == 0 {
      return nil
    }
    return indexPath
  }

  private var isSharing: Bool = false
  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if indexPath.section == 1 {
      tableView.deselectRow(at: indexPath, animated: true)
      if isSharing { return }
      Task {
        isSharing = true
        await share(indexPath)
        isSharing = false
      }
    }
  }
}
