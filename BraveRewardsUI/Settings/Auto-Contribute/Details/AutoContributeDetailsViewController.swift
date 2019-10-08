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
  private static let pageSize = 10
  private var publishers: [PublisherInfo] = []
  private var hasMoreContent = true
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
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
    
    title = Strings.AutoContribute
    
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(tappedEditButton))
    
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    reloadData()
  }
  
  func reloadData() {
    loadPublishers(start: 0) {[weak self] publishersList in
      guard let self = self else { return }
      self.publishers = publishersList
      self.hasMoreContent = publishersList.count == AutoContributeDetailViewController.pageSize
      self.contentView.tableView.reloadData()
      if !self.contentView.tableView.isEditing {
        self.navigationItem.rightBarButtonItem?.isEnabled = !self.publishers.isEmpty
      }
    }
  }
  
  private func loadPublishers(start: Int, limit: Int = AutoContributeDetailViewController.pageSize, completion: @escaping ([PublisherInfo]) -> Void) {
    let sort = ActivityInfoFilterOrderPair().then {
      $0.propertyName = "percent"
      $0.ascending = false
    }
    let filter = ActivityInfoFilter().then {
      $0.id = ""
      $0.excluded = .filterAllExceptExcluded
      $0.percent = 1 //exclude 0% sites.
      $0.orderBy = [sort]
      $0.nonVerified = true
    }
    
    state.ledger.listActivityInfo(fromStart: UInt32(start), limit: UInt32(limit), filter: filter) { publishersList in
      completion(publishersList)
    }
  }
  
  private func totalSitesAttributedString(from total: Int) -> NSAttributedString {
    let format = String(format: Strings.TotalSites, total)
    let s = NSMutableAttributedString(string: format)
    guard let range = format.range(of: String(total)) else { return s }
    s.addAttribute(.font, value: UIFont.systemFont(ofSize: 14.0, weight: .semibold), range: NSRange(range, in: format))
    return s
  }
  
  private let headerView = TableHeaderRowView(
    columns: [
      TableHeaderRowView.Column(
        title: Strings.Site.uppercased(),
        width: .percentage(0.7)
      ),
      TableHeaderRowView.Column(
        title: Strings.Attention.uppercased(),
        width: .percentage(0.3),
        align: .right
      ),
    ],
    tintColor: BraveUX.autoContributeTintColor
  )
  
  enum SummaryRows: Int, CaseIterable {
    case settings
    case monthlyPayment
    case nextContribution
    case supportedSites
    case excludedSites
    
    func dequeuedCell(from tableView: UITableView, indexPath: IndexPath) -> TableViewCell {
      switch self {
      case .monthlyPayment, .supportedSites:
        return tableView.dequeueReusableCell(for: indexPath) as Value1TableViewCell
      case .nextContribution, .settings, .excludedSites:
        return tableView.dequeueReusableCell(for: indexPath) as TableViewCell
      }
    }
    
    static func numberOfRows(_ isExcludingSites: Bool) -> Int {
      var cases = Set<SummaryRows>(SummaryRows.allCases)
      if !isExcludingSites {
        cases.remove(.excludedSites)
      }
      return cases.count
    }
  }
  
  private var nextContributionDateView: LabelAccessoryView {
    let view = LabelAccessoryView()
    let dateFormatter = DateFormatter().then {
      $0.dateStyle = .short
      $0.timeStyle = .none
    }
    let reconcileDate = Date(timeIntervalSince1970: TimeInterval(state.ledger.autoContributeProps.reconcileStamp))
    view.label.text = dateFormatter.string(from: reconcileDate)
    view.bounds = CGRect(origin: .zero, size: view.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize))
    return view
  }
  
  // MARK: - Actions
  
  @objc private func tappedEditButton() {
    contentView.tableView.setEditing(true, animated: true)
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDoneButton))
  }
  
  @objc private func tappedDoneButton() {
    contentView.tableView.setEditing(false, animated: true)
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(tappedEditButton))
    navigationItem.rightBarButtonItem?.isEnabled = !self.publishers.isEmpty
  }
}

