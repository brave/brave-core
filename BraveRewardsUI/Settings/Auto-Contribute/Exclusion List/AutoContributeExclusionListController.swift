// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards

final class AutoContributeExclusionListController: UIViewController {
  private let state: RewardsState
  private let ledgerObserver: LedgerObserver
  
  init(state: RewardsState) {
    self.state = state
    ledgerObserver = LedgerObserver(ledger: state.ledger)
    state.ledger.add(ledgerObserver)
    super.init(nibName: nil, bundle: nil)
    setupLedgerObservers()
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  private var contentView: SettingsTableView {
    return view as! SettingsTableView // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = SettingsTableView()
  }
  
  private lazy var editButton = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(tappedEdit))
  private lazy var doneButton = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
  private lazy var restoreAllButton = UIBarButtonItem(title: Strings.restoreAllSitesToolbarButtonTitle, style: .plain, target: self, action: #selector(tappedRestoreAll(_:)))
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    contentView.tableView.register(ExclusionListCell.self)
    contentView.tableView.register(EmptyTableCell.self)

    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    title = Strings.exclusionListTitle.capitalized
    
    navigationItem.rightBarButtonItem = editButton
    toolbarItems = [
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
      restoreAllButton,
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil)
    ]
    
    // Not enabled unless we have more than 1 publisher
    restoreAllButton.isEnabled = false
    reloadData()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    navigationController?.setToolbarHidden(false, animated: animated)
  }
  
  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    navigationController?.setToolbarHidden(true, animated: animated)
  }
  
  @objc private func tappedEdit() {
    contentView.tableView.setEditing(true, animated: true)
    navigationItem.setRightBarButton(doneButton, animated: true)
  }
  
  @objc private func tappedDone() {
    contentView.tableView.setEditing(false, animated: true)
    navigationItem.setRightBarButton(editButton, animated: true)
  }
  
  @objc private func tappedRestoreAll(_ sender: UIBarButtonItem) {
    let numberOfExcludedSites = publishers.count
    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    if let presenter = alert.popoverPresentationController {
      presenter.barButtonItem = sender
      presenter.permittedArrowDirections = [.up, .down]
    }
    alert.addAction(UIAlertAction(title: String(format: Strings.autoContributeRestoreExcludedSites, numberOfExcludedSites), style: .default, handler: { _ in
      self.state.ledger.restoreAllExcludedPublishers()
      self.navigationController?.popViewController(animated: true)
    }))
    alert.addAction(UIAlertAction(title: Strings.cancel, style: .cancel, handler: nil))
    present(alert, animated: true)
  }
  
  // MARK: - Data
  
  private var publishers: [PublisherInfo] = [] {
    didSet {
      restoreAllButton.isEnabled = !publishers.isEmpty
    }
  }
  
  func reloadData() {
    let filter = state.ledger.excludedPublishersFilter
    state.ledger.listActivityInfo(fromStart: 0, limit: 0, filter: filter) { [weak self] list in
      guard let self = self else { return }
      self.publishers = list
      self.editButton.isEnabled = !list.isEmpty
      self.contentView.tableView.reloadData()
    }
  }
  
  func setupLedgerObservers() {
    ledgerObserver.excludedSitesChanged = { [weak self] _, _ in
      guard let self = self, self.isViewLoaded else {
        return
      }
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.3, execute: {
        self.reloadData()
      })
    }
  }
}

// MARK: - UITableViewDelegate
extension AutoContributeExclusionListController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, willBeginEditingRowAt indexPath: IndexPath) {
    navigationItem.setRightBarButton(doneButton, animated: true)
  }
  
  func tableView(_ tableView: UITableView, didEndEditingRowAt indexPath: IndexPath?) {
    navigationItem.setRightBarButton(editButton, animated: true)
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if !publishers.isEmpty, let url = URL(string: publishers[indexPath.row].url) {
      state.delegate?.loadNewTabWithURL(url)
    }
  }
}

// MARK: - UITableViewDataSource
extension AutoContributeExclusionListController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return publishers.isEmpty ? 1 : publishers.count
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if publishers.isEmpty {
      let cell = tableView.dequeueReusableCell(for: indexPath) as EmptyTableCell
      cell.label.text = Strings.emptyExclusionList
      return cell
    }
    guard let publisher = publishers[safe: indexPath.row] else {
      assertionFailure("No Publisher found at index: \(indexPath.row)")
      return UITableViewCell()
    }
    let cell = tableView.dequeueReusableCell(for: indexPath) as ExclusionListCell
    if let url = URL(string: publisher.url) {
      state.dataSource?.retrieveFavicon(for: url, faviconURL: URL(string: publisher.faviconUrl)) { data in
        cell.siteImageView.image = data?.image ?? UIImage(frameworkResourceNamed: "defaultFavicon")
        cell.siteImageView.backgroundColor = data?.backgroundColor
      }
    }
    
    cell.verifiedStatusImageView.isHidden = publisher.status == .notVerified
    let provider = " \(publisher.provider.isEmpty ? "" : String(format: Strings.onProviderText, publisher.providerDisplayString))"
    let attrName = NSMutableAttributedString(string: publisher.name).then {
      $0.append(NSMutableAttributedString(string: provider, attributes: [.font: UIFont.boldSystemFont(ofSize: 14.0),
                                                                         .foregroundColor: UIColor.gray]))
    }
    cell.siteNameLabel.attributedText = attrName
    return cell
  }
  
  func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
    let action = UIContextualAction(
      style: .normal,
      title: Strings.restore,
      handler: { action, view, handler in
        self.tableView(tableView, commit: .delete, forRowAt: indexPath)
        handler(true)
      }
    )
    action.backgroundColor = Colors.blurple400
    let config = UISwipeActionsConfiguration(
      actions: [
        action
      ]
    )
    return config
  }
  
  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    if let publisher = publishers[safe: indexPath.row] {
      tableView.performBatchUpdates({
        publishers.remove(at: indexPath.row)
        tableView.deleteRows(at: [indexPath], with: .automatic)
        if publishers.isEmpty {
          tableView.insertRows(at: [IndexPath(row: 0, section: 0)], with: .automatic)
        }
      }, completion: { _ in
        self.state.ledger.updatePublisherExclusionState(
          withId: publisher.id,
          state: .default
        )
      })
    }
  }
}
