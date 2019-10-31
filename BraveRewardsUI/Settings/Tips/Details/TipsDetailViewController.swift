/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards

class TipsDetailViewController: UIViewController {
  private var ledgerObserver: LedgerObserver
  private var tipsView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  private var totalBatTips: Double = 0.0
  private var tipsList: [PublisherInfo] = []
  private let state: RewardsState

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
  
  override func loadView() {
    self.view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    setupLedgerObservers()
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(tappedEditButton))
    
    tipsView.tableView.delegate = self
    tipsView.tableView.dataSource = self

    title = Strings.Tips
    loadData()
  }
  
  private var nextContributionDateView: LabelAccessoryView {
    let view = LabelAccessoryView()
    let dateFormatter = DateFormatter().then {
      $0.dateFormat = Strings.AutoContributeDateFormat
    }
    let date = Date(timeIntervalSince1970: TimeInterval(state.ledger.autoContributeProps.reconcileStamp))
    view.label.text = dateFormatter.string(from: date)
    view.bounds = CGRect(origin: .zero, size: view.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize))
    return view
  }
  
  private func loadData() {
    _ = getTipsThisMonth().then {
      
      if let oneTimeTips = BATValue(probi: "\($0.oneTimeDonation)"),
        let recurringTips = BATValue(probi: "\($0.recurringDonation)") {
        totalBatTips = oneTimeTips.doubleValue + recurringTips.doubleValue
      }
    }
    
    state.ledger.listOneTimeTips {[weak self] infoList in
      guard let self = self else { return }
      infoList.forEach({$0.category = Int32(RewardsType.oneTimeTip.rawValue)})
      self.tipsList.append(contentsOf: infoList)
      (self.view as? View)?.tableView.reloadData()
    }
    
    state.ledger.listRecurringTips {[weak self] infoList in
      guard let self = self else { return }
      infoList.forEach({$0.category = Int32(RewardsType.recurringTip.rawValue)})
      self.tipsList.insert(contentsOf: infoList, at: 0)
      if !self.tipsView.tableView.isEditing {
        self.navigationItem.rightBarButtonItem?.isEnabled = !self.tipsList.filter { $0.rewardsCategory == .recurringTip }.isEmpty
      }
      (self.view as? View)?.tableView.reloadData()
    }
  }
  
  private let headerView = TableHeaderRowView(
    columns: [
      TableHeaderRowView.Column(
        title: Strings.Site.uppercased(),
        width: .percentage(0.7)
      ),
      TableHeaderRowView.Column(
        title: Strings.Tokens.uppercased(),
        width: .percentage(0.3),
        align: .right
      ),
    ],
    tintColor: BraveUX.tipsTintColor
  )
  
  // MARK: - Actions
  
  @objc private func tappedEditButton() {
    tipsView.tableView.setEditing(true, animated: true)
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDoneButton))
  }
  
  @objc private func tappedDoneButton() {
    tipsView.tableView.setEditing(false, animated: true)
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(tappedEditButton))
    navigationItem.rightBarButtonItem?.isEnabled = !self.tipsList.filter { $0.rewardsCategory == .recurringTip }.isEmpty
  }
}

extension TipsDetailViewController: UITableViewDataSource, UITableViewDelegate {
  private enum Section: Int, CaseIterable {
    case summary
    case tips
  }
  
