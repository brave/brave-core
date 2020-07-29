// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import BraveUI

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
    
    title = Strings.settingsAdsTitle
    
    generateRows()

    fetchAdsDetails()
  }
  
  func generateRows() {
    rows.removeAll()

    rows.append(.adsPerHour)
    if state.ads.shouldAllowSubdivisionTargeting {
      rows.append(.subdivisionTargeting)
    }
    rows.append(.currentEarnings)
    rows.append(.nextPayment)
    rows.append(.numberOfAdsReceived)
  }

  func fetchAdsDetails() {
    state.ads.detailsForCurrentCycle { [weak self] adsReceived, estimatedEarnings, nextPaymentDate in
      self?.updateAdsInfo(adsReceived: adsReceived, estimatedEarnings: estimatedEarnings, nextPaymentDate: nextPaymentDate)
    }
  }
  
  func updateAdsInfo(adsReceived: Int, estimatedEarnings: Double, nextPaymentDate: Date?) {
    self.adsReceived = adsReceived
    self.estimatedEarnings = estimatedEarnings
    if let date = nextPaymentDate {
      let formatter = DateFormatter().then {
        $0.dateFormat = Strings.adsPayoutDateFormat
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
    Strings.oneAdPerHour,
    Strings.twoAdsPerHour,
    Strings.threeAdsPerHour,
    Strings.fourAdsPerHour,
    Strings.fiveAdsPerHour
  ]

  // TODO(https://github.com/brave/brave-browser/issues/10316) return subdivisions from ads lib
  private let subdivisionTargetingOptions: KeyValuePairs = [
    "AUTO": Strings.adsSubdivisionTargetingAutoDetect,
    "DISABLED": Strings.adsSubdivisionTargetingDisable,
    "US-AL": "Alabama",
    "US-AK": "Alaska",
    "US-AZ": "Arizona",
    "US-AR": "Arkansas",
    "US-CA": "California",
    "US-CO": "Colorado",
    "US-CT": "Connecticut",
    "US-DE": "Delaware",
    "US-FL": "Florida",
    "US-GA": "Georgia",
    "US-HI": "Hawaii",
    "US-ID": "Idaho",
    "US-IL": "Illinois",
    "US-IN": "Indiana",
    "US-IA": "Iowa",
    "US-KS": "Kansas",
    "US-KY": "Kentucky",
    "US-LA": "Louisiana",
    "US-ME": "Maine",
    "US-MD": "Maryland",
    "US-MA": "Massachusetts",
    "US-MI": "Michigan",
    "US-MN": "Minnesota",
    "US-MS": "Mississippi",
    "US-MO": "Missouri",
    "US-MT": "Montana",
    "US-NE": "Nebraska",
    "US-NV": "Nevada",
    "US-NH": "New Hampshire",
    "US-NJ": "New Jersey",
    "US-NM": "New Mexico",
    "US-NY": "New York",
    "US-NC": "North Carolina",
    "US-ND": "North Dakota",
    "US-OH": "Ohio",
    "US-OK": "Oklahoma",
    "US-OR": "Oregon",
    "US-PA": "Pennsylvania",
    "US-RI": "Rhode Island",
    "US-SC": "South Carolina",
    "US-SD": "South Dakota",
    "US-TN": "Tennessee",
    "US-TX": "Texas",
    "US-UT": "Utah",
    "US-VT": "Vermont",
    "US-VA": "Virginia",
    "US-WA": "Washington",
    "US-WV": "West Virginia",
    "US-WI": "Wisconsin",
    "US-WY": "Wyoming"
  ]

  private var adsReceived: Int = 0
  private var estimatedEarnings: Double = 0.0

  private var rows: [Row] = []
}

extension AdsDetailsViewController: UITableViewDelegate, UITableViewDataSource {
  private enum Row: Int, CaseIterable {
    case adsPerHour
    case subdivisionTargeting
    case currentEarnings
    case nextPayment
    case numberOfAdsReceived
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    
    guard let row = rows[safe: indexPath.row] else { fatalError() }
    
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
      controller.title = Strings.numberOfAdsPerHourOptionsTitle
      navigationController?.pushViewController(controller, animated: true)
    } else if row == .subdivisionTargeting {
      guard let selectedIndex = subdivisionTargetingOptions.firstIndex(where: { $0.0 == state.ads.subdivisionTargetingCode }) else { return }

      let controller = OptionsSelectionViewController(
        options: subdivisionTargetingOptions.map { $0.value },
        selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
          guard let self = self else { return }
          self.state.ads.subdivisionTargetingCode = self.subdivisionTargetingOptions[selectedIndex].0
          self.navigationController?.popViewController(animated: true)
      }
      controller.title = Strings.adsSubdivisionTargeting
      navigationController?.pushViewController(controller, animated: true)
    }
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return rows.count
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let row = rows[safe: indexPath.row] else { fatalError() }
    let cell = tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
    cell.label.font = SettingsUX.bodyFont
    cell.label.numberOfLines = 0
    cell.label.lineBreakMode = .byWordWrapping
    cell.accessoryLabel?.font = SettingsUX.bodyFont
    cell.accessoryType = .none
    switch row {
    case .adsPerHour:
      cell.accessoryType = .disclosureIndicator
      cell.label.text = Strings.adsMaxPerHour
      let adsPerHour = state.ads.adsPerHour
      if adsPerHour - 1 < adsPerHourOptions.count {
        cell.accessoryLabel?.text = adsPerHourOptions[adsPerHour - 1]
      }
    case .subdivisionTargeting:
      cell.accessoryType = .disclosureIndicator

      cell.label.text = Strings.adsSubdivisionTargeting

      var adsSubdivisionTargetingCode: String
      if state.ads.subdivisionTargetingCode == "AUTO" {
        adsSubdivisionTargetingCode = state.ads.automaticallyDetectedSubdivisionTargetingCode
      } else {
        adsSubdivisionTargetingCode = state.ads.subdivisionTargetingCode
      }

      if adsSubdivisionTargetingCode == "DISABLED" {
        cell.accessoryLabel?.text = Strings.adsSubdivisionTargetingDisabled
      } else {
        guard let selectedIndex = subdivisionTargetingOptions.firstIndex(where: { $0.0 == adsSubdivisionTargetingCode }) else { fatalError() }
        cell.accessoryLabel?.text = subdivisionTargetingOptions[selectedIndex].1
      }
    case .currentEarnings:
      cell.label.text = Strings.adsEstimatedEarnings
      cell.selectionStyle = .none
      cell.accessoryView = BATUSDPairView().then {
        $0.batContainer.amountLabel.text = BATValue(estimatedEarnings).displayString
        $0.usdContainer.amountLabel.text = state.ledger.dollarStringForBATAmount(estimatedEarnings, includeCurrencyCode: false)
        $0.bounds = CGRect(origin: .zero, size: $0.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize))
      }
    case .nextPayment:
      cell.label.text = Strings.nextPaymentDate
      cell.selectionStyle = .none
      cell.accessoryView = nextPaymentDateView
    case .numberOfAdsReceived:
      cell.label.text = Strings.adNotificationsReceived
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
