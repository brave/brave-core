/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import BraveRewards
import Network

protocol WalletContentView: AnyObject {
  var innerScrollView: UIScrollView? { get }
  var displaysRewardsSummaryButton: Bool { get }
}

class WalletViewController: UIViewController, RewardsSummaryProtocol {
  private let networkMonitor = NWPathMonitor()
  let state: RewardsState
  let ledgerObserver: LedgerObserver
  weak var currentNotification: RewardsNotification?
  private var recurringTipAmount: Double = 0.0
  private var publisher: PublisherInfo?
  
  init(state: RewardsState) {
    self.state = state
    self.ledgerObserver = LedgerObserver(ledger: state.ledger)
    super.init(nibName: nil, bundle: nil)
    
    self.state.ledger.add(self.ledgerObserver)
    setupLedgerObservers()

    if !isLocal {
      self.fetchPublisherActivity()
    }
    self.reloadPublisherDetails()
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  /// Fetches the publisher for the page's url.
  private func fetchPublisherActivity() {
    if let dataSource = state.dataSource {
      dataSource.pageHTML(for: state.tabId) { [weak self] html in
        guard let self = self else { return }
        self.state.ledger.fetchPublisherActivity(
          from: self.state.url,
          faviconURL: self.state.faviconURL,
          publisherBlob: html,
          tabId: self.state.tabId
        )
      }
    } else {
      state.ledger.fetchPublisherActivity(
        from: state.url,
        faviconURL: state.faviconURL,
        publisherBlob: nil,
        tabId: state.tabId
      )
    }
  }
  
  // MARK: -
  
  var walletView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  let rewardsSummaryView = RewardsSummaryView()
  
  override func loadView() {
    view = View(frame: UIScreen.main.bounds)
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    // Not actually visible from this controller
    title = Strings.PanelTitle
    
    if let grants = state.ledger.walletInfo?.grants, !grants.isEmpty {
      walletView.headerView.grantsButton.isHidden = false
    } else {
      walletView.headerView.grantsButton.isHidden = true
    }
    
    navigationController?.setNavigationBarHidden(true, animated: false)
    
    rewardsSummaryView.rewardsSummaryButton.addTarget(self, action: #selector(tappedRewardsSummaryButton), for: .touchUpInside)
    rewardsSummaryView.disclaimerView = disclaimerView
    
    walletView.headerView.addFundsButton.addTarget(self, action: #selector(tappedAddFunds), for: .touchUpInside)
    walletView.headerView.settingsButton.addTarget(self, action: #selector(tappedSettings), for: .touchUpInside)
    walletView.headerView.grantsButton.addTarget(self, action: #selector(tappedGrantsButton), for: .touchUpInside)

    updateWalletHeader()
    state.ledger.fetchBalance(nil)
    
    rewardsSummaryView.monthYearLabel.text = summaryPeriod
    rewardsSummaryView.rows = summaryRows
    
    reloadUIState()
    setupPublisherView(publisherSummaryView)
    view.layoutIfNeeded()
    startNotificationObserver()
    startNetworkObserver()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    if presentedViewController == nil {
      navigationController?.setNavigationBarHidden(true, animated: animated)
    }
    reloadUIState()
  }
  
  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    
    if presentedViewController == nil {
      navigationController?.setNavigationBarHidden(false, animated: animated)
    }
  }
  
  deinit {
    networkMonitor.cancel()
    NotificationCenter.default.removeObserver(self)
  }
  
  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    
    walletView.headerView.layoutIfNeeded()
    walletView.contentView?.layoutIfNeeded()
    rewardsSummaryView.layoutIfNeeded()
    
    guard let contentView = walletView.contentView else { return }
    
    var height: CGFloat = walletView.headerView.bounds.height
    if contentView.displaysRewardsSummaryButton {
      height += rewardsSummaryView.rewardsSummaryButton.bounds.height
    }
    if let scrollView = walletView.contentView?.innerScrollView {
      scrollView.contentInset = UIEdgeInsets(top: walletView.headerView.bounds.height, left: 0, bottom: 0, right: 0)
      scrollView.scrollIndicatorInsets = scrollView.contentInset
      
      height += scrollView.contentSize.height
    } else {
      height += contentView.bounds.height
    }
    
    if state.ledger.isEnabled {
      if isLocal && rewardsSummaryView.rows.isEmpty {
        // 20 = offset between header view and content view
        // FIXME: Generate this height without using 20 or manual math
        height = rewardsSummaryView.scrollView.contentSize.height +
          rewardsSummaryView.scrollView.frame.origin.y + 20 +
          walletView.headerView.bounds.height
      } else {
        height = RewardsUX.preferredPanelSize.height
      }
    }
    
    let newSize = CGSize(width: RewardsUX.preferredPanelSize.width, height: height)
    if preferredContentSize != newSize {
      preferredContentSize = newSize
    }
  }
  
  // MARK: -
  
  private lazy var publisherSummaryView = PublisherSummaryView()
  private lazy var rewardsDisabledView = RewardsDisabledView().then {
    $0.termsOfServiceLabel.onLinkedTapped = tappedDisclaimerLink
  }
  
  func setupPublisherView(_ publisherSummaryView: PublisherSummaryView) {
    publisherSummaryView.tipButton.addTarget(self, action: #selector(tappedSendTip), for: .touchUpInside)
    publisherSummaryView.monthlyTipView.addTarget(self, action: #selector(tappedMonthlyTip), for: .touchUpInside)
    
    publisherSummaryView.monthlyTipView.isHidden = true
    publisherSummaryView.monthlyTipView.batValueView.amountLabel.text = "\(Int(self.recurringTipAmount))"
    
    let publisherView = publisherSummaryView.publisherView
    
    publisherView.learnMoreTapped = { [weak self] in
      guard let self = self, let url = URL(string: DisclaimerLinks.unclaimedFundsURL) else { return }
      self.state.delegate?.loadNewTabWithURL(url)
    }
    
    walletView.rewardsSummaryView?.disclaimerView?.onLinkedTapped = { [weak self] _ in
      guard let self = self, let url = URL(string: DisclaimerLinks.unclaimedFundsURL) else { return }
      self.state.delegate?.loadNewTabWithURL(url)
    }
    
    publisherView.onCheckAgainTapped = { [weak self] in
      guard let self = self, let publisher = self.publisher else { return }
      
      publisherView.setCheckAgainIsLoading(true)
      let date = Date()
      
      self.state.ledger.refreshPublisher(withId: publisher.id, completion: { status in
        let updateStates = {
          publisherView.setCheckAgainIsLoading(false)
          publisherView.checkAgainButton.isHidden = true
          publisherView.setStatus(status)
        }

        // Create an artificial delay so user sees something is happening
        let delay = max(0, min(2.5, 2.5 - Date().timeIntervalSince(date)))
        if status == .verified {
          updateStates()
        } else {
          DispatchQueue.main.asyncAfter(deadline: .now() + delay, execute: {
            updateStates()
          })
        }
      })
    }
    
    publisherSummaryView.autoContributeChanged = { [weak self] enabled in
      guard let self = self, let publisher = self.publisher else { return }
      
      // setting publisher exclusion to .included didn't seem to work, using .default instead.
      let state: PublisherExclude = enabled ? .included : .excluded
      self.state.ledger.updatePublisherExclusionState(withId: publisher.id, state: state)
    }
  }
  
  var isLocal: Bool {
    return state.url.host == "127.0.0.1" || state.url.host == "localhost"
  }
  
  func reloadPublisherDetails() {
    reloadUIState()
    
    let publisherView = publisherSummaryView.publisherView
    let attentionView = publisherSummaryView.attentionView
    
    publisherView.setVerificationStatusHidden(isLocal)
    
    if !isLocal {
      attentionView.valueLabel.text = "0%"
      
      publisherView.checkAgainButton.isHidden = publisher != nil
      publisherView.setCheckAgainIsLoading(state.ledger.isLoadingPublisherList)
      
      state.dataSource?.retrieveFavicon(for: state.url, faviconURL: URL(string: publisher?.faviconUrl ?? "") ?? state.faviconURL, completion: { [weak self] faviconData in
        guard let data = faviconData else { return }
        
        self?.publisherSummaryView.publisherView.faviconImageView.do {
          $0.image = data.image
          $0.backgroundColor = data.backgroundColor
        }
      })
      
      guard let publisher = publisher else {
        publisherView.updatePublisherName(state.dataSource?.displayString(for: state.url) ?? "", provider: "")
        publisherView.setStatus(.notVerified)
        return
      }
      
      let provider = " \(publisher.provider.isEmpty ? "" : String(format: Strings.OnProviderText, publisher.providerDisplayString))"
      publisherView.updatePublisherName(publisher.name, provider: provider)
      
      publisherView.setStatus(publisher.status)
      publisherView.checkAgainButton.isHidden = publisher.status != .notVerified
      
      self.publisherSummaryView.setAutoContribute(enabled:
        publisher.excluded != PublisherExclude.excluded.rawValue)
      
      attentionView.valueLabel.text = "\(publisher.percent)%"
      
      self.state.ledger.listRecurringTips { [weak self] in
        guard let self = self, let recurringTip = $0.first(where: { $0.id == publisher.id && $0.rewardsCategory == .recurringTip }) else { return }
        
        guard let contributionAmount = recurringTip.contributions.first?.value else { return }
        
        self.recurringTipAmount = contributionAmount
        self.publisherSummaryView.monthlyTipView.batValueView.amountLabel.text = "\(Int(contributionAmount))"
        self.publisherSummaryView.monthlyTipView.isHidden = false
      }
    }
  }
  
  func reloadUIState() {
    if state.ledger.isEnabled {
      if isLocal {
        rewardsSummaryView.rewardsSummaryButton.isEnabled = false
        
        self.view.backgroundColor = Colors.blurple800
        walletView.contentView = rewardsSummaryView
        
        rewardsSummaryView.rewardsSummaryButton.snp.remakeConstraints {
          $0.top.leading.trailing.equalTo(self.walletView.summaryLayoutGuide)
        }
      } else {
        walletView.rewardsSummaryView = rewardsSummaryView
        walletView.contentView = publisherSummaryView
        
        publisherSummaryView.updateViewVisibility(globalAutoContributionEnabled: state.ledger.isAutoContributeEnabled)
      }
    } else {
      if rewardsDisabledView.enableRewardsButton.allTargets.count == 0 {
        rewardsDisabledView.enableRewardsButton.addTarget(self, action: #selector(tappedEnableBraveRewards), for: .touchUpInside)
      }
      walletView.contentView = rewardsDisabledView
    }
  }
  
  // MARK: - Actions
  
  @objc private func tappedGrantsButton() {
    let controller = GrantsListViewController(ledger: state.ledger)
    navigationController?.pushViewController(controller, animated: true)
  }
  
  @objc private func tappedAddFunds() {
    let controller = AddFundsViewController(state: state)
    let container = PopoverNavigationController(rootViewController: controller)
    present(container, animated: true)
  }
  
  @objc private func tappedSettings() {
    let controller = SettingsViewController(state: state)
    navigationController?.pushViewController(controller, animated: true)
  }
  
  @objc private func tappedSendTip() {
    guard let publisherInfo = publisher else { return }
    
    let controller = TippingViewController(state: self.state, publisherInfo: publisherInfo)
    self.state.delegate?.presentBraveRewardsController(controller)
  }
  
  @objc private func tappedMonthlyTip() {
    guard let publisher = publisher else { return }
    state.ledger.publisherBanner(forId: publisher.id, completion: { [weak self] banner in
      guard let self = self else { return }
      
      var options = TippingViewController.defaultTippingAmounts.map({ BATValue($0) })
      if let banner = banner, !banner.amounts.isEmpty {
        options = banner.amounts.map({ BATValue($0.doubleValue) })
      }
      
      options.insert(BATValue(0.0), at: 0)
      
      let selectedIndex = options.firstIndex(where: { Int($0.doubleValue) == Int(self.recurringTipAmount) }) ?? 0
      
      let optionsVC = BATValueOptionsSelectionViewController(ledger: self.state.ledger, options: options, selectedOptionIndex: selectedIndex, optionSelected: { [weak self] optionIndex in
        guard let self = self else { return }
        
        self.recurringTipAmount = options[optionIndex].doubleValue
        
        self.navigationController?.popToViewController(self, animated: true)
        // swiftlint:ignore:next
        self.publisherSummaryView.monthlyTipView.batValueView.amountLabel.text = options[safe: optionIndex]?.displayString ?? options[0].displayString
        
        // The user decided to remove an existing tip.
        if Int(self.recurringTipAmount) == 0 {
          self.state.ledger.removeRecurringTip(publisherId: publisher.id)
          self.publisherSummaryView.monthlyTipView.isHidden = true
        }
      })
      
      self.navigationController?.pushViewController(optionsVC, animated: true)
    })
  }
  
  @objc private func tappedEnableBraveRewards() {
    state.ledger.isEnabled = true
    reloadUIState()
  }
  
  @objc private func tappedRewardsSummaryButton() {
    let contentView = walletView.contentView
    
    let isExpanding = rewardsSummaryView.transform.ty == 0
    rewardsSummaryView.rewardsSummaryButton.slideToggleImageView.image =
      UIImage(frameworkResourceNamed: isExpanding ? "slide-down" : "slide-up")
    
    // Animating the rewards summary with a bit of a bounce
    UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 0.85, initialSpringVelocity: 0, options: [], animations: {
      if isExpanding {
        self.rewardsSummaryView.transform = CGAffineTransform(
          translationX: 0,
          y: -self.walletView.summaryLayoutGuide.layoutFrame.height + self.rewardsSummaryView.rewardsSummaryButton.bounds.height
        )
      } else {
        self.rewardsSummaryView.transform = .identity
      }
    }, completion: nil)
    
    if isExpanding {
      // Prepare animation
      rewardsSummaryView.monthYearLabel.isHidden = false
      rewardsSummaryView.monthYearLabel.alpha = 0.0
    }
    // But animate the rest without a bounce (since it doesnt make sense)
    UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [], animations: {
      if isExpanding {
        contentView?.alpha = 0.0
        self.rewardsSummaryView.monthYearLabel.alpha = 1.0
        self.view.backgroundColor = Colors.blurple800
      } else {
        contentView?.alpha = 1.0
        self.rewardsSummaryView.monthYearLabel.alpha = 0.0
        self.view.backgroundColor = .white
      }
    }) { _ in
      self.rewardsSummaryView.monthYearLabel.isHidden = !(self.rewardsSummaryView.monthYearLabel.alpha > 0.0)
      self.rewardsSummaryView.alpha = 1.0
    }
  }
  
  private func tappedDisclaimerLink(_ url: URL) {
    switch url.path {
    case "terms":
      guard let url = URL(string: DisclaimerLinks.termsOfUseURL) else { return }
      state.delegate?.loadNewTabWithURL(url)
      
    case "policy":
      guard let url = URL(string: DisclaimerLinks.policyURL) else { return }
      state.delegate?.loadNewTabWithURL(url)
      
    default:
      assertionFailure()
    }
  }
}

extension WalletViewController {
  func startNetworkObserver() {
    networkMonitor.pathUpdateHandler = {[weak self] path in
      guard let self = self else {
        return
      }
        if path.status == .satisfied {
            //hide network not available banner
            self.walletView.setNotificationView(nil, animated: true)
            self.loadNextNotification()
        } else {
            //Show network not available banner
            let notification = RewardsNotificationViewBuilder.networkUnavailableNotification
            notification.closeButton.isHidden = true
            self.walletView.setNotificationView(notification, animated: true)
        }
    }
    networkMonitor.start(queue: .main)
  }
}

extension WalletViewController {
  
  func startNotificationObserver() {
    // Stopping as a precaution
    // Add observer
    NotificationCenter.default.addObserver(self, selector: #selector(notificationAdded(_:)), name: BraveLedger.NotificationAdded, object: nil)
    loadNextNotification()
  }
  
  @objc private func notificationAdded(_ notification: Notification) {
    // TODO: Filter notification?
    // LoadNext if there is no current notification
    if currentNotification == nil {
      loadNextNotification()
    }
  }
  
  private func clearCurrentNotification() {
    if let currentNotification = currentNotification {
      state.ledger.clearNotification(currentNotification)
      self.currentNotification = nil
    }
  }
  
  private func loadNextNotification() {
    if let notification = state.ledger.notifications.first {
      currentNotification = notification
      if let notificationView = RewardsNotificationViewBuilder.get(notification: notification) {
        notificationView.closeButton.addTarget(self, action: #selector(tappedNotificationClose), for: .touchUpInside)
        (notificationView as? WalletActionNotificationView)?.actionButton.addTarget(self, action: #selector(tappedNotificationAction), for: .touchUpInside)
        walletView.setNotificationView(notificationView, animated: true)
      } else {
        clearCurrentNotification()
        loadNextNotification()
      }
    } else {
      walletView.setNotificationView(nil, animated: true)
    }
  }
  
  @objc private func tappedNotificationClose() {
    clearCurrentNotification()
    loadNextNotification()
  }
  
  @objc private func tappedNotificationAction() {
    if let notification = currentNotification {
      switch notification.kind {
      case .grant, .grantAds:
        tappedGrantsButton()
      case .adsLaunch:
        tappedSettings()
      default:
        break
      }
      tappedNotificationClose()
    }
  }
  
  func updateWalletHeader() {
    walletView.headerView.setWalletBalance(
      state.ledger.balanceString,
      crypto: "BAT",
      dollarValue: state.ledger.usdBalanceString
    )
  }
  
  func setupLedgerObservers() {
    ledgerObserver.fetchedPanelPublisher = { [weak self] publisher, tabId in
      guard let self = self, self.state.tabId == tabId else { return }
      self.publisher = publisher
      if let activity = self.state.ledger.currentActivityInfo(withPublisherId: publisher.id) {
        self.publisher?.percent = activity.percent
      }
      self.reloadPublisherDetails()
    }
    ledgerObserver.publisherListUpdated = { [weak self] in
      guard let self = self else { return }
      self.publisherSummaryView.publisherView.setCheckAgainIsLoading(self.state.ledger.isLoadingPublisherList)
    }
    ledgerObserver.fetchedBalance = { [weak self] in
      self?.updateWalletHeader()
    }
    ledgerObserver.activityRemoved = { [weak self] publisherKey in
      guard let self = self, publisherKey == self.publisher?.id else { return }
      self.fetchPublisherActivity()
    }
    ledgerObserver.balanceReportUpdated = { [weak self] in
      guard let self = self, self.isViewLoaded else {
        return
      }
      let rows = self.summaryRows.map({ row -> RowView in
        row.isHidden = true
        return row
      })
      UIView.animate(withDuration: 0.15, animations: {
        self.rewardsSummaryView.stackView.arrangedSubviews.forEach({
          $0.isHidden = true
          $0.alpha = 0
        })
      }, completion: { _ in
        self.rewardsSummaryView.rows = rows
        UIView.animate(withDuration: 0.15, animations: {
          self.rewardsSummaryView.stackView.arrangedSubviews.forEach({
            $0.isHidden = false
            $0.alpha = 1
          })
        })
      })
    }
  }
}