  func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }
  
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    guard let typedSection = Section(rawValue: section), typedSection == .tips else { return nil }
    return headerView
  }
  
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    guard let typedSection = Section(rawValue: section), typedSection == .tips else { return 0.0 }
    return headerView.systemLayoutSizeFitting(
      CGSize(width: tableView.bounds.width, height: tableView.bounds.height),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).height
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    guard let typedSection = Section(rawValue: section) else { return 0 }
    switch typedSection {
    case .summary:
      let hasRecurringTips = tipsList.contains { $0.rewardsCategory == .recurringTip }
      return 1 + (hasRecurringTips ? 1 : 0)
    case .tips:
      return tipsList.isEmpty ? 1 : tipsList.count
    }
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    guard Section(rawValue: indexPath.section) == .tips,
      !tipsList.isEmpty,
      let tipURL = URL(string: tipsList[indexPath.row].url)
    else { return }
    state.delegate?.loadNewTabWithURL(tipURL)
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let section = Section(rawValue: indexPath.section) else {
      assertionFailure()
      return UITableViewCell()
    }
    switch section {
    case .summary:
      if indexPath.row == 0 {
        let cell = tableView.dequeueReusableCell(for: indexPath) as TipsSummaryTableCell
        cell.layoutMargins = UIEdgeInsets(top: 25, left: 25, bottom: 25, right: 25)
        cell.batValueView.amountLabel.text = "\(totalBatTips)"
        cell.usdValueView.amountLabel.text = state.ledger.dollarStringForBATAmount(totalBatTips, includeCurrencyCode: false)
        return cell
      }
      // Next Contribution Row
      let cell = tableView.dequeueReusableCell(for: indexPath) as TableViewCell
      cell.label.font = SettingsUX.bodyFont
      cell.label.text = Strings.MonthlyTippingNextDate
      cell.accessoryView = nextContributionDateView
      cell.selectionStyle = .none
      return cell
    case .tips:
      if tipsList.isEmpty {
        let cell = tableView.dequeueReusableCell(for: indexPath) as EmptyTableCell
        cell.label.text = Strings.EmptyTipsText
        cell.selectionStyle = .none
        return cell
      }
      let cell = tableView.dequeueReusableCell(for: indexPath) as TipsTableCell
      let tip = tipsList[indexPath.row]
      let provider = " \(tip.provider.isEmpty ? "" : String(format: Strings.OnProviderText, tip.providerDisplayString))"
      
      let attrName = NSMutableAttributedString(string: tip.name).then {
        $0.append(NSMutableAttributedString(string: provider, attributes: [.font: UIFont.boldSystemFont(ofSize: 14.0),
                                                                           .foregroundColor: UIColor.gray]))
      }
      cell.siteNameLabel.attributedText = attrName
      
      cell.siteImageView.image = UIImage(frameworkResourceNamed: "defaultFavicon")
      setFavicon(identifier: tip.id, pageURL: tip.url, faviconURL: tip.faviconUrl)
      cell.verifiedStatusImageView.isHidden = tip.status == .notVerified
      let contribution = tip.weight
      switch tip.rewardsCategory {
      case .oneTimeTip:
        let value = BATValue(probi: "\(contribution)")
        cell.typeNameLabel.text = Strings.OneTimeText + Date.stringFrom(reconcileStamp: tip.reconcileStamp)
        cell.tokenView.batContainer.amountLabel.text = value?.displayString
        cell.tokenView.usdContainer.amountLabel.text = state.ledger.dollarStringForBATAmount(value?.doubleValue ?? 0, includeCurrencyCode: false)
      case .recurringTip:
        cell.tokenView.batContainer.amountLabel.text = BATValue(contribution).displayString
        cell.tokenView.usdContainer.amountLabel.text = state.ledger.dollarStringForBATAmount(contribution, includeCurrencyCode: false)
        cell.typeNameLabel.text = Strings.RecurringText
      default:
        cell.tokenView.batContainer.amountLabel.text = ""
        cell.tokenView.usdContainer.amountLabel.text = ""
        cell.typeNameLabel.text = ""
      }
      return cell
    }
  }
  
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    guard Section(rawValue: indexPath.section) == .tips &&
      !tipsList.isEmpty &&
      tipsList[indexPath.row].rewardsCategory == .recurringTip else { return false }
    return true
  }
  
  func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
    guard Section(rawValue: indexPath.section) == .tips &&
      !tipsList.isEmpty &&
      tipsList[indexPath.row].rewardsCategory == .recurringTip else { return .none }
    return .delete
  }
  
  func tableView(_ tableView: UITableView, titleForDeleteConfirmationButtonForRowAt indexPath: IndexPath) -> String? {
    return Strings.Remove
  }
  
  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    guard Section(rawValue: indexPath.section) == .tips &&
      !tipsList.isEmpty &&
      tipsList[indexPath.row].rewardsCategory == .recurringTip else { return }
    let publisherID = tipsList[indexPath.row].id
    state.ledger.removeRecurringTip(publisherId: publisherID)
  }
}

extension TipsDetailViewController {
  class View: UIView {
    let tableView = UITableView(frame: .zero, style: .grouped)
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      tableView.backgroundView = UIView().then {
        $0.backgroundColor = SettingsUX.backgroundColor
      }
      tableView.tableHeaderView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: CGFloat.leastNormalMagnitude))
      tableView.separatorInset = .zero
      tableView.register(TipsTableCell.self)
      tableView.register(TipsSummaryTableCell.self)
      tableView.register(TableViewCell.self)
      tableView.register(EmptyTableCell.self)
      
      addSubview(tableView)
      tableView.snp.makeConstraints {
        $0.edges.equalTo(self)
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}

extension TipsDetailViewController {
  fileprivate func getTipsThisMonth() -> BalanceReportInfo {
    let month = Date().currentMonthNumber
    let year = Date().currentYear
    var report = BalanceReportInfo()
    state.ledger.balanceReport(for: ActivityMonth(rawValue: month) ?? .any, year: Int32(year)) {
      if let balance = $0 { report = balance }
    }
    return report
  }
  
  fileprivate func setFavicon(identifier: String, pageURL: String, faviconURL: String?) {
    if let pageURL = URL(string: pageURL) {
      state.dataSource?.retrieveFavicon(for: pageURL, faviconURL: URL(string: faviconURL ?? ""), completion: {[weak self] favData in
        guard let self = self,
          let image = favData?.image,
          let index = self.tipsList.firstIndex(where: {$0.id == identifier}) else {
            return
        }

        if let tableView = (self.view as? View)?.tableView,
          let cell = tableView.cellForRow(at: IndexPath(row: index, section: Section.tips.rawValue)) as? TipsTableCell {
          cell.siteImageView.image = image
        }
      })
    }
  }
  
  func setupLedgerObservers() {
    ledgerObserver.recurringTipRemoved = { [weak self] key in
      guard let self = self, self.isViewLoaded else {
        return
      }
      self.tipsList.removeAll(where: {
        $0.category == RewardsType.recurringTip.rawValue &&
        $0.id == key
      })
      self.tipsView.tableView.reloadData()
    }
  }
}
