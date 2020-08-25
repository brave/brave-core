// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards

final class PendingContributionDetailController: UIViewController {
  private let state: RewardsState
  private let contribution: PendingContributionInfo
  
  init(state: RewardsState, contribution: PendingContributionInfo) {
    self.state = state
    self.contribution = contribution
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
  
  override func viewDidLoad() {
    super.viewDidLoad()
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    contentView.tableView.register(Value1TableViewCell.self)
    contentView.tableView.register(TableViewCell.self)
    
    title = contribution.name
  }
  
  private let dateFormatter = DateFormatter().then {
    $0.dateStyle = .short
  }
}

extension PendingContributionDetailController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if indexPath.section != Section.remove.rawValue { return }
    state.ledger.removePendingContribution(contribution) { [weak self] _ in
      self?.navigationController?.popViewController(animated: true)
    }
  }
}

extension PendingContributionDetailController: UITableViewDataSource {
  enum Section: Int, CaseIterable {
    case details
    case remove
  }
  enum DetailsRow: Int, CaseIterable {
    case type
    case amount
    case pendingUntil
  }
  
  func numberOfSections(in tableView: UITableView) -> Int {
    Section.allCases.count
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return section == Section.details.rawValue ? DetailsRow.allCases.count : 1
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let section = Section(rawValue: indexPath.section) else {
      return UITableViewCell()
    }
    switch section {
    case .details:
      guard let row = DetailsRow(rawValue: indexPath.row) else {
        return UITableViewCell()
      }
      let cell = tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
      cell.label.font = SettingsUX.bodyFont
      cell.accessoryLabel?.font = SettingsUX.bodyFont
      cell.manualSeparators = []
      cell.selectionStyle = .none
      switch row {
      case .type:
        cell.label.text = Strings.pendingContributionType
        cell.accessoryLabel?.text = contribution.type.displayString
      case .amount:
        cell.label.text = Strings.pendingContributionAmount
        let value = BATValue(contribution.amount)
        var amountString = "\(value.displayString) \(Strings.BAT)"
        cell.accessoryLabel?.text = amountString
      case .pendingUntil:
        cell.label.text = Strings.pendingContributionPendingUntil
        cell.accessoryLabel?.text = dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(contribution.expirationDate)))
      }
      return cell
    case .remove:
      let cell = tableView.dequeueReusableCell(for: indexPath) as TableViewCell
      cell.label.font = SettingsUX.bodyFont
      cell.manualSeparators = []
      cell.selectionStyle = .default
      cell.label.textAlignment = .center
      cell.label.appearanceTextColor = BraveUX.braveOrange
      cell.label.text = Strings.removePendingContribution
      return cell
    }
  }
}