extension AutoContributeDetailViewController: UITableViewDataSource, UITableViewDelegate {
  private enum Section: Int, CaseIterable {
    case summary
    case contributions
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    
    switch indexPath.section {
    case Section.summary.rawValue:
      switch indexPath.row {
      case SummaryRows.settings.rawValue:
        // Settings
        let controller = AutoContributeSettingsViewController(ledger: state.ledger)
        navigationController?.pushViewController(controller, animated: true)
      case SummaryRows.monthlyPayment.rawValue:
        // Monthly payment
        guard let wallet = state.ledger.walletInfo else { break }
        let monthlyPayment = state.ledger.contributionAmount
        let choices = wallet.parametersChoices.map { $0.doubleValue }
        let selectedIndex = choices.firstIndex(of: monthlyPayment) ?? 0
        let stringChoices = choices.map { choice -> String in
          var amount = "\(choice) BAT"
          if let dollarRate = state.ledger.dollarStringForBATAmount(choice) {
            amount.append(" (\(dollarRate))")
          }
          return amount
        }
        let controller = OptionsSelectionViewController(
          options: stringChoices,
          selectedOptionIndex: selectedIndex) { [weak self] (selectedIndex) in
            guard let self = self else { return }
            if selectedIndex < choices.count {
              self.state.ledger.contributionAmount = choices[selectedIndex]
            }
            self.navigationController?.popViewController(animated: true)
        }
        controller.title = Strings.AutoContributeMonthlyPaymentTitle
        navigationController?.pushViewController(controller, animated: true)
      case SummaryRows.excludedSites.rawValue:
        let numberOfExcludedSites = state.ledger.numberOfExcludedPublishers
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alert.addAction(UIAlertAction(title: String(format: Strings.AutoContributeRestoreExcludedSites, numberOfExcludedSites), style: .default, handler: { _ in
          self.state.ledger.restoreAllExcludedPublishers()
        }))
        alert.addAction(UIAlertAction(title: Strings.Cancel, style: .cancel, handler: nil))
        present(alert, animated: true)
      default:
        break
      }
    case Section.contributions.rawValue:
      if !publishers.isEmpty, let url = URL(string: publishers[indexPath.row].url) {
        state.delegate?.loadNewTabWithURL(url)
      }
      
    default:
      break
    }
  }
  
