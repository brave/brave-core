// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveUI
import Fuzi
import FeedKit

class BraveNewsAddSourceViewController: UITableViewController {
    
    private let feedDataSource: FeedDataSource
    var sourcesAdded: ((Set<RSSFeedLocation>) -> Void)?
    
    private var isLoading: Bool = false {
        didSet {
            if isLoading {
                self.activityIndicator.startAnimating()
            } else {
                self.activityIndicator.stopAnimating()
            }
        }
    }
    private let activityIndicator = UIActivityIndicatorView(style: .gray).then {
        $0.hidesWhenStopped = true
    }
    
    init(dataSource: FeedDataSource) {
        self.feedDataSource = dataSource
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
    
    deinit {
        pageTask?.cancel()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.BraveNews.addSource
        
        navigationController?.navigationBar.prefersLargeTitles = true
        navigationItem.largeTitleDisplayMode = .always
        navigationItem.backButtonTitle = ""
        navigationItem.leftBarButtonItem = .init(barButtonSystemItem: .cancel, target: self, action: #selector(tappedCancel))
        navigationItem.rightBarButtonItem = .init(customView: activityIndicator)
        
        textField.addTarget(self, action: #selector(textFieldTextChanged), for: .editingChanged)
        textField.delegate = self
        
        tableView.register(FeedSearchCellClass.self)
        tableView.register(CenteredButtonCell.self)
        tableView.tableHeaderView = UIView(frame: .init(x: 0, y: 0, width: 0, height: 10))
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        textField.becomeFirstResponder()
    }
    
    @objc private func tappedCancel() {
        dismiss(animated: true, completion: nil)
    }
    
    @objc private func textFieldTextChanged() {
        if let cell = tableView.cellForRow(at: IndexPath(row: 1, section: 0)) as? CenteredButtonCell {
            // Update the color of the search row when text field is non empty
            cell.tintColor = isSearchEnabled && !isLoading ? .braveOrange : .braveDisabled
        }
    }
    
    private func tappedImportOPML() {
        let picker = UIDocumentPickerViewController(documentTypes: ["public.opml"], in: .import)
        picker.delegate = self
        picker.allowsMultipleSelection = false
        if #available(iOS 13.0, *) {
            picker.shouldShowFileExtensions = true
        }
        present(picker, animated: true)
    }
    
    private func rssLocationFromOPMLOutline(_ outline: OPML.Outline) -> RSSFeedLocation? {
        guard let url = outline.xmlUrl?.asURL else { return nil }
        return .init(title: outline.text, url: url)
    }
    
    private let session: URLSession = {
        let configuration = URLSessionConfiguration.ephemeral
        configuration.timeoutIntervalForRequest = 5
        return URLSession(configuration: configuration, delegate: nil, delegateQueue: .main)
    }()
    
    private func displayError(_ error: FindFeedsError) {
        let alert = UIAlertController(title: Strings.BraveNews.addSourceFailureTitle, message: error.localizedDescription, preferredStyle: .alert)
        alert.addAction(.init(title: Strings.OKString, style: .default, handler: nil))
        present(alert, animated: true)
    }
    
    private func searchPageForFeeds() {
        guard var text = textField.text else { return }
        if text.hasPrefix("feed:"), let range = text.range(of: "feed:") {
            text.replaceSubrange(range, with: [])
        }
        guard let url = URIFixup.getURL(text) else { return }
        isLoading = true
        downloadPageData(for: url) { [weak self] result in
            guard let self = self else { return }
            self.isLoading = false
            switch result {
            case .success(let data):
                let resultsController = BraveNewsAddSourceResultsViewController(
                    dataSource: self.feedDataSource,
                    searchedURL: url,
                    rssFeedLocations: data,
                    sourcesAdded: self.sourcesAdded
                )
                self.navigationController?.pushViewController(resultsController, animated: true)
            case .failure(let error):
                self.displayError(error)
            }
        }
    }
    
    private enum FindFeedsError: Error {
        /// An error occured while attempting to download the page
        case dataTaskError(Error)
        /// The data was either not received or is in the incorrect format
        case invalidData
        /// The data downloaded did not match a
        case parserError(ParserError)
        /// No feeds were found at the given URL
        case noFeedsFound
        
        var localizedDescription: String {
            switch self {
            case .dataTaskError(let error as URLError) where error.code == .notConnectedToInternet:
                return error.localizedDescription
            case .dataTaskError:
                return Strings.BraveNews.addSourceNetworkFailureMessage
            case .invalidData, .parserError:
                return Strings.BraveNews.addSourceInvalidDataMessage
            case .noFeedsFound:
                return Strings.BraveNews.addSourceNoFeedsFoundMessage
            }
        }
    }
    
    private var pageTask: URLSessionDataTask?
    private func downloadPageData(for url: URL, _ completion: @escaping (Result<[RSSFeedLocation], FindFeedsError>) -> Void) {
        pageTask = session.dataTask(with: url) { [weak self] (data, response, error) in
            guard let self = self else { return }
            if let error = error {
                completion(.failure(.dataTaskError(error)))
                return
            }
            guard let data = data,
                  let root = try? HTMLDocument(data: data),
                  let url = response?.url else {
                completion(.failure(.invalidData))
                return
            }
            
            // Check if `data` is actually an RSS feed
            if case .success(let feed) = FeedParser(data: data).parse() {
                // User provided a direct feed
                var title: String?
                switch feed {
                case .atom(let atom):
                    title = atom.title
                case .json(let json):
                    title = json.title
                case .rss(let rss):
                    title = rss.title
                }
                completion(.success([.init(title: title, url: url)]))
                return
            }
            
            // Check if `data` is actually an OPML list
            if let opml = OPMLParser.parse(data: data), !opml.outlines.isEmpty {
                let locations = opml.outlines.compactMap(self.rssLocationFromOPMLOutline)
                completion(locations.isEmpty ? .failure(.noFeedsFound) : .success(locations))
                return
            }
            
            // Ensure page is reloaded to final landing page before looking for
            // favicons
            var reloadUrl: URL?
            for meta in root.xpath("//head/meta") {
                if let refresh = meta["http-equiv"]?.lowercased(), refresh == "refresh",
                   let content = meta["content"],
                   let index = content.range(of: "URL="),
                   let url = NSURL(string: String(content.suffix(from: index.upperBound))) {
                    reloadUrl = url as URL
                }
            }
            
            if let url = reloadUrl {
                self.downloadPageData(for: url, completion)
                return
            }
            
            var feeds: [RSSFeedLocation] = []
            let xpath = "//head//link[contains(@type, 'application/rss+xml') or contains(@type, 'application/atom+xml') or contains(@type, 'application/json')]"
            for link in root.xpath(xpath) {
                guard let href = link["href"], let url = URL(string: href, relativeTo: url),
                      url.isWebPage(includeDataURIs: false) else {
                    continue
                }
                feeds.append(.init(title: link["title"], url: url))
            }
            if feeds.isEmpty {
                completion(.failure(.noFeedsFound))
            } else {
                completion(.success(feeds))
            }
        }
        pageTask?.resume()
    }
    
    private let textField = UITextField().then {
        $0.attributedPlaceholder = NSAttributedString(
            string: Strings.BraveNews.searchTextFieldPlaceholder,
            attributes: [.foregroundColor: UIColor.lightGray]
        )
        $0.font = .preferredFont(forTextStyle: .body)
        $0.keyboardType = .URL
        $0.autocorrectionType = .no
        $0.autocapitalizationType = .none
        $0.returnKeyType = .search
    }
    
    private var isSearchEnabled: Bool {
        if let text = textField.text {
            return URIFixup.getURL(text) != nil
        }
        return false
    }
    
    // MARK: - UITableViewDelegate
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if indexPath.section == 0 && indexPath.row == 1, isSearchEnabled, !isLoading {
            searchPageForFeeds()
        }
        if indexPath.section == 1 && indexPath.row == 0 {
            tappedImportOPML()
        }
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    override func tableView(_ tableView: UITableView, shouldHighlightRowAt indexPath: IndexPath) -> Bool {
        if indexPath.section == 0 {
            if indexPath.row == 1 {
                return isSearchEnabled && !isLoading
            }
            return false
        }
        return true
    }
    
    // MARK: - UITableViewDataSource
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        switch indexPath.section {
        case 0:
            switch indexPath.row {
            case 0:
                let cell = tableView.dequeueReusableCell(for: indexPath) as FeedSearchCellClass
                cell.textField = textField
                return cell
            case 1:
                let cell = tableView.dequeueReusableCell(for: indexPath) as CenteredButtonCell
                cell.textLabel?.text = Strings.BraveNews.searchButtonTitle
                cell.tintColor = isSearchEnabled && !isLoading ? .braveOrange : .braveDisabled
                return cell
            default:
                fatalError("No cell available for index path: \(indexPath)")
            }
        case 1:
            let cell = tableView.dequeueReusableCell(for: indexPath) as CenteredButtonCell
            cell.textLabel?.text = Strings.BraveNews.importOPML
            cell.tintColor = .braveOrange
            return cell
        default:
            fatalError("No cell available for index path: \(indexPath)")
        }
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        2
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch section {
        case 0: return 2
        case 1: return 1
        default: return 0
        }
    }
}

extension BraveNewsAddSourceViewController: UIDocumentPickerDelegate {
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
        guard let url = urls.first, url.isFileURL, let data = try? Data(contentsOf: url) else {
            controller.dismiss(animated: true) {
                self.displayError(.noFeedsFound)
            }
            return
        }
        DispatchQueue.global(qos: .userInitiated).async {
            let opml = OPMLParser.parse(data: data)
            DispatchQueue.main.async {
                guard let opml = opml else {
                    controller.dismiss(animated: true) {
                        self.displayError(.invalidData)
                    }
                    return
                }
                let locations = opml.outlines.compactMap(self.rssLocationFromOPMLOutline)
                if locations.isEmpty {
                    controller.dismiss(animated: true) {
                        self.displayError(.noFeedsFound)
                    }
                    return
                }
                let resultsController = BraveNewsAddSourceResultsViewController(
                    dataSource: self.feedDataSource,
                    searchedURL: url,
                    rssFeedLocations: locations,
                    sourcesAdded: self.sourcesAdded
                )
                self.navigationController?.pushViewController(resultsController, animated: true)
            }
        }
    }
    func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
        controller.dismiss(animated: true)
    }
}

extension BraveNewsAddSourceViewController: UITextFieldDelegate {
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        if isSearchEnabled {
            textField.resignFirstResponder()
            searchPageForFeeds()
            return true
        }
        return false
    }
}

private class FeedSearchCellClass: UITableViewCell, TableViewReusable {
    var textField: UITextField? {
        willSet {
            textField?.removeFromSuperview()
        }
        didSet {
            if let textField = textField {
                contentView.addSubview(textField)
                textField.snp.makeConstraints {
                    $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 0, left: 12, bottom: 0, right: 12))
                    $0.height.greaterThanOrEqualTo(44)
                }
            }
        }
    }
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
