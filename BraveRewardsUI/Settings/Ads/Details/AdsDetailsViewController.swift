// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards

class AdsDetailsViewController: UIViewController {
  
  var contentView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  let state: RewardsState
  private let observer: LedgerObserver
  
  init(state: RewardsState) {
    self.state = state
    observer = LedgerObserver(ledger: state.ledger)
    super.init(nibName: nil, bundle: nil)
    
    state.ledger.add(observer)
    observer.confirmationsTransactionHistoryDidChange = { [weak self] in
      self?.fetchAdsDetails()
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func loadView() {
    view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    title = Strings.SettingsAdsTitle
    
    fetchAdsDetails()
  }
  
  func fetchAdsDetails() {
    state.ledger.adsDetailsForCurrentCycle { [weak self] adsReceived, estimatedEarnings, nextPaymentDate in
      self?.updateAdsInfo(adsReceived: adsReceived, estimatedEarnings: estimatedEarnings, nextPaymentDate: nextPaymentDate)
    }
  }
  
  func updateAdsInfo(adsReceived: Int, estimatedEarnings: Double, nextPaymentDate: Date?) {
    self.adsReceived = adsReceived
    self.estimatedEarnings = estimatedEarnings
    if let date = nextPaymentDate {
      let formatter = DateFormatter().then {
        $0.dateFormat = Strings.AdsPayoutDateFormat
      }
      nextPaymentDateView.label.text = formatter.string(from: date)
    } else {
      nextPaymentDateView.label.text = ""
    }
    nextPaymentDateView.bounds = CGRect(origin: .zero, size: nextPaymentDateView.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize))
    contentView.tableView.reloadData()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    contentView.tableView.reloadData()
  }
  
  private let nextPaymentDateView = LabelAccessoryView()
  
  private let adsPerHourOptions = [
    Strings.OneAdPerHour,
    Strings.TwoAdsPerHour,
    Strings.ThreeAdsPerHour,
    Strings.FourAdsPerHour,
    Strings.FiveAdsPerHour
  ]
  
  private var adsReceived: Int = 0
  private var estimatedEarnings: Double = 0.0
}

extension AdsDetailsViewController: UITableViewDelegate, UITableViewDataSource {
  private enum Row: Int, CaseIterable {
    case adsPerHour
    case currentEarnings
    case nextPayment
    case numberOfAdsReceived
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    
    guard let row = Row(rawValue: indexPath.row) else { fatalError() }
    
    if row == .adsPerHour {
      let choices = [Int](1..<adsPerHourOptions.count+1)
      let selectedIndex = state.ads.adsPerHour - 1
      let controller = OptionsSelectionViewController(
        options: adsPerHourOptions,
        selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
          guard let self = self else { return }
          if selectedIndex < choices.count {
            self.state.ads.adsPerHour = choices[selectedIndex]
          }
          self.navigationController?.popViewController(animated: true)
      }
      controller.title = Strings.NumberOfAdsPerHourOptionsTitle
      navigationController?.pushViewController(controller, animated: true)
    }
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return Row.allCases.count
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let row = Row(rawValue: indexPath.row) else { fatalError() }
    let cell = tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
    cell.label.font = SettingsUX.bodyFont
    cell.label.numberOfLines = 0
    cell.label.lineBreakMode = .byWordWrapping
    cell.accessoryLabel?.textColor = Colors.grey100
    cell.accessoryLabel?.font = SettingsUX.bodyFont
    cell.accessoryType = .none
    switch row {
    case .adsPerHour:
      cell.accessoryType = .disclosureIndicator
      cell.label.text = Strings.AdsMaxPerHour
      let adsPerHour = state.ads.adsPerHour
      if adsPerHour - 1 < adsPerHourOptions.count {
        cell.accessoryLabel?.text = adsPerHourOptions[adsPerHour - 1]
      }
    case .currentEarnings:
      cell.label.text = Strings.AdsEstimatedEarnings
      cell.selectionStyle = .none
      cell.accessoryView = BATUSDPairView().then {
        $0.batContainer.amountLabel.text = BATValue(estimatedEarnings).displayString
        $0.usdContainer.amountLabel.text = state.ledger.dollarStringForBATAmount(estimatedEarnings, includeCurrencyCode: false)
        $0.bounds = CGRect(origin: .zero, size: $0.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize))
      }
    case .nextPayment:
      cell.label.text = Strings.NextPaymentDate
      cell.selectionStyle = .none
      cell.accessoryView = nextPaymentDateView
    case .numberOfAdsReceived:
      cell.label.text = Strings.AdNotificationsReceived
      cell.selectionStyle = .none
      cell.accessoryLabel?.text = "\(adsReceived)"
    }
    return cell
  }
}

extension AdsDetailsViewController {
  class View: UIView {
    let tableView = UITableView(frame: .zero, style: .grouped)
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      tableView.backgroundView = UIView().then {
        $0.backgroundColor = SettingsUX.backgroundColor
      }
      tableView.tableHeaderView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: CGFloat.leastNormalMagnitude))
      tableView.separatorInset = .zero
      tableView.register(TableViewCell.self)
      tableView.register(Value1TableViewCell.self)
      tableView.layoutMargins = UIEdgeInsets(top: 15.0, left: 15.0, bottom: 15.0, right: 15.0)
      
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
