// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards

extension RewardsType {
  fileprivate var tableHeaderView: TableHeaderRowView {
    return TableHeaderRowView(columns: [.init(title: displayString.uppercased(), width: .percentage(1.0))], tintColor: Colors.grey300)
  }
}

final class PendingContributionListController: UIViewController {
  
  private let state: RewardsState
  
  init(state: RewardsState) {
    self.state = state
    super.init(nibName: nil, bundle: nil)
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
  private lazy var removeAllButton = UIBarButtonItem(title: Strings.removeAllSitesToolbarButtonTitle, style: .plain, target: self, action: #selector(tappedRemoveAll(_:)))
  private lazy var removeSelectedButton = UIBarButtonItem(title: Strings.removeSelectedToolbarButtonTitle, style: .plain, target: self, action: #selector(tappedRemoveSelected(_:)))
  
  private let tableHeaders: [RewardsType: TableHeaderRowView] = [
    .autoContribute: RewardsType.autoContribute.tableHeaderView,
    .oneTimeTip: RewardsType.oneTimeTip.tableHeaderView,
    .recurringTip: RewardsType.recurringTip.tableHeaderView,
  ]
  
  override func viewDidLoad() {
    super.viewDidLoad()
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    contentView.tableView.allowsMultipleSelectionDuringEditing = true
    
    contentView.tableView.register(PendingContributionCell.self)
    
    removeSelectedButton.isEnabled = false
    
    title = Strings.pendingContributionsTitle.capitalized
    
    navigationController?.toolbar.do {
      $0.appearanceBarTintColor = nil
      $0.appearanceBackgroundColor = nil
    }
    
    navigationItem.rightBarButtonItem = editButton
    toolbarItems = [
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
      removeAllButton,
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil)
    ]
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    navigationController?.setToolbarHidden(false, animated: animated)
    reloadData()
  }
  
  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    navigationController?.setToolbarHidden(true, animated: animated)
  }
  
  @objc private func tappedEdit() {
    contentView.tableView.setEditing(true, animated: true)
    navigationItem.setRightBarButton(doneButton, animated: true)
    setToolbarItems([
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
      removeSelectedButton,
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil)
    ], animated: true)
  }
  
  @objc private func tappedDone() {
    contentView.tableView.setEditing(false, animated: true)
    navigationItem.setRightBarButton(editButton, animated: true)
    setToolbarItems([
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil),
      removeAllButton,
      .init(barButtonSystemItem: .flexibleSpace, target: nil, action: nil)
    ], animated: true)
  }
  
  // MARK: - Data
  
  private var contributionSections: [(type: RewardsType, contributions: [PendingContributionInfo])] = [] {
    didSet {
      removeAllButton.isEnabled = !contributionSections.isEmpty
    }
  }
  
  func reloadData() {
    state.ledger.pendingContributions { [weak self] list in
      guard let self = self else { return }
      // Turns list of pending contributions into sections grouped by their type
      // (recurring tip, tips, auto-contribute).
      self.contributionSections = Dictionary(grouping: list, by: { $0.type })
        // Then turn it into an array of tuples so we can use it in table view
        .map { ($0.key, $0.value) }
        // Sorting the array based on the type (so as to not deal with
        // dictionaries unordered keys)
        .sorted(by: { $0.type.rawValue < $1.type.rawValue })
      self.editButton.isEnabled = !list.isEmpty
      self.contentView.tableView.reloadData()
    }
  }
  
  // MARK: - Actions
  
  @objc private func tappedRemoveAll(_ sender: UIBarButtonItem) {
    contentView.tableView.performBatchUpdates({
      self.contentView.tableView.deleteSections(IndexSet(0..<contributionSections.count), with: .automatic)
      self.contributionSections.removeAll()
    }, completion: { _ in
      self.state.ledger.removeAllPendingContributions { [weak self] _ in
        self?.reloadData()
      }
    })
  }
  
  @objc private func tappedRemoveSelected(_ sender: UIBarButtonItem) {
    guard let selectedIndexes = contentView.tableView.indexPathsForSelectedRows else { return }
    let contributionsToBeRemoved = selectedIndexes.map { contributionSections[$0.section].contributions[$0.row] }
    let group = DispatchGroup()
    contributionsToBeRemoved.forEach {
      group.enter()
      state.ledger.removePendingContribution($0) { _ in
        group.leave()
      }
    }
    group.notify(queue: .main) {
      self.reloadData()
      self.tappedDone() // End editing
    }
  }
}

// MARK: - UITableViewDelegate
extension PendingContributionListController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if tableView.isEditing {
      removeSelectedButton.isEnabled = tableView.indexPathsForSelectedRows != nil
      return
    }
    if contributionSections.isEmpty {
      return
    }
    let contribution = contributionSections[indexPath.section].contributions[indexPath.row]
    let details = PendingContributionDetailController(state: state, contribution: contribution)
    navigationController?.pushViewController(details, animated: true)
  }
  
  func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
    if tableView.isEditing {
      removeSelectedButton.isEnabled = tableView.indexPathsForSelectedRows != nil
    }
  }
}

// MARK: - UITableViewDataSource
extension PendingContributionListController: UITableViewDataSource {
  
  func numberOfSections(in tableView: UITableView) -> Int {
    return contributionSections.count
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return contributionSections[section].contributions.count
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let contribution = contributionSections[safe: indexPath.section]?.contributions[safe: indexPath.row] else {
      assertionFailure("No Publisher found at index: \(indexPath.row)")
      return UITableViewCell()
    }
    let cell = tableView.dequeueReusableCell(for: indexPath) as PendingContributionCell
    cell.selectionStyle = .default
    cell.accessoryType = .disclosureIndicator
    
    if let url = URL(string: contribution.url) {
      state.dataSource?.retrieveFavicon(for: url, faviconURL: URL(string: contribution.faviconUrl)) { data in
        cell.siteImageView.image = data?.image ?? UIImage(frameworkResourceNamed: "defaultFavicon")
        cell.siteImageView.backgroundColor = data?.backgroundColor
      }
    }
    
    cell.verifiedStatusImageView.isHidden = contribution.status == .notVerified
    let provider = " \(contribution.provider.isEmpty ? "" : String(format: Strings.onProviderText, contribution.providerDisplayString))"
    let attrName = NSMutableAttributedString(string: contribution.name).then {
      $0.append(NSMutableAttributedString(string: provider, attributes: [.font: UIFont.boldSystemFont(ofSize: 14.0),
                                                                         .foregroundColor: UIColor.gray]))
    }
    let value = BATValue(contribution.amount)
    cell.siteNameLabel.attributedText = attrName
    cell.tokenView.batContainer.amountLabel.text = value.displayString
    cell.tokenView.usdContainer.amountLabel.text = state.ledger.dollarStringForBATAmount(value.doubleValue, includeCurrencyCode: false)
    return cell
  }
  
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    if contributionSections.isEmpty { return nil }
    return tableHeaders[contributionSections[section].type]
  }
  
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    guard !contributionSections.isEmpty, let header = tableHeaders[contributionSections[section].type] else {
      return CGFloat.leastNormalMagnitude
    }
    return header.systemLayoutSizeFitting(
      CGSize(width: tableView.bounds.width, height: tableView.bounds.height),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).height
  }
}
