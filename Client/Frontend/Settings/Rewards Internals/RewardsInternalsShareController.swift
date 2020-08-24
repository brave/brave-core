// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import BraveRewards
import Static
import Shared
import ZIPFoundation
import BraveShared

private let log = Logger.browserLogger

private class RewardsInternalsSharableCell: UITableViewCell, TableViewReusable, Themeable {
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
        
        textLabel?.numberOfLines = 0
        detailTextLabel?.numberOfLines = 0
        selectedBackgroundView = UIView()
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    func applyTheme(_ theme: Theme) {
        selectedBackgroundView?.backgroundColor = theme.isDark ? UIColor(white: 0.2, alpha: 1.0) : BraveUX.braveOrange.withAlphaComponent(0.06)
    }
}

/// Bundles up a set of Rewards Internals information to share with support
class RewardsInternalsShareController: UITableViewController {
    private(set) var initiallySelectedSharables: [Int]
    
    private let rewards: BraveRewards
    
    init(rewards: BraveRewards, initiallySelectedSharables sharables: [RewardsInternalsSharable]) {
        self.rewards = rewards
        self.initiallySelectedSharables = sharables
            .compactMap { sharable in
                RewardsInternalsSharable.all.firstIndex(where: { $0.id == sharable.id })
        }
        // Ensure basic info is always selected
        if !sharables.contains(.basic), let indexOfBasic = RewardsInternalsSharable.all.firstIndex(of: .basic) {
            self.initiallySelectedSharables.insert(indexOfBasic, at: 0)
        }
        
        let tempDirectory = NSTemporaryDirectory()
        let uuid = UUID()
        dropDirectory = URL(fileURLWithPath: tempDirectory).appendingPathComponent(uuid.uuidString)
        zipPath = URL(fileURLWithPath: tempDirectory).appendingPathComponent("rewards-internals.zip")
        
        if #available(iOS 13.0, *) {
            super.init(style: .insetGrouped)
        } else {
            super.init(style: .grouped)
        }
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
            tableView.selectRow(at: IndexPath(row: index, section: 0), animated: false, scrollPosition: .none)
        }
        
        tableView.tableFooterView = progressIndiciator
        progressIndiciator.observedProgress = shareProgress
        
        progressIndiciator.sizeToFit()
        
        navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(tappedCancel))
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
        progressIndiciator.frame = CGRect(x: 16, y: progressIndiciator.frame.origin.y, width: view.bounds.width - 32, height: 16)
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
        do {
            if FileManager.default.fileExists(atPath: dropDirectory.path) {
                try FileManager.default.removeItem(at: dropDirectory)
            }
            if FileManager.default.fileExists(atPath: zipPath.path) {
                try FileManager.default.removeItem(at: zipPath)
            }
        } catch {
            log.warning("Failed to cleanup sharing Rewards Internals files: \(error)")
        }
    }
    
    private let dropDirectory: URL
    private let zipPath: URL
    
    private func share(_ senderIndexPath: IndexPath) {
        // create temp folder, zip, share
        guard let selectedIndexPaths = tableView.indexPathsForSelectedRows else { return }
        let sharables = selectedIndexPaths.map { RewardsInternalsSharable.all[$0.row] }
        
        let dateFormatter = DateFormatter().then {
            $0.dateStyle = .long
        }
        let dateAndTimeFormatter = DateFormatter().then {
            $0.dateStyle = .long
            $0.timeStyle = .long
        }
        let builder = RewardsInternalsSharableBuilder(rewards: self.rewards, dateFormatter: dateFormatter, dateAndTimeFormatter: dateAndTimeFormatter)
        do {
            if FileManager.default.fileExists(atPath: dropDirectory.path) {
                try FileManager.default.removeItem(at: dropDirectory)
            }
            try FileManager.default.createDirectory(at: dropDirectory, withIntermediateDirectories: true, attributes: nil)
            let group = DispatchGroup()
            
            isSharing = true
            for sharable in sharables {
                let sharableFolder = dropDirectory.appendingPathComponent(sharable.id)
                try FileManager.default.createDirectory(at: sharableFolder, withIntermediateDirectories: true, attributes: nil)
                group.enter()
                sharable.generator.generateFiles(at: sharableFolder.path, using: builder) { error in
                    defer { group.leave() }
                    if let error = error {
                        log.error("Failed to generate files for the Rewards Intenrnals sharable with ID: \(sharable.id). Error: \(error)")
                    }
                }
            }
            group.notify(queue: .global(qos: .userInitiated)) { [weak self] in
                guard let self = self else { return }
                do {
                    if FileManager.default.fileExists(atPath: self.zipPath.path) {
                        try FileManager.default.removeItem(at: self.zipPath)
                    }
                    try FileManager.default.zipItem(at: self.dropDirectory, to: self.zipPath, shouldKeepParent: false, compressionMethod: .deflate, progress: self.shareProgress)
                    DispatchQueue.main.async {
                        let controller = UIActivityViewController(activityItems: [self.zipPath], applicationActivities: nil)
                        controller.popoverPresentationController?.sourceView = self.tableView.cellForRow(at: senderIndexPath) ?? self.tableView
                        controller.popoverPresentationController?.permittedArrowDirections = [.up, .down]
                        controller.completionWithItemsHandler = { _, _, _, _ in
                            self.cleanup()
                        }
                        self.present(controller, animated: true)
                    }
                } catch {
                    log.error("Failed to zip directory: \(error)")
                    self.cleanup()
                }
            }
        } catch {
            log.error("Failed to make temporary directory for rewards internals sharing: \(error)")
            self.cleanup()
        }
    }
    
    // MARK: - UITableViewDataSource
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return 2
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        if section == 0 {
            return RewardsInternalsSharable.all.count
        }
        return 1 // Share button
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        if indexPath.section == 0 {
            let sharable = RewardsInternalsSharable.all[indexPath.row]
            let cell = tableView.dequeueReusableCell(for: indexPath) as RewardsInternalsSharableCell
            cell.textLabel?.text = sharable.title
            cell.detailTextLabel?.text = sharable.description
            cell.applyTheme(Theme.of(nil))
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
    
    override func tableView(_ tableView: UITableView, willDeselectRowAt indexPath: IndexPath) -> IndexPath? {
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
            share(indexPath)
        }
    }
}
