/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards

class AutoContributeDetailViewController: UIViewController {
  private var ledgerObserver: LedgerObserver
  private var contentView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  // Just copy pasted this in, needs design specific for auto-contribute
  private var publishersCount: UInt = 0
  private var excludedPublishersCount: UInt = 0
  private let state: RewardsState
  
  init(state: RewardsState) {
    self.state = state
    ledgerObserver = LedgerObserver(ledger: state.ledger)
    state.ledger.add(ledgerObserver)
    super.init(nibName: nil, bundle: nil)
    setupLedgerObservers()
  }
  
  func setupLedgerObservers() {
    ledgerObserver.fetchedBalance = { [weak self] in
      // Monthly payment options don't exist until after balance is fetched
      self?.contentView.tableView.reloadData()
    }
    ledgerObserver.excludedSitesChanged = { [weak self] _, _ in
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
        // TODO: Remove this delay after DB migration
        self?.reloadData()
      }
    }
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
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    contentView.allowVideoContributionsSwitch.addTarget(self, action: #selector(allowVideoValueChanged), for: .valueChanged)
    contentView.allowUnverifiedContributionsSwitch.addTarget(self, action: #selector(allowUnverifiedValueChanged), for: .valueChanged)
    
    title = Strings.autoContribute
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    reloadData()
  }
  
  private var nextContributionDateView: LabelAccessoryView {
    let view = LabelAccessoryView()
    let dateFormatter = DateFormatter().then {
      $0.dateFormat = Strings.autoContributeDateFormat
    }
    let reconcileDate = Date(timeIntervalSince1970: TimeInterval(state.ledger.autoContributeProps.reconcileStamp))
    view.label.text = dateFormatter.string(from: reconcileDate)
    view.bounds = CGRect(origin: .zero, size: view.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize))
    return view
  }
  
  private func reloadData() {
    contentView.allowUnverifiedContributionsSwitch.isOn = state.ledger.allowUnverifiedPublishers
    contentView.allowVideoContributionsSwitch.isOn = state.ledger.allowVideoContributions
    
    let filter = state.ledger.supportedPublishersFilter
    state.ledger.listActivityInfo(fromStart: 0, limit: 0, filter: filter) { pubs in
      self.publishersCount = UInt(pubs.count)
      self.contentView.tableView.reloadData()
    }
    let excludedFilter = state.ledger.excludedPublishersFilter
    state.ledger.listActivityInfo(fromStart: 0, limit: 0, filter: excludedFilter) { pubs in
      self.excludedPublishersCount = UInt(pubs.count)
      self.contentView.tableView.reloadData()
    }
    contentView.tableView.reloadData()
  }
  
  // MARK: - Actions
  
  @objc private func allowUnverifiedValueChanged() {
    state.ledger.allowUnverifiedPublishers = contentView.allowUnverifiedContributionsSwitch.isOn
  }
  
  @objc private func allowVideoValueChanged() {
    state.ledger.allowVideoContributions = contentView.allowVideoContributionsSwitch.isOn
  }
}

extension AutoContributeDetailViewController: UITableViewDataSource, UITableViewDelegate {
  private enum Section: Int, CaseIterable {
    case summary
    case sites
    case settings
  }
  
  private enum SummaryRows: Int, CaseIterable {
    case monthlyPayment
    case nextContribution
    
    func dequeuedCell(from tableView: UITableView, indexPath: IndexPath) -> TableViewCell {
      switch self {
      case .monthlyPayment:
        return tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
      case .nextContribution:
        return tableView.dequeueReusableCell(for: indexPath) as TableViewCell
      }
    }
  }
  
  private enum SitesRows: Int, CaseIterable {
    case supportedSites
    case excludedSites
    
    func dequeuedCell(from tableView: UITableView, indexPath: IndexPath) -> TableViewCell {
      return tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
    }
  }
  
  private enum SettingsRows: Int, CaseIterable {
    case minimumLength
    case minimumVisits
    case allowUnverifiedContributions
    case allowVideoContributions
    
