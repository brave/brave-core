// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards

class AutoContributeSupportedListController: UIViewController {
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
  
  private var contentView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = View()
  }
  
  private lazy var editButton = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(tappedEdit))
  private lazy var doneButton = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    contentView.tableView.register(AutoContributeCell.self)
    contentView.tableView.register(EmptyTableCell.self)
    
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    title = Strings.autoContributeSupportedSites.capitalized
    
    navigationItem.rightBarButtonItem = editButton
    reloadData()
  }
  
  @objc private func tappedEdit() {
    contentView.tableView.setEditing(true, animated: true)
    navigationItem.setRightBarButton(doneButton, animated: true)
  }
  
  @objc private func tappedDone() {
    contentView.tableView.setEditing(false, animated: true)
    navigationItem.setRightBarButton(editButton, animated: true)
  }
  
  // MARK: - Data
  
  private var publishers: [PublisherInfo] = []
  
  func reloadData() {
    let filter = state.ledger.supportedPublishersFilter
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
extension AutoContributeSupportedListController: UITableViewDelegate {
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

extension AutoContributeSupportedListController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return publishers.isEmpty ? 1 : publishers.count
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if publishers.isEmpty {
      let cell = tableView.dequeueReusableCell(for: indexPath) as EmptyTableCell
      cell.label.text = Strings.emptyAutoContribution
      return cell
    }
    guard let publisher = publishers[safe: indexPath.row] else {
      assertionFailure("No Publisher found at index: \(indexPath.row)")
      return UITableViewCell()
    }
    let cell = tableView.dequeueReusableCell(for: indexPath) as AutoContributeCell
    if let url = URL(string: publisher.url) {
      state.dataSource?.retrieveFavicon(for: url, on: cell.siteImageView.imageView)
    }
    
    cell.verifiedStatusImageView.isHidden = publisher.status == .notVerified
    let provider = " \(publisher.provider.isEmpty ? "" : String(format: Strings.onProviderText, publisher.providerDisplayString))"
    let attrName = NSMutableAttributedString(string: publisher.name).then {
      $0.append(NSMutableAttributedString(string: provider, attributes: [.font: UIFont.boldSystemFont(ofSize: 14.0),
                                                                         .foregroundColor: UIColor.gray]))
    }
    cell.siteNameLabel.attributedText = attrName
    cell.attentionAmount = CGFloat(publisher.percent)
    return cell
  }
  
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    return !publishers.isEmpty
  }
  
  func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
    if publishers.isEmpty { return .none }
    return .delete
  }
  
  func tableView(_ tableView: UITableView, titleForDeleteConfirmationButtonForRowAt indexPath: IndexPath) -> String? {
    return Strings.exclude
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
          state: .excluded
        )
      })
    }
  }
  
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    return contentView.headerView
  }
  
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return contentView.headerView.systemLayoutSizeFitting(
      CGSize(width: tableView.bounds.width, height: tableView.bounds.height),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).height
  }
}

extension AutoContributeSupportedListController {
  private class View: SettingsTableView {
    fileprivate let headerView = TableHeaderRowView(
      columns: [
        TableHeaderRowView.Column(
          title: Strings.site.uppercased(),
          width: .percentage(0.7)
        ),
        TableHeaderRowView.Column(
          title: Strings.attention.uppercased(),
          width: .percentage(0.3),
          align: .right
        ),
      ],
      tintColor: BraveUX.autoContributeTintColor
    )
  }
}
