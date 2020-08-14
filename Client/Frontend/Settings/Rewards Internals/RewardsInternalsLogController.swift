// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import XCGLogger
import BraveUI
import BraveRewards

private let rewardsLogger = Logger.rewardsLogger
private let browserLogger = Logger.browserLogger

fileprivate class LogLineCell: UITableViewCell, TableViewReusable {
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
        textLabel?.font = .systemFont(ofSize: 12, weight: .regular)
        textLabel?.numberOfLines = 0
        detailTextLabel?.font = .systemFont(ofSize: 14, weight: .semibold)
        detailTextLabel?.numberOfLines = 0
        selectionStyle = .none
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}

class RewardsInternalsLogController: UITableViewController {
    struct LogLine {
        var metadata: String?
        var message: String
        
        init?(from line: String) {
            let logPrefix = #"^[0-9]{4}-[0-9]{2}-[0-9]{2}[^>]+"#
            if let range = line.range(of: logPrefix, options: .regularExpression, range: line.startIndex..<line.endIndex), !range.isEmpty {
                metadata = line[range].trimmingCharacters(in: .whitespacesAndNewlines)
                message = line[line.index(range.upperBound, offsetBy: 1)..<line.endIndex].trimmingCharacters(in: .whitespacesAndNewlines)
            } else {
                metadata = nil
                message = line
            }
            if metadata == nil && message.isEmpty {
                return nil
            }
        }
    }
    
    private var lines: [LogLine] = [] {
        didSet {
            lineCountToolbarLabel.text = String.localizedStringWithFormat(Strings.RewardsInternals.logsCount, lines.count)
            lineCountToolbarLabel.sizeToFit()
        }
    }
    
    private let lineCountToolbarLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 13)
        $0.adjustsFontSizeToFitWidth = true
    }
    
    private let rewards: BraveRewards
    
    init(rewards: BraveRewards) {
        self.rewards = rewards
        super.init(style: .plain)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.RewardsInternals.logsTitle
        
        tableView.register(LogLineCell.self)
        tableView.estimatedRowHeight = UITableView.automaticDimension
        
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .action, target: self, action: #selector(tappedShare)).then {
            $0.accessibilityLabel = Strings.RewardsInternals.shareInternalsTitle
        }
        toolbarItems = [
            .init(barButtonSystemItem: .refresh, target: self, action: #selector(tappedRefreshLogs)),
            .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
            .init(customView: lineCountToolbarLabel),
            .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
            UIBarButtonItem(barButtonSystemItem: .trash, target: self, action: #selector(clearLogs(_:))).then {
                $0.accessibilityLabel = Strings.RewardsInternals.clearLogsTitle
            }
        ]
        refreshLogs(animated: false)
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        navigationController?.setToolbarHidden(false, animated: animated)
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        navigationController?.setToolbarHidden(true, animated: animated)
    }
    
    @objc private func clearLogs(_ sender: UIBarButtonItem) {
        let alert = UIAlertController(title: Strings.RewardsInternals.clearLogsTitle, message: Strings.RewardsInternals.clearLogsConfirmation, preferredStyle: .actionSheet)
        alert.addAction(UIAlertAction(title: Strings.yes, style: .destructive, handler: { _ in
            rewardsLogger.deleteAllLogs()
            rewardsLogger.newLogWithDate(Date(), configureDestination: { destination in
                // Same as debug log, Rewards framework handles function names in message
                destination.showFunctionName = false
                destination.showThreadName = false
            })
            self.lines = []
            self.tableView.reloadData()
        }))
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        alert.popoverPresentationController?.barButtonItem = sender
        alert.popoverPresentationController?.permittedArrowDirections = .any
        present(alert, animated: true)
    }
    
    @objc private func tappedShare() {
        let controller = RewardsInternalsShareController(rewards: self.rewards, initiallySelectedSharables: [.logs])
        let container = UINavigationController(rootViewController: controller)
        present(container, animated: true)
    }
    
    @objc private func tappedRefreshLogs() {
        refreshLogs(animated: true)
    }
    
    private static let logDisplayLimit = 5000
    
    private func refreshLogs(animated: Bool) {
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            do {
                var lines: [LogLine] = []
                let urls = try rewardsLogger.logFilenamesAndURLs().map(\.1).reversed()
                for url in urls {
                    // If user leaves controller and its dealloc'd no reason to continue reading files
                    if self == nil { return }
                    let fileURL = URL(fileURLWithPath: url.path)
                    do {
                        // Load file
                        let contents = try String(contentsOf: fileURL)
                        // Grab all lines from it, since we can't start from the _end_ and enumerate upwards
                        // we have to grab the whole thing first
                        var fileLines: [String] = []
                        contents.enumerateLines { line, finish in
                            fileLines.append(line)
                        }
                        // Parse based on available space.
                        let availableSpace = Self.logDisplayLimit - lines.count
                        // We want the most recent logs, so we have to reverse the enumerated lines, grab
                        // at most N lines based on available space, then parse them into `LogLine`, then
                        // finally re-reverse them back into the order we want them displayed
                        let parsedLines = fileLines.suffix(availableSpace).compactMap(LogLine.init(from:))
                        lines.insert(contentsOf: parsedLines, at: 0)
                        if lines.count >= Self.logDisplayLimit {
                            // If we're now over the limit we can stop parsing files
                            break
                        }
                    } catch {
                        browserLogger.error("Failed to load log file: \(fileURL.absoluteString) with error: \(String(describing: error))")
                    }
                }
                DispatchQueue.main.async {
                    self?.lines = lines
                    self?.tableView.reloadData()
                    self?.tableView.scrollToRow(at: IndexPath(row: lines.count - 1, section: 0), at: .bottom, animated: animated)
                }
            } catch {
                browserLogger.error("Failed to fetch logs for Rewards: \(String(describing: error))")
            }
        }
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(for: indexPath) as LogLineCell
        cell.textLabel?.text = lines[safe: indexPath.row]?.metadata
        cell.detailTextLabel?.text = lines[safe: indexPath.row]?.message
        return cell
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return lines.count
    }
}

/// A file generator that copies all Rewards related log files into the sharable directory
struct RewardsInternalsLogsGenerator: RewardsInternalsFileGenerator {
    func generateFiles(at path: String, using builder: RewardsInternalsSharableBuilder, completion: @escaping (Error?) -> Void) {
        do {
            let fileURLs = try rewardsLogger.logFilenamesAndURLs().map { URL(fileURLWithPath: $0.1.path) }
            for url in fileURLs {
                let logPath = URL(fileURLWithPath: path).appendingPathComponent(url.lastPathComponent)
                try FileManager.default.copyItem(atPath: url.path, toPath: logPath.path)
            }
            completion(nil)
        } catch {
            completion(error)
        }
    }
}
