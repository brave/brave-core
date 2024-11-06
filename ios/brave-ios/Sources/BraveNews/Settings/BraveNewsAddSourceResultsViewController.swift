// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import Foundation
import Preferences
import Shared
import UIKit

public class BraveNewsAddSourceResultsViewController: UITableViewController {

  let feedDataSource: FeedDataSource
  let searchedURL: URL
  private let secureLocations: [RSSFeedLocation]
  private let insecureLocations: [RSSFeedLocation]
  var sourcesAdded: ((Set<RSSFeedLocation>) -> Void)?

  private var selectedLocations: Set<RSSFeedLocation>

  public init(
    dataSource: FeedDataSource,
    searchedURL: URL,
    rssFeedLocations: [RSSFeedLocation],
    sourcesAdded: ((Set<RSSFeedLocation>) -> Void)?
  ) {
    self.feedDataSource = dataSource
    self.searchedURL = searchedURL
    let locations = Set(rssFeedLocations)
    self.secureLocations = locations.filter { $0.url.scheme == "https" }
    self.insecureLocations = Array(locations.subtracting(self.secureLocations))
    self.selectedLocations = locations
    self.sourcesAdded = sourcesAdded

    super.init(style: .insetGrouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private lazy var doneButton = UIBarButtonItem(
    title: Strings.BraveNews.addSourceAddButtonTitle,
    style: .done,
    target: self,
    action: #selector(tappedAdd)
  )

  public override func viewDidLoad() {
    super.viewDidLoad()

    title = searchedURL.baseDomain

    navigationItem.largeTitleDisplayMode = .never
    navigationItem.rightBarButtonItem = doneButton

    tableView.register(FeedLocationCell.self)

    if navigationController?.viewControllers.first === self {
      // Presented via share screen or isolated
      navigationItem.leftBarButtonItem = .init(
        barButtonSystemItem: .cancel,
        target: self,
        action: #selector(tappedCancel)
      )
    }
  }

  @objc private func tappedAdd() {
    // Add selected sources to feed
    for location in selectedLocations {
      feedDataSource.addRSSFeedLocation(location)
    }
    sourcesAdded?(selectedLocations)
    dismiss(animated: true)
    Preferences.Review.braveNewsCriteriaPassed.value = true
  }

  @objc private func tappedCancel() {
    dismiss(animated: true)
  }

  // MARK: - UITableViewDelegate

  public override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    let locations = indexPath.section == 0 ? secureLocations : insecureLocations
    if let location = locations[safe: indexPath.row],
      let cell = tableView.cellForRow(at: indexPath) as? FeedLocationCell
    {
      if selectedLocations.remove(location) == nil {
        selectedLocations.insert(location)
      }
      cell.accessoryType = selectedLocations.contains(location) ? .checkmark : .none
      doneButton.isEnabled = !selectedLocations.isEmpty
    }
    tableView.deselectRow(at: indexPath, animated: true)
  }

  // MARK: - UITableViewDataSource

  public override func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    let locations = indexPath.section == 0 ? secureLocations : insecureLocations
    guard let location = locations[safe: indexPath.row] else {
      assertionFailure()
      return UITableViewCell()
    }
    let cell = tableView.dequeueReusableCell(for: indexPath) as FeedLocationCell
    cell.imageView?.image =
      indexPath.section == 0
      ? UIImage(braveSystemNamed: "leo.lock.plain", compatibleWith: nil)?
        .applyingSymbolConfiguration(
          .init(font: .preferredFont(for: .body, weight: .semibold), scale: .small)
        ) : UIImage(named: "insecure-site-icon", in: .module, compatibleWith: nil)!
    cell.imageView?.tintColor = .braveLabel
    cell.textLabel?.text = location.title
    cell.detailTextLabel?.text = location.url.absoluteString
    cell.accessoryType = selectedLocations.contains(location) ? .checkmark : .none
    return cell
  }

  public override func tableView(
    _ tableView: UITableView,
    numberOfRowsInSection section: Int
  ) -> Int {
    section == 0 ? secureLocations.count : insecureLocations.count
  }

  public override func numberOfSections(in tableView: UITableView) -> Int {
    insecureLocations.isEmpty ? 1 : 2
  }

  public override func tableView(
    _ tableView: UITableView,
    titleForHeaderInSection section: Int
  ) -> String? {
    if section == 1 {
      return Strings.BraveNews.insecureSourcesHeader
    }
    return nil
  }
}

private class FeedLocationCell: UITableViewCell, TableViewReusable {
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
