/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Storage
import QuickLook

private struct DownloadsPanelUX {
    static let welcomeScreenPadding: CGFloat = 15
    static let welcomeScreenItemWidth = 170
}

private let log = Logger.browserLogger

struct DownloadedFile: Equatable {
    let path: URL
    let size: UInt64
    let lastModified: Date
    
    var canShowInWebView: Bool {
        return MIMEType.canShowInWebView(mimeType)
    }
    
    var filename: String {
        return path.lastPathComponent
    }
    
    var fileExtension: String {
        return path.pathExtension
    }
    
    var formattedSize: String {
        return ByteCountFormatter.string(fromByteCount: Int64(size), countStyle: .file)
    }
    
    var mimeType: String {
        return MIMEType.mimeTypeFromFileExtension(fileExtension)
    }
    
    static public func ==(lhs: DownloadedFile, rhs: DownloadedFile) -> Bool {
        return lhs.path == rhs.path
    }
}

class DownloadsPanel: UIViewController, UITableViewDelegate, UITableViewDataSource {
    let profile: Profile
    var tableView = UITableView()
    
    private let events: [Notification.Name] =
        [.fileDidDownload, .privateDataClearedDownloadedFiles, .dynamicFontChanged]
    
    private lazy var emptyStateOverlayView: UIView = self.createEmptyStateOverlayView()
    
    private var groupedDownloadedFiles = DateGroupedTableData<DownloadedFile>()
    private var fileExtensionIcons: [String: UIImage] = [:]
    