    func dequeuedCell(from tableView: UITableView, indexPath: IndexPath) -> TableViewCell {
      switch self {
      case .minimumLength, .minimumVisits:
        return tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
      default:
        return tableView.dequeueReusableCell(for: indexPath) as TableViewCell
      }
    }
    var accessoryType: UITableViewCell.AccessoryType {
      switch self {
      case .minimumLength, .minimumVisits:
        return .disclosureIndicator
      default:
        return .none
      }
    }
  }
  
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return CGFloat.leastNormalMagnitude
  }
  
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    return UIView()
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    
    guard let section = Section(rawValue: indexPath.section) else { return }
    switch section {
    case .summary:
      guard let row = SummaryRows(rawValue: indexPath.row) else { return }
      switch row {
      case .monthlyPayment:
        // Monthly payment
        guard let wallet = state.ledger.walletInfo else { break }
        let monthlyPayment = state.ledger.contributionAmount
        let choices = wallet.parametersChoices.map { BATValue($0.doubleValue) }
        let selectedIndex = choices.map({ $0.doubleValue }).firstIndex(of: monthlyPayment) ?? 0
        
        let controller = BATValueOptionsSelectionViewController(
          ledger: state.ledger,
          options: choices,
          isSelectionPrecise: false,
          selectedOptionIndex: selectedIndex
        ) { [weak self] selectedIndex in
          guard let self = self else { return }
          if selectedIndex < choices.count {
            self.state.ledger.contributionAmount = choices[selectedIndex].doubleValue
          }
          self.navigationController?.popViewController(animated: true)
        }
        controller.title = Strings.autoContributeMonthlyPaymentTitle
        navigationController?.pushViewController(controller, animated: true)
      default:
        break
      }
    case .sites:
      guard let row = SitesRows(rawValue: indexPath.row) else { return }
      switch row {
      case .supportedSites:
        let supportedList = AutoContributeSupportedListController(state: state)
        navigationController?.pushViewController(supportedList, animated: true)
      case .excludedSites:
        let exclusionList = AutoContributeExclusionListController(state: state)
        navigationController?.pushViewController(exclusionList, animated: true)
      }
    case .settings:
      guard let row = SettingsRows(rawValue: indexPath.row) else { return }
      switch row {
      case .minimumLength:
        let choices = BraveLedger.MinimumVisitDurationOptions.allCases.map { $0.rawValue }
        let selectedIndex = choices.firstIndex(of: state.ledger.minimumVisitDuration) ?? 0
        let controller = OptionsSelectionViewController(
          options: BraveLedger.MinimumVisitDurationOptions.allCases,
          selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
            guard let self = self else { return }
            if selectedIndex < choices.count {
              self.state.ledger.minimumVisitDuration = choices[selectedIndex]
            }
            self.navigationController?.popViewController(animated: true)
        }
        controller.title = Strings.autoContributeMinimumLength
        navigationController?.pushViewController(controller, animated: true)
      case .minimumVisits:
        let choices = BraveLedger.MinimumVisitsOptions.allCases.map { $0.rawValue }
        let selectedIndex = choices.firstIndex(of: state.ledger.minimumNumberOfVisits) ?? 0
        let controller = OptionsSelectionViewController(
          options: BraveLedger.MinimumVisitsOptions.allCases,
          selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
            guard let self = self else { return }
            if selectedIndex < choices.count {
              self.state.ledger.minimumNumberOfVisits = choices[selectedIndex]
            }
            self.navigationController?.popViewController(animated: true)
        }
        controller.title = Strings.autoContributeMinimumVisits
        navigationController?.pushViewController(controller, animated: true)
      default:
        break
      }
    }
  }
  
  func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    guard let typedSection = Section(rawValue: section) else { return 0 }
    switch typedSection {
    case .summary:
      return SummaryRows.allCases.count
    case .sites:
      return SitesRows.allCases.count
    case .settings:
      return SettingsRows.allCases.count
    }
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let section = Section(rawValue: indexPath.section) else {
      assertionFailure()
      return UITableViewCell()
    }
    switch section {
    case .summary:
      guard let row = SummaryRows(rawValue: indexPath.row) else { return UITableViewCell() }
      let cell = row.dequeuedCell(from: tableView, indexPath: indexPath)
      cell.manualSeparators = []
      cell.label.font = SettingsUX.bodyFont
      cell.label.appearanceTextColor = .black
      cell.label.numberOfLines = 0
      cell.accessoryLabel?.font = SettingsUX.bodyFont
      cell.accessoryType = .none
      cell.selectionStyle = .none
      switch row {
      case .monthlyPayment:
        cell.label.text = Strings.autoContributeMonthlyPayment
        cell.accessoryType = .disclosureIndicator
        if let dollarAmount = state.ledger.dollarStringForBATAmount(state.ledger.contributionAmount) {
          let amount = "\(state.ledger.contributionAmount) \(Strings.BAT) (\(dollarAmount))"
          cell.accessoryLabel?.text = String(format: Strings.settingsAutoContributeUpToValue, amount)
        }
        cell.selectionStyle = .default
      case .nextContribution:
        cell.label.text = Strings.autoContributeNextDate
        cell.accessoryView = nextContributionDateView
      }
      return cell
    case .sites:
      guard let row = SitesRows(rawValue: indexPath.row) else { return UITableViewCell() }
      let cell = row.dequeuedCell(from: tableView, indexPath: indexPath)
      cell.manualSeparators = []
      cell.label.font = SettingsUX.bodyFont
      cell.label.appearanceTextColor = .black
      cell.label.numberOfLines = 0
      cell.accessoryLabel?.font = UIFont.monospacedDigitSystemFont(ofSize: 14.0, weight: .semibold)
      cell.accessoryType = .disclosureIndicator
      cell.selectionStyle = .default
      switch row {
      case .supportedSites:
        cell.label.text = Strings.autoContributeSupportedSites
        cell.accessoryLabel?.text = "\(publishersCount)"
      case .excludedSites:
        cell.label.text = Strings.exclusionListTitle
        cell.accessoryLabel?.text = "\(excludedPublishersCount)"
      }
      return cell
    case .settings:
      guard let row = SettingsRows(rawValue: indexPath.row) else { return UITableViewCell() }
      let cell = row.dequeuedCell(from: tableView, indexPath: indexPath)
      cell.manualSeparators = []
      cell.label.font = SettingsUX.bodyFont
      cell.label.numberOfLines = 0
      cell.label.lineBreakMode = .byWordWrapping
      cell.accessoryLabel?.font = SettingsUX.bodyFont
      cell.accessoryType = row.accessoryType
      switch row {
      case .minimumLength:
        cell.label.text = Strings.autoContributeMinimumLengthMessage
        cell.accessoryLabel?.text = BraveLedger.MinimumVisitDurationOptions(rawValue: state.ledger.minimumVisitDuration)?.displayString
      case .minimumVisits:
        cell.label.text = Strings.autoContributeMinimumVisitsMessage
        cell.accessoryLabel?.text = BraveLedger.MinimumVisitsOptions(rawValue: state.ledger.minimumNumberOfVisits)?.displayString
      case .allowUnverifiedContributions:
        cell.label.text = Strings.autoContributeToUnverifiedSites
        cell.accessoryView = contentView.allowUnverifiedContributionsSwitch
        cell.selectionStyle = .none
      case .allowVideoContributions:
        cell.label.text = Strings.autoContributeToVideos
        cell.accessoryView = contentView.allowVideoContributionsSwitch
        cell.selectionStyle = .none
      }
      return cell
    }
  }
}

extension AutoContributeDetailViewController {
  class View: UIView {
    let tableView = UITableView(frame: .zero, style: .grouped)
    let allowUnverifiedContributionsSwitch = UISwitch().then {
      $0.onTintColor = BraveUX.braveOrange
    }
    
    let allowVideoContributionsSwitch = UISwitch().then {
      $0.onTintColor = BraveUX.braveOrange
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      tableView.backgroundView = UIView().then {
        $0.backgroundColor = SettingsUX.backgroundColor
      }
      tableView.tableHeaderView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: CGFloat.leastNormalMagnitude))
      tableView.separatorInset = .zero
      tableView.register(TableViewCell.self)
      tableView.register(Value1TableViewCell.self)
      tableView.register(EmptyTableCell.self)
      tableView.layoutMargins = UIEdgeInsets(top: 15.0, left: 15.0, bottom: 15.0, right: 15.0)
      tableView.appearanceSeparatorColor = UIColor(white: 0.85, alpha: 1.0)
      
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
