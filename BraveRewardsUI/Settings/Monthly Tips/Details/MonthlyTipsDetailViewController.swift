// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards

class MonthlyTipsDetailViewController: UIViewController {
  private let state: RewardsState
  private var publishers: [PublisherInfo] = []
  private var totalBatTips: Double = 0.0
 
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
    self.view = SettingsTableView()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = Strings.settingsMonthlyTipsTitle
    
    contentView.tableView.register(TipsTableCell.self)
    contentView.tableView.register(TipsSummaryTableCell.self)
    contentView.tableView.register(TableViewCell.self)
    contentView.tableView.register(EmptyTableCell.self)
    
    contentView.tableView.dataSource = self
    contentView.tableView.delegate = self
    
    navigationItem.rightBarButtonItem = editButton
    
    reloadData()
  }
  
  private lazy var editButton = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(tappedEdit))
  private lazy var doneButton = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
  
  private let headerView = TableHeaderRowView(
    columns: [
      TableHeaderRowView.Column(
        title: Strings.site.uppercased(),
        width: .percentage(0.7)
      ),
      TableHeaderRowView.Column(
        title: Strings.tokens.uppercased(),
        width: .percentage(0.3),
        align: .right
      ),
    ],
    tintColor: BraveUX.tipsTintColor
  )
  
  private var nextContributionDateView: LabelAccessoryView {
    let view = LabelAccessoryView()
    let dateFormatter = DateFormatter().then {
      $0.dateFormat = Strings.autoContributeDateFormat
    }
    let date = Date(timeIntervalSince1970: TimeInterval(state.ledger.autoContributeProperties.reconcileStamp))
    view.label.text = dateFormatter.string(from: date)
    view.bounds = CGRect(origin: .zero, size: view.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize))
    return view
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
  
  func reloadData() {
    state.ledger.listRecurringTips { [weak self] publishers in
      guard let self = self else { return }
      self.publishers = publishers
      self.editButton.isEnabled = !publishers.isEmpty
      self.totalBatTips = publishers.reduce(0, { result, publisher -> Double in
        return result + publisher.weight
      })
      self.contentView.tableView.reloadData()
    }
  }
}

// MARK: - UITableViewDelegate
extension MonthlyTipsDetailViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, willBeginEditingRowAt indexPath: IndexPath) {
    navigationItem.setRightBarButton(doneButton, animated: true)
  }
  
  func tableView(_ tableView: UITableView, didEndEditingRowAt indexPath: IndexPath?) {
    navigationItem.setRightBarButton(editButton, animated: true)
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard let section = Section(rawValue: indexPath.section) else { return }
    switch section {
    case .summary:
      break
    case .contributions:
      if !publishers.isEmpty, let url = URL(string: publishers[indexPath.row].url) {
        state.delegate?.loadNewTabWithURL(url)
      }
    }
  }
}

// MARK: - UITableViewDataSource
extension MonthlyTipsDetailViewController: UITableViewDataSource {
  enum Section: Int, CaseIterable {
    case summary
    case contributions
  }
  
  func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    guard let section = Section(rawValue: section) else { return 0 }
    switch section {
    case .summary:
      return 2
    case .contributions:
      return publishers.isEmpty ? 1 : publishers.count
    }
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let section = Section(rawValue: indexPath.section) else {
      assertionFailure("Not a valid index path: \(indexPath)")
      return UITableViewCell()
    }
    switch section {
    case .summary:
      if indexPath.row == 0 {
        let cell = tableView.dequeueReusableCell(for: indexPath) as TipsSummaryTableCell
        cell.totalTipsThisMonthLabel.text = Strings.monthlyContributionsTotalThisMonth
        cell.layoutMargins = UIEdgeInsets(top: 25, left: 25, bottom: 25, right: 25)
        cell.batValueView.amountLabel.text = "\(totalBatTips)"
        cell.usdValueView.amountLabel.text = state.ledger.dollarStringForBATAmount(totalBatTips, includeCurrencyCode: false)
        return cell
      }
      // Next Contribution Row
      let cell = tableView.dequeueReusableCell(for: indexPath) as TableViewCell
      cell.label.font = SettingsUX.bodyFont
      cell.label.text = Strings.monthlyTippingNextDate
      cell.accessoryView = nextContributionDateView
      cell.selectionStyle = .none
      return cell
    case .contributions:
      if publishers.isEmpty {
        let cell = tableView.dequeueReusableCell(for: indexPath) as EmptyTableCell
        cell.label.text = Strings.emptyTipsText
        cell.selectionStyle = .none
        return cell
      }
      let cell = tableView.dequeueReusableCell(for: indexPath) as TipsTableCell
      let publisher = publishers[indexPath.row]
      let provider = " \(publisher.provider.isEmpty ? "" : String(format: Strings.onProviderText, publisher.providerDisplayString))"
      
      let attrName = NSMutableAttributedString(string: publisher.name).then {
        $0.append(NSMutableAttributedString(string: provider, attributes: [.font: UIFont.boldSystemFont(ofSize: 14.0),
                                                                           .foregroundColor: UIColor.gray]))
      }
      cell.siteNameLabel.attributedText = attrName
      cell.siteImageView.image = UIImage(frameworkResourceNamed: "defaultFavicon")
      if let url = URL(string: publisher.url) {
        state.dataSource?.retrieveFavicon(for: url, on: cell.siteImageView)
      }
      cell.verifiedStatusImageView.isHidden = publisher.status == .notVerified
      let contribution = publisher.weight
      
      cell.tokenView.batContainer.amountLabel.text = BATValue(contribution).displayString
      cell.tokenView.usdContainer.amountLabel.text = state.ledger.dollarStringForBATAmount(contribution, includeCurrencyCode: false)
      return cell
    }
  }
  
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    if section != Section.contributions.rawValue { return nil }
    return headerView
  }
  
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    if section != Section.contributions.rawValue { return 0.0 }
    return headerView.systemLayoutSizeFitting(
      CGSize(width: tableView.bounds.width, height: tableView.bounds.height),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).height
  }
  
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    return indexPath.section == Section.contributions.rawValue && !publishers.isEmpty
  }
  
  func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
    if indexPath.section == Section.contributions.rawValue { return .delete }
    return .none
  }
  
  func tableView(_ tableView: UITableView, titleForDeleteConfirmationButtonForRowAt indexPath: IndexPath) -> String? {
    return Strings.remove
  }
  
  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    if indexPath.section != Section.contributions.rawValue { return }
    let publisher = publishers[indexPath.row]
    tableView.performBatchUpdates({
      publishers.remove(at: indexPath.row)
      totalBatTips -= publisher.weight
      tableView.deleteRows(at: [indexPath], with: .automatic)
      if publishers.isEmpty {
        tableView.insertRows(at: [IndexPath(row: 0, section: Section.contributions.rawValue)], with: .automatic)
      }
    }, completion: { _ in
      self.state.ledger.removeRecurringTip(publisherId: publisher.id)
    })
  }
}