    let welcomeLabel = UILabel()
    let overlayView = UIView()
    let logoImageView = UIImageView(image: #imageLiteral(resourceName: "emptyDownloads").template)
    
    // MARK: - Lifecycle
    init(profile: Profile) {
        self.profile = profile
        super.init(nibName: nil, bundle: nil)
        events.forEach { NotificationCenter.default.addObserver(self, selector: #selector(notificationReceived), name: $0, object: nil) }
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        reloadData()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        view.addSubview(tableView)
        tableView.snp.makeConstraints { make in
            make.edges.equalTo(self.view)
            return
        }
        
        tableView.delegate = self
        tableView.dataSource = self
        tableView.register(TwoLineTableViewCell.self, forCellReuseIdentifier: "TwoLineTableViewCell")
        tableView.layoutMargins = .zero
        tableView.keyboardDismissMode = .onDrag
        tableView.backgroundColor = UIColor.Photon.white100
        tableView.separatorColor = UIColor.Photon.grey30
        tableView.accessibilityIdentifier = "DownloadsTable"
        tableView.cellLayoutMarginsFollowReadableWidth = false
        
        // Set an empty footer to prevent empty cells from appearing in the list.
        tableView.tableFooterView = UIView()
        
        title = Strings.downloadsMenuItem
    }
    
    deinit {
        // The view might outlive this view controller thanks to animations;
        // explicitly nil out its references to us to avoid crashes. Bug 1218826.
        tableView.dataSource = nil
        tableView.delegate = nil
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        if traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
            updateThemeForUserInterfaceStyleChange()
        }
    }
    
    @objc func notificationReceived(_ notification: Notification) {
        DispatchQueue.main.async {
            self.reloadData()
            
            switch notification.name {
            case .fileDidDownload, .privateDataClearedDownloadedFiles:
                break
            case .dynamicFontChanged:
                if self.emptyStateOverlayView.superview != nil {
                    self.emptyStateOverlayView.removeFromSuperview()
                }
                self.emptyStateOverlayView = self.createEmptyStateOverlayView()
                break
            default:
                // no need to do anything at all
                log.warning("Error: Received unexpected notification \(notification.name)")
                break
            }
        }
    }
    
    func reloadData() {
        groupedDownloadedFiles = DateGroupedTableData<DownloadedFile>()
        
        let downloadedFiles = fetchData()
        for downloadedFile in downloadedFiles {
            groupedDownloadedFiles.add(downloadedFile, timestamp: downloadedFile.lastModified.timeIntervalSince1970)
        }
        
        fileExtensionIcons = [:]
        
        tableView.reloadData()
        updateEmptyPanelState()
    }
    
    private func fetchData() -> [DownloadedFile] {
        var downloadedFiles: [DownloadedFile] = []
        do {
            let downloadsPath = try FileManager.default.downloadsPath()
            let files = try FileManager.default.contentsOfDirectory(at: downloadsPath, includingPropertiesForKeys: nil, options: [.skipsHiddenFiles, .skipsPackageDescendants, .skipsSubdirectoryDescendants])
            
            for file in files {
                let attributes = try FileManager.default.attributesOfItem(atPath: file.path) as NSDictionary
                let downloadedFile = DownloadedFile(path: file, size: attributes.fileSize(), lastModified: attributes.fileModificationDate() ?? Date())
                downloadedFiles.append(downloadedFile)
            }
        } catch {
            log.error("Unable to get files: \(error.localizedDescription)")
            return []
        }
        
        return downloadedFiles.sorted(by: { a, b -> Bool in
            return a.lastModified > b.lastModified
        })
    }
    
    private func deleteDownloadedFile(_ downloadedFile: DownloadedFile) -> Bool {
        do {
            try FileManager.default.removeItem(at: downloadedFile.path)
            return true
        } catch {
            log.error("Unable to delete downloaded file: \(error)")
        }
        
        return false
    }
    
    private func shareDownloadedFile(_ downloadedFile: DownloadedFile, indexPath: IndexPath) {
        let helper = ShareExtensionHelper(url: downloadedFile.path, tab: nil)
        let controller = helper.createActivityViewController { completed, activityType, _ in
            log.debug("Shared downloaded file: \(completed)")
        }

        if let popoverPresentationController = controller.popoverPresentationController {
            guard let tableViewCell = tableView.cellForRow(at: indexPath) else {
                log.warning("Unable to get table view cell at index path: \(indexPath)")
                return
            }

            popoverPresentationController.sourceView = tableViewCell
            popoverPresentationController.sourceRect = tableViewCell.bounds
            popoverPresentationController.permittedArrowDirections = .any
        }
        
        present(controller, animated: true, completion: nil)
    }
    
    private func iconForFileExtension(_ fileExtension: String) -> UIImage? {
        if let icon = fileExtensionIcons[fileExtension] {
            return icon
        }
        
        guard let icon = roundRectImageWithLabel(fileExtension, width: 29, height: 29) else {
            return nil
        }
        
        fileExtensionIcons[fileExtension] = icon
        return icon
    }
    
    private func roundRectImageWithLabel(_ label: String, width: CGFloat, height: CGFloat,
                                         radius: CGFloat = 5.0, strokeWidth: CGFloat = 1.0,
                                         strokeColor: UIColor = UIColor.Photon.grey60,
                                         fontSize: CGFloat = 9.0) -> UIImage? {
        UIGraphicsBeginImageContextWithOptions(CGSize(width: width, height: height), false, 0.0)
        let context = UIGraphicsGetCurrentContext()
        context?.setStrokeColor(strokeColor.cgColor)
        
        let rect = CGRect(x: strokeWidth / 2, y: strokeWidth / 2, width: width - strokeWidth, height: height - strokeWidth)
        let bezierPath = UIBezierPath(roundedRect: rect, cornerRadius: radius)
        bezierPath.lineWidth = strokeWidth
        bezierPath.stroke()
        
        let attributedString = NSAttributedString(string: label, attributes: [
            .baselineOffset: -(strokeWidth * 2),
            .font: UIFont.systemFont(ofSize: fontSize),
            .foregroundColor: strokeColor
            ])
        let stringHeight: CGFloat = fontSize * 2
        let stringWidth = attributedString.boundingRect(with: CGSize(width: width, height: stringHeight), options: .usesLineFragmentOrigin, context: nil).size.width
        attributedString.draw(at: CGPoint(x: (width - stringWidth) / 2 + strokeWidth, y: (height - stringHeight) / 2 + strokeWidth))
        
        let image = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()
        
        return image
    }
    
    // MARK: - Empty State
    private func updateEmptyPanelState() {
        if groupedDownloadedFiles.isEmpty {
            if emptyStateOverlayView.superview == nil {
                view.addSubview(emptyStateOverlayView)
                view.bringSubviewToFront(emptyStateOverlayView)
                emptyStateOverlayView.snp.makeConstraints { make in
                    make.edges.equalTo(self.tableView)
                }
            }
        } else {
            emptyStateOverlayView.removeFromSuperview()
        }
    }
    
    fileprivate func createEmptyStateOverlayView() -> UIView {
        overlayView.backgroundColor = UIColor.Photon.white100
        
        overlayView.addSubview(logoImageView)
        logoImageView.snp.makeConstraints { make in
            make.centerX.equalTo(overlayView)
            make.size.equalTo(60)
            // Sets proper top constraint for iPhone 6 in portait and for iPad.
            make.centerY.equalTo(overlayView).offset(-180).priority(100)
            
            // Sets proper top constraint for iPhone 4, 5 in portrait.
            make.top.greaterThanOrEqualTo(overlayView).offset(50)
        }
        
        overlayView.addSubview(welcomeLabel)
        welcomeLabel.text = Strings.downloadsPanelEmptyStateTitle
        welcomeLabel.textAlignment = .center
        welcomeLabel.font = DynamicFontHelper.defaultHelper.DeviceFontLight
        welcomeLabel.textColor = UIColor.Photon.grey50
        welcomeLabel.numberOfLines = 0
        welcomeLabel.adjustsFontSizeToFitWidth = true
        
        welcomeLabel.snp.makeConstraints { make in
            make.centerX.equalTo(overlayView)
            make.top.equalTo(logoImageView.snp.bottom).offset(DownloadsPanelUX.welcomeScreenPadding)
            make.width.equalTo(DownloadsPanelUX.welcomeScreenItemWidth)
        }
        
        return overlayView
    }
    
    fileprivate func downloadedFileForIndexPath(_ indexPath: IndexPath) -> DownloadedFile? {
        let downloadedFilesInSection = groupedDownloadedFiles.itemsForSection(indexPath.section)
        return downloadedFilesInSection[safe: indexPath.row]
    }
    // MARK: - TableView Delegate / DataSource
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let cell = tableView.dequeueReusableCell(withIdentifier: "TwoLineTableViewCell", for: indexPath) as? TwoLineTableViewCell else {
            log.error("Could not get table view cell as TwoLineTableViewCell")
            return UITableViewCell()
        }
        
        return configureDownloadedFile(cell, for: indexPath)
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        guard groupedDownloadedFiles.numberOfItemsForSection(section) > 0 else { return nil }
        
        switch section {
        case 0: return Strings.today
        case 1: return Strings.yesterday
        case 2: return Strings.lastWeek
        case 3: return Strings.lastMonth
        default: log.error("Invalid Downloads section \(section)")
        }
        
        return nil
    }
    
    func configureDownloadedFile(_ cell: UITableViewCell, for indexPath: IndexPath) -> UITableViewCell {
        if let downloadedFile = downloadedFileForIndexPath(indexPath), let cell = cell as? TwoLineTableViewCell {
            cell.setLines(downloadedFile.filename, detailText: downloadedFile.formattedSize)
            cell.imageView?.image = iconForFileExtension(downloadedFile.fileExtension)
        }
        return cell
    }
    
    private var previewingFileDataSource: PreviewingDataSource?
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        
        if let downloadedFile = downloadedFileForIndexPath(indexPath) {
            let previewingItem = PreviewingDataSource(file: downloadedFile)
            if QLPreviewController.canPreview(previewingItem.file.path as QLPreviewItem) {
                previewingFileDataSource = previewingItem
                let quickLook = QLPreviewController()
                quickLook.dataSource = previewingFileDataSource
                quickLook.delegate = self
                present(quickLook, animated: true)
                return
            }
            
            guard downloadedFile.canShowInWebView else {
                shareDownloadedFile(downloadedFile, indexPath: indexPath)
                return
            }
        }
    }
    
