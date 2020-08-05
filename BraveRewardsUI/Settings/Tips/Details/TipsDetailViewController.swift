/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards

class TipsDetailViewController: UIViewController {
  private var ledgerObserver: LedgerObserver
  private var tipsView: SettingsTableView {
    return view as! SettingsTableView // swiftlint:disable:this force_cast
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
    self.view = SettingsTableView()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    setupLedgerObservers()
    
    tipsView.tableView.register(TipsTableCell.self)
    tipsView.tableView.register(TipsSummaryTableCell.self)
    tipsView.tableView.register(TableViewCell.self)
    tipsView.tableView.register(EmptyTableCell.self)
    
    tipsView.tableView.delegate = self
    tipsView.tableView.dataSource = self

    title = Strings.tips
    loadData()
  }
  
  private func loadData() {
    getTipsThisMonth { [weak self] report in
      guard let self = self else { return }
      self.totalBatTips = report.oneTimeDonation + report.recurringDonation
      (self.view as? SettingsTableView)?.tableView.reloadData()
    }
    
    state.ledger.listOneTimeTips { [weak self] infoList in
      guard let self = self else { return }
      infoList.forEach({$0.category = Int32(RewardsType.oneTimeTip.rawValue)})
      self.tipsList.append(contentsOf: infoList)
      (self.view as? SettingsTableView)?.tableView.reloadData()
    }
  }
  
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
      return 1
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
      let cell = tableView.dequeueReusableCell(for: indexPath) as TipsSummaryTableCell
      cell.layoutMargins = UIEdgeInsets(top: 25, left: 25, bottom: 25, right: 25)
      cell.batValueView.amountLabel.text = "\(totalBatTips)"
      cell.usdValueView.amountLabel.text = state.ledger.dollarStringForBATAmount(totalBatTips, includeCurrencyCode: false)
      return cell
    case .tips:
      if tipsList.isEmpty {
        let cell = tableView.dequeueReusableCell(for: indexPath) as EmptyTableCell
        cell.label.text = Strings.emptyTipsText
        cell.selectionStyle = .none
        return cell
      }
      let cell = tableView.dequeueReusableCell(for: indexPath) as TipsTableCell
      let tip = tipsList[indexPath.row]
      let provider = " \(tip.provider.isEmpty ? "" : String(format: Strings.onProviderText, tip.providerDisplayString))"
      
      let attrName = NSMutableAttributedString(string: tip.name).then {
        $0.append(NSMutableAttributedString(string: provider, attributes: [.font: UIFont.boldSystemFont(ofSize: 14.0),
                                                                           .foregroundColor: UIColor.gray]))
      }
      cell.siteNameLabel.attributedText = attrName
      
      cell.siteImageView.image = UIImage(frameworkResourceNamed: "defaultFavicon")
      if let url = URL(string: tip.url) {
        state.dataSource?.retrieveFavicon(for: url, on: cell.siteImageView)
      }
      cell.verifiedStatusImageView.isHidden = tip.status == .notVerified
      cell.typeNameLabel.text = Strings.oneTimeText + Date.stringFrom(reconcileStamp: tip.reconcileStamp)
      cell.tokenView.batContainer.amountLabel.text = "\(tip.weight)"
      cell.tokenView.usdContainer.amountLabel.text = state.ledger.dollarStringForBATAmount(tip.weight, includeCurrencyCode: false)
      return cell
    }
  }
}

extension TipsDetailViewController {
  fileprivate func getTipsThisMonth(_ completion: @escaping (BalanceReportInfo) -> Void) {
    let month = Date().currentMonthNumber
    let year = Date().currentYear
    state.ledger.balanceReport(for: ActivityMonth(rawValue: month) ?? .any, year: Int32(year)) {
      completion($0 ?? .init())
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