  func numberOfSections(in tableView: UITableView) -> Int {
    return Section.allCases.count
  }
  
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    guard let typedSection = Section(rawValue: section), typedSection == .contributions else { return nil }
    return headerView
  }
  
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    guard let typedSection = Section(rawValue: section), typedSection == .contributions else { return 0.0 }
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
      let isExcludingSites = state.ledger.numberOfExcludedPublishers > 0
      return SummaryRows.numberOfRows(isExcludingSites)
    case .contributions:
      return publishers.isEmpty ? 1 : publishers.count
    }
  }
  
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    guard Section(rawValue: indexPath.section) == .contributions, !publishers.isEmpty else { return false }
    return true
  }
  
  func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
    guard Section(rawValue: indexPath.section) == .contributions, !publishers.isEmpty else { return .none }
    return .delete
  }
  
  func tableView(_ tableView: UITableView, titleForDeleteConfirmationButtonForRowAt indexPath: IndexPath) -> String? {
    return Strings.Exclude
  }
  
  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    guard Section(rawValue: indexPath.section) == .contributions else { return }
    if let publisher = publishers[safe: indexPath.row] {
      state.ledger.updatePublisherExclusionState(withId: publisher.id, state: .excluded)
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
      cell.label.textColor = .black
      cell.label.numberOfLines = 0
      cell.accessoryLabel?.textColor = Colors.grey100
      cell.accessoryLabel?.font = SettingsUX.bodyFont
      cell.accessoryType = .none
      cell.selectionStyle = .none
      switch row {
      case .settings:
        cell.label.text = Strings.Settings
        cell.imageView?.image = UIImage(frameworkResourceNamed: "settings").alwaysTemplate
        cell.imageView?.tintColor = BraveUX.autoContributeTintColor
        cell.accessoryType = .disclosureIndicator
        cell.selectionStyle = .default
      case .monthlyPayment:
        cell.label.text = Strings.AutoContributeMonthlyPayment
        cell.accessoryType = .disclosureIndicator
        if let dollarAmount = state.ledger.dollarStringForBATAmount(state.ledger.contributionAmount) {
          cell.accessoryLabel?.text = "\(state.ledger.contributionAmount) BAT (\(dollarAmount))"
        }
        cell.selectionStyle = .default
      case .nextContribution:
        cell.label.text = Strings.AutoContributeNextDate
        cell.accessoryView = nextContributionDateView
      case .supportedSites:
        cell.label.text = Strings.AutoContributeSupportedSites
        cell.accessoryLabel?.attributedText = totalSitesAttributedString(from: publishers.count)
      case .excludedSites:
        let numberOfExcludedSites = state.ledger.numberOfExcludedPublishers
        cell.label.text = String(format: Strings.AutoContributeRestoreExcludedSites, numberOfExcludedSites)
        cell.label.textColor = Colors.blurple400
        cell.selectionStyle = .default
      }
      return cell
    case .contributions:
      if publishers.isEmpty {
        let cell = tableView.dequeueReusableCell(for: indexPath) as EmptyTableCell
        cell.label.text = Strings.EmptyAutoContribution
        return cell
      }
      guard let publisher = publishers[safe: indexPath.row] else {
        assertionFailure("No Publisher found at index: \(indexPath.row)")
        return UITableViewCell()
      }
      let cell = tableView.dequeueReusableCell(for: indexPath) as AutoContributeCell
      cell.selectionStyle = .none
      
      if let url = URL(string: publisher.url) {
        state.dataSource?.retrieveFavicon(for: url, faviconURL: URL(string: publisher.faviconUrl)) { data in
          cell.siteImageView.image = data?.image ?? UIImage(frameworkResourceNamed: "defaultFavicon")
          cell.siteImageView.backgroundColor = data?.backgroundColor
        }
      }
      
      cell.verifiedStatusImageView.isHidden = publisher.status != .verified
      let provider = " \(publisher.provider.isEmpty ? "" : String(format: Strings.OnProviderText, publisher.providerDisplayString))"
      let attrName = NSMutableAttributedString(string: publisher.name).then {
        $0.append(NSMutableAttributedString(string: provider, attributes: [.font: UIFont.boldSystemFont(ofSize: 14.0),
                                                                           .foregroundColor: UIColor.gray]))
      }
      cell.siteNameLabel.attributedText = attrName
      cell.attentionAmount = CGFloat(publisher.percent)
      return cell
    }
  }
  
  func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell, forRowAt indexPath: IndexPath) {
    if indexPath.section == Section.contributions.rawValue,
      hasMoreContent && indexPath.row == publishers.count - 2 {
      
      loadPublishers(start: publishers.count) {[weak self] publisherList in
        guard let self = self else { return }
        self.publishers.append(contentsOf: publisherList)
        self.hasMoreContent = publisherList.count == AutoContributeDetailViewController.pageSize
        // TODO: Animate this update
        tableView.reloadData()
      }
    }
  }
}

extension AutoContributeDetailViewController {
  class View: UIView {
    let tableView = UITableView(frame: .zero, style: .grouped)
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      tableView.backgroundView = UIView().then {
        $0.backgroundColor = SettingsUX.backgroundColor
      }
      tableView.tableHeaderView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: CGFloat.leastNormalMagnitude))
      tableView.separatorInset = .zero
      tableView.register(AutoContributeCell.self)
      tableView.register(TableViewCell.self)
      tableView.register(Value1TableViewCell.self)
      tableView.register(EmptyTableCell.self)
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

/// Ledger Observers
extension AutoContributeDetailViewController {
  func setupLedgerObservers() {
    ledgerObserver.excludedSitesChanged = { [weak self] key, exclude -> Void in
      guard let self = self, self.isViewLoaded else {
        return
      }
      let tableView = self.contentView.tableView
      switch exclude {
      case .all:
        tableView.reloadData()
      case .excluded:
        //The delay is to ensure the db is updated. This is just a fail safe.
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3, execute: {
          self.loadPublishers(start: 0, limit: self.publishers.count, completion: { info in
            self.publishers = info
            tableView.reloadData()
          })
        })
      default:
        return
      }
    }
  }
}