    func numberOfSections(in tableView: UITableView) -> Int {
        return 4
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return groupedDownloadedFiles.numberOfItemsForSection(section)
    }
    
    func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        // Intentionally blank. Required to use UITableViewRowActions
    }
    
    func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]? {
        let delete = UITableViewRowAction(style: .destructive, title: Strings.delete,
                                          handler: { action, indexPath in
            if let downloadedFile = self.downloadedFileForIndexPath(indexPath) {
                if self.deleteDownloadedFile(downloadedFile) {
                    self.tableView.beginUpdates()
                    self.groupedDownloadedFiles.remove(downloadedFile)
                    self.tableView.deleteRows(at: [indexPath], with: .right)
                    self.tableView.endUpdates()
                    self.updateEmptyPanelState()
                }
            }
        })
        let share = UITableViewRowAction(style: .normal, title: Strings.share,
                                         handler: { action, indexPath in
            if let downloadedFile = self.downloadedFileForIndexPath(indexPath) {
                self.shareDownloadedFile(downloadedFile, indexPath: indexPath)
            }
        })
        share.backgroundColor = view.tintColor
        return [delete, share]
    }
}

// MARK: - UIDocumentInteractionControllerDelegate

extension DownloadsPanel: UIDocumentInteractionControllerDelegate {
    func documentInteractionControllerViewControllerForPreview(
        _ controller: UIDocumentInteractionController) -> UIViewController {
        return self
    }
}

extension DownloadsPanel: Themeable {
    
    func applyTheme(_ theme: Theme) {
        emptyStateOverlayView.removeFromSuperview()
        emptyStateOverlayView = createEmptyStateOverlayView()
        updateEmptyPanelState()
        
        welcomeLabel.appearanceTextColor = theme.colors.tints.home
        overlayView.appearanceBackgroundColor = theme.colors.home
        logoImageView.tintColor = theme.colors.tints.home
        
        tableView.reloadData()
    }
}

extension DownloadsPanel: QLPreviewControllerDelegate {
    func previewControllerDidDismiss(_ controller: QLPreviewController) {
        previewingFileDataSource = nil
    }
}

private class PreviewingDataSource: NSObject, QLPreviewControllerDataSource {
    let file: DownloadedFile
    
    init(file: DownloadedFile) {
        self.file = file
    }
    
    func numberOfPreviewItems(in controller: QLPreviewController) -> Int {
        return 1
    }
    
    func previewController(_ controller: QLPreviewController, previewItemAt index: Int) -> QLPreviewItem {
        return file.path as QLPreviewItem
    }
}
