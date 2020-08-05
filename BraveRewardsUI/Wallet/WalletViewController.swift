/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import BraveRewards
import Network
import BraveShared
import BraveUI

protocol WalletContentView: AnyObject {
  var innerScrollView: UIScrollView? { get }
  var displaysRewardsSummaryButton: Bool { get }
}

class WalletViewController: UIViewController, RewardsSummaryProtocol {
  private let networkMonitor = NWPathMonitor()
  let state: RewardsState
  let ledgerObserver: LedgerObserver
  var currentNotification: RewardsNotification?
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
    title = Strings.panelTitle
    
    walletView.headerView.grantsButton.isHidden = state.ledger.finishedPromotions.isEmpty
    
    navigationController?.setNavigationBarHidden(true, animated: false)
    
    rewardsSummaryView.rewardsSummaryButton.addTarget(self, action: #selector(tappedRewardsSummaryButton), for: .touchUpInside)
    
    walletView.headerView.addFundsButton.addTarget(self, action: #selector(tappedAddFunds), for: .touchUpInside)
    walletView.headerView.settingsButton.addTarget(self, action: #selector(tappedSettings), for: .touchUpInside)
    walletView.headerView.grantsButton.addTarget(self, action: #selector(tappedGrantsButton), for: .touchUpInside)
    walletView.headerView.connectUserWalletButton.addTarget(self, action: #selector(tappedConnectWalletButton), for: .touchUpInside)
    walletView.headerView.verifyUserWalletButton.addTarget(self, action: #selector(tappedVerifyWalletButton), for: .touchUpInside)
    walletView.headerView.disconnectedUserWalletButton.addTarget(self, action: #selector(tappedDisconnectWalletButton), for: .touchUpInside)
    walletView.headerView.verifiedUserWalletButton.addTarget(self, action: #selector(tappedVerifiedUserWalletButton), for: .touchUpInside)
    walletView.headerView.setUserWalletStatus(.hidden) // Hidden by default
    
    updatePendingContributionsState()
    updateWalletHeader()
    updateWalletState()
    updateExternalWallet()
    
    rewardsSummaryView.monthYearLabel.text = summaryPeriod
    summaryRows { [weak self] rows in
      self?.rewardsSummaryView.rows = rows
    }
    
    reloadUIState()
    setupPublisherView(publisherSummaryView)
    view.layoutIfNeeded()
    startNetworkObserver()
    
    NotificationCenter.default.addObserver(self, selector: #selector(notificationAdded(_:)), name: BraveLedger.NotificationAdded, object: nil)
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    if presentedViewController == nil {
      navigationController?.setNavigationBarHidden(true, animated: animated)
    }
    reloadUIState()
    
    loadNextNotification()
  }
  
  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    
    if presentedViewController == nil {
      navigationController?.setNavigationBarHidden(false, animated: animated)
    }
    
    if rewardsSummaryView.transform.ty != 0 {
      tappedRewardsSummaryButton()
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
  
  func updatePendingContributionsState() {
    state.ledger.pendingContributionsTotal(completion: { amount in
      let labels = self.disclaimerLabels(for: amount)
      if labels.isEmpty {
        self.rewardsSummaryView.disclaimerView = nil
      } else {
        self.rewardsSummaryView.disclaimerView = WalletDisclaimerView().then {
          $0.labels = labels
          $0.labels.forEach { label in
            label.onLinkedTapped = { [weak self] link in
              guard let self = self, let disclaimerLink = RewardsSummaryLink(rawValue: link.absoluteString) else { return }
              switch disclaimerLink {
              case .learnMore:
                if let url = URL(string: DisclaimerLinks.unclaimedFundsURL) {
                  self.state.delegate?.loadNewTabWithURL(url)
                }
              case .showPendingContributions:
                let pending = PendingContributionListController(state: self.state)
                self.navigationController?.pushViewController(pending, animated: true)
              }
            }
          }
        }
      }
    })
  }
  
  func updateWalletState() {
    state.ledger.fetchBalance { balance in
      if balance == nil && self.state.ledger.balance == nil {
        // No balance to display: Error
        self.showServerErrorNotification()
      }
    }
    state.ledger.getRewardsParameters { properties in
      if properties == nil && self.state.ledger.rewardsParameters == nil {
        // No wallet properties to display: Error
        self.showServerErrorNotification()
      }
    }
  }
  
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
    
    publisherView.onCheckAgainTapped = { [weak self] in
      guard let self = self, let publisher = self.publisher else { return }
      
      publisherView.setCheckAgainIsLoading(true)
      let date = Date()
      
      self.state.ledger.refreshPublisher(withId: publisher.id, completion: { status in
        let updateStates = {
          publisherView.setCheckAgainIsLoading(false)
          publisherView.checkAgainButton.isHidden = true
          publisherView.setStatus(
            status,
            externalWalletStatus: self.state.ledger.upholdWalletStatus,
            hasBraveFunds: self.state.ledger.walletContainsBraveFunds
          )
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
      
      state.dataSource?.retrieveFavicon(for: state.url, on: publisherSummaryView.publisherView.faviconImageView.imageView)
      
      guard let publisher = publisher else {
        publisherView.updatePublisherName(state.dataSource?.displayString(for: state.url) ?? "", provider: "")
        publisherView.setStatus(
          .notVerified,
          externalWalletStatus: state.ledger.upholdWalletStatus,
          hasBraveFunds: state.ledger.walletContainsBraveFunds
        )
        return
      }
      
      let provider = " \(publisher.provider.isEmpty ? "" : String(format: Strings.onProviderText, publisher.providerDisplayString))"
      publisherView.updatePublisherName(publisher.name, provider: provider)
      
      publisherView.setStatus(
        publisher.status,
        externalWalletStatus: state.ledger.upholdWalletStatus,
        hasBraveFunds: state.ledger.walletContainsBraveFunds
      )
      publisherView.checkAgainButton.isHidden = publisher.status != .notVerified
      
      self.publisherSummaryView.setAutoContribute(enabled:
        publisher.excluded != PublisherExclude.excluded)
      
      attentionView.valueLabel.text = "\(publisher.percent)%"
      
      self.state.ledger.listRecurringTips { [weak self] in
        guard let self = self, let recurringTip = $0.first(where: { $0.id == publisher.id && $0.rewardsCategory == .recurringTip }) else { return }
        
        self.recurringTipAmount = recurringTip.weight
        self.publisherSummaryView.monthlyTipView.batValueView.amountLabel.text = "\(Int(recurringTip.weight))"
        self.publisherSummaryView.monthlyTipView.isHidden = false
      }
    }
  }
  
  func reloadUIState() {
    if state.ledger.isEnabled {
      if isLocal {
        rewardsSummaryView.rewardsSummaryButton.isEnabled = false
        
        self.view.backgroundColor = Colors.blurple100
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
  
  @objc private func tappedClaimGrantButton(_ sender: ActionButton) {
    guard let promotion = state.ledger.pendingPromotions.first else { return }
    sender.loaderView = LoaderView(size: .small)
    sender.loaderPlacement = .replacesContent
    sender.isLoading = true
    sender.isEnabled = false
    state.ledger.claimPromotion(promotion) { success in
      sender.isEnabled = true
      sender.isLoading = false
      if !success {
        let alert = UIAlertController(title: Strings.genericErrorTitle, message: Strings.genericErrorBody, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.ok, style: .default, handler: nil))
        self.present(alert, animated: true)
        return
      }
      let grantAmount = BATValue(promotion.approximateValue).displayString
      let isAdGrant = promotion.type == .ads
      let claimedVC = GrantClaimedViewController(
        grantAmount: grantAmount,
        kind: isAdGrant ? .ads : .ugp(expirationDate: Date(timeIntervalSince1970: TimeInterval(promotion.expiresAt)))
      )
      self.present(claimedVC, animated: true) {
        self.tappedNotificationClose()
      }
    }
  }
  
  @objc private func tappedGrantsButton() {
    let controller = GrantsListViewController(ledger: state.ledger)
    navigationController?.pushViewController(controller, animated: true)
  }
  
  @objc private func tappedAddFunds() {
    guard let wallet = state.ledger.externalWallets[.uphold], let url = URL(string: wallet.addUrl) else { return }
    state.delegate?.loadNewTabWithURL(url)
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
      
      optionsVC.title = Strings.recurringTipTitle
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
        self.view.backgroundColor = Colors.blurple100
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
  
  @objc private func tappedConnectWalletButton() {
    let verifyOnboarding = VerifyUserWalletViewController {
      self.dismiss(animated: true, completion: {
        guard let wallet = self.state.ledger.externalWallets[.uphold],
            let percentEncoded = wallet.verifyUrl.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed),
            let upholdURL = URL(string: percentEncoded) else { return }
          self.state.delegate?.loadNewTabWithURL(upholdURL)
      })
    }
    self.present(verifyOnboarding, animated: true)
  }
  
  private func showUserWalletDetails() {
    guard let wallet = state.ledger.externalWallets[.uphold] else { return }
    let details = UserWalletDetailsViewController(state: state, wallet: wallet) {
      self.state.ledger.disconnectWallet(ofType: .uphold) { result in
        if result == .ledgerOk {
          // Disconnected
          self.updateExternalWallet()
          self.navigationController?.popViewController(animated: true)
        }
      }
    }
    self.navigationController?.pushViewController(details, animated: true)
  }
  
  @objc private func tappedVerifyWalletButton() {
    showUserWalletDetails()
  }
  
  @objc private func tappedDisconnectWalletButton() {
    dismiss(animated: true, completion: {
      guard let wallet = self.state.ledger.externalWallets[.uphold],
        let percentEncoded = wallet.verifyUrl.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed),
        let upholdURL = URL(string: percentEncoded) else { return }
      self.state.delegate?.loadNewTabWithURL(upholdURL)
    })
  }
  
  @objc private func tappedVerifiedUserWalletButton() {
    showUserWalletDetails()
  }
}

extension WalletViewController {
  func showServerErrorNotification() {
    let notification = RewardsNotificationViewBuilder.networkUnavailableNotification
    notification.closeButton.isHidden = true
    self.walletView.setNotificationView(notification, animated: true)
  }
  
  func startNetworkObserver() {
    networkMonitor.pathUpdateHandler = {[weak self] path in
      guard let self = self else {
        return
      }
      if path.status == .satisfied {
        if self.state.ledger.balance != nil &&
          self.state.ledger.rewardsParameters != nil {
          //hide network not available banner
          self.walletView.setNotificationView(nil, animated: true)
          self.loadNextNotification()
        }
      } else {
        //Show network not available banner
        self.showServerErrorNotification()
      }
    }
    networkMonitor.start(queue: .main)
  }
}

extension WalletViewController {
  
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
    if state.ledger.balance == nil || state.ledger.rewardsParameters == nil {
      // Showing error
      return
    }
    
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
  
  @objc private func tappedNotificationAction(_ sender: ActionButton) {
    if let notification = currentNotification {
      switch notification.kind {
      case .grant, .grantAds:
        tappedClaimGrantButton(sender)
      case .adsLaunch:
        tappedSettings()
        tappedNotificationClose()
      default:
        tappedNotificationClose()
      }
    }
  }
  
  func updateWalletHeader() {
    walletView.headerView.setWalletBalance(
      state.ledger.balanceString,
      crypto: Strings.walletBalanceType,
      dollarValue: state.ledger.usdBalanceString
    )
    #if NO_USER_WALLETS
    if let publisher = publisher {
      publisherSummaryView.publisherView.setStatus(
        publisher.status,
        externalWalletStatus: .notConnected,
        hasBraveFunds: false
      )
    }
    #else
    if let publisher = publisher {
      publisherSummaryView.publisherView.setStatus(
        publisher.status,
        externalWalletStatus: self.state.ledger.upholdWalletStatus,
        hasBraveFunds: self.state.ledger.walletContainsBraveFunds
      )
    }
    if let wallet = state.ledger.externalWallets[.uphold] {
      switch wallet.status {
      case .notConnected:
        self.walletView.headerView.setUserWalletStatus(.notConnected)
      case .connected,
           .pending:
        self.walletView.headerView.setUserWalletStatus(.connectedNotVerified)
      case .disconnectedVerified,
           .disconnectedNotVerified:
        self.walletView.headerView.setUserWalletStatus(.disconnected)
      case .verified:
        self.walletView.headerView.setUserWalletStatus(.verified)
      @unknown default:
        break
      }
      self.walletView.headerView.addFundsButton.isHidden = wallet.status != .verified
    }
    #endif
  }
  
  /// Fetch an updated external wallet from ledger if the user isn't in JP
  func updateExternalWallet() {
    #if !NO_USER_WALLETS
    if Preferences.Rewards.isUsingBAP.value == true { return }
    
    // If we can show Uphold, grab verification status of the wallet
    state.ledger.fetchExternalWallet(forType: .uphold) { _ in
      self.updateWalletHeader()
    }
    #endif
  }
  
  func setupLedgerObservers() {
    ledgerObserver.fetchedPanelPublisher = { [weak self] publisher, tabId in
      guard let self = self, self.state.tabId == tabId else { return }
      self.publisher = publisher
      self.reloadPublisherDetails()
    }
    ledgerObserver.finishedPromotionsAdded = { [weak self] promotions in
      self?.walletView.headerView.grantsButton.isHidden = promotions.isEmpty
    }
    ledgerObserver.publisherListUpdated = { [weak self] in
      guard let self = self else { return }
      self.publisherSummaryView.publisherView.setCheckAgainIsLoading(self.state.ledger.isLoadingPublisherList)
      self.fetchPublisherActivity()
    }
    ledgerObserver.fetchedBalance = { [weak self] in
      self?.updateWalletHeader()
    }
    ledgerObserver.activityRemoved = { [weak self] publisherKey in
      guard let self = self, publisherKey == self.publisher?.id else { return }
      self.fetchPublisherActivity()
    }
    ledgerObserver.externalWalletAuthorized = { [weak self] _ in
      self?.updateWalletHeader()
    }
    ledgerObserver.balanceReportUpdated = { [weak self] in
      guard let self = self, self.isViewLoaded else {
        return
      }
      self.summaryRows { [weak self] rows in
        guard let self = self else { return }
        rows.forEach {
          $0.isHidden = true
        }
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
    ledgerObserver.pendingContributionAdded = { [weak self] in
      self?.updatePendingContributionsState()
    }
    ledgerObserver.pendingContributionsRemoved = { [weak self] _ in
      self?.updatePendingContributionsState()
    }
  }
}
