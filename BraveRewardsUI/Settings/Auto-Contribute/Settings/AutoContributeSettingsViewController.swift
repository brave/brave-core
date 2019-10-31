/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards

class AutoContributeSettingsViewController: UIViewController {
  
  private let ledger: BraveLedger
  
  init(ledger: BraveLedger) {
    self.ledger = ledger
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  var contentView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    self.view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    contentView.allowVideoContributionsSwitch.addTarget(self, action: #selector(allowVideoValueChanged), for: .valueChanged)
    contentView.allowUnverifiedContributionsSwitch.addTarget(self, action: #selector(allowUnverifiedValueChanged), for: .valueChanged)
    
    reloadData()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    reloadData()
  }
  
  func reloadData() {
    contentView.allowUnverifiedContributionsSwitch.isOn = ledger.allowUnverifiedPublishers
    contentView.allowVideoContributionsSwitch.isOn = ledger.allowVideoContributions

    contentView.tableView.reloadData()
  }
  
  enum Row: Int, CaseIterable {
    case monthlyPayment
    case minimumLength
    case minimumVisits
    case allowUnverifiedContributions
    case allowVideoContributions
    
    func dequeuedCell(from tableView: UITableView, indexPath: IndexPath) -> TableViewCell {
      switch self {
      case .monthlyPayment, .minimumLength, .minimumVisits:
        return tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
      default:
        return tableView.dequeueReusableCell(for: indexPath) as TableViewCell
      }
    }
    var accessoryType: UITableViewCell.AccessoryType {
      switch self {
      case .monthlyPayment, .minimumLength, .minimumVisits:
        return .disclosureIndicator
      default:
        return .none
      }
    }
  }
  
  // MARK: - Actions
  
  @objc private func allowUnverifiedValueChanged() {
    ledger.allowUnverifiedPublishers = contentView.allowUnverifiedContributionsSwitch.isOn
  }
  
  @objc private func allowVideoValueChanged() {
    ledger.allowVideoContributions = contentView.allowVideoContributionsSwitch.isOn
  }
}

extension AutoContributeSettingsViewController: UITableViewDelegate, UITableViewDataSource {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    guard let row = Row(rawValue: indexPath.row) else { return }
    switch row {
    case .monthlyPayment:
      guard let wallet = ledger.walletInfo else { break }
      let monthlyPayment = ledger.contributionAmount
      let choices = wallet.parametersChoices.map { BATValue($0.doubleValue) }
      let selectedIndex = choices.map({ $0.doubleValue }).firstIndex(of: monthlyPayment) ?? 0
      
      let controller = BATValueOptionsSelectionViewController(ledger: ledger, options: choices, selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
        guard let self = self else { return }
        if selectedIndex < choices.count {
          self.ledger.contributionAmount = choices[selectedIndex].doubleValue
        }
        self.navigationController?.popViewController(animated: true)
      }
      
      controller.title = Strings.AutoContributeMonthlyPaymentTitle
      navigationController?.pushViewController(controller, animated: true)
    case .minimumLength:
      let choices = BraveLedger.MinimumVisitDurationOptions.allCases.map { $0.rawValue }
      let selectedIndex = choices.firstIndex(of: ledger.minimumVisitDuration) ?? 0
      let controller = OptionsSelectionViewController(
        options: BraveLedger.MinimumVisitDurationOptions.allCases,
        selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
          guard let self = self else { return }
          if selectedIndex < choices.count {
            self.ledger.minimumVisitDuration = choices[selectedIndex]
          }
          self.navigationController?.popViewController(animated: true)
      }
      controller.title = Strings.AutoContributeMinimumLength
      navigationController?.pushViewController(controller, animated: true)
    case .minimumVisits:
      let choices = BraveLedger.MinimumVisitsOptions.allCases.map { $0.rawValue }
      let selectedIndex = choices.firstIndex(of: ledger.minimumNumberOfVisits) ?? 0
      let controller = OptionsSelectionViewController(
        options: BraveLedger.MinimumVisitsOptions.allCases,
        selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
          guard let self = self else { return }
          if selectedIndex < choices.count {
            self.ledger.minimumNumberOfVisits = choices[selectedIndex]
          }
          self.navigationController?.popViewController(animated: true)
      }
      controller.title = Strings.AutoContributeMinimumVisits
      navigationController?.pushViewController(controller, animated: true)
    default:
      break
    }
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return Row.allCases.count
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let row = Row(rawValue: indexPath.row) else { return UITableViewCell() }
    // Setup
    let cell = row.dequeuedCell(from: tableView, indexPath: indexPath)
    cell.label.font = SettingsUX.bodyFont
    cell.label.numberOfLines = 0
    cell.label.lineBreakMode = .byWordWrapping
    cell.accessoryLabel?.appearanceTextColor = Colors.grey100
    cell.accessoryLabel?.font = SettingsUX.bodyFont
    cell.accessoryType = row.accessoryType
    switch row {
    case .monthlyPayment:
      cell.label.text = Strings.AutoContributeMonthlyPayment
      if let dollarAmount = ledger.dollarStringForBATAmount(ledger.contributionAmount) {
        cell.accessoryLabel?.text = "\(ledger.contributionAmount) \(Strings.BAT) (\(dollarAmount))"
      }
    case .minimumLength:
      cell.label.text = Strings.AutoContributeMinimumLengthMessage
      cell.accessoryLabel?.text = BraveLedger.MinimumVisitDurationOptions(rawValue: ledger.minimumVisitDuration)?.displayString
    case .minimumVisits:
      cell.label.text = Strings.AutoContributeMinimumVisitsMessage
      cell.accessoryLabel?.text = BraveLedger.MinimumVisitsOptions(rawValue: ledger.minimumNumberOfVisits)?.displayString
    case .allowUnverifiedContributions:
      cell.label.text = Strings.AutoContributeToUnverifiedSites
      cell.accessoryView = contentView.allowUnverifiedContributionsSwitch
      cell.selectionStyle = .none
    case .allowVideoContributions:
      cell.label.text = Strings.AutoContributeToVideos
      cell.accessoryView = contentView.allowVideoContributionsSwitch
      cell.selectionStyle = .none
    }
    return cell
  }
}

extension AutoContributeSettingsViewController {
  class View: UIView {
    let tableView = UITableView(frame: .zero, style: .grouped)
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      tableView.backgroundView = UIView().then { $0.backgroundColor = SettingsUX.backgroundColor }
      tableView.tableHeaderView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: CGFloat.leastNormalMagnitude))
      tableView.separatorStyle = .none
      tableView.register(TableViewCell.self)
      tableView.register(Value1TableViewCell.self)
      
      addSubview(tableView)
      tableView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
      tableView.layoutMargins = UIEdgeInsets(top: 15.0, left: 15.0, bottom: 15.0, right: 15.0)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    
    let allowUnverifiedContributionsSwitch = UISwitch().then {
      $0.onTintColor = BraveUX.braveOrange
    }
    
    let allowVideoContributionsSwitch = UISwitch().then {
      $0.onTintColor = BraveUX.braveOrange
    }
  }
}
