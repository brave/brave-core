/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import Shared

class SettingsViewController: UIViewController {
  
  var settingsView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  let state: RewardsState
  let ledgerObserver: LedgerObserver
  
  init(state: RewardsState) {
    self.state = state
    self.ledgerObserver = LedgerObserver(ledger: state.ledger)
    super.init(nibName: nil, bundle: nil)
    self.state.ledger.add(self.ledgerObserver)
    setupLedgerObservers()
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
    
    title = Strings.settingsTitle
    
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
    
    preferredContentSize = CGSize(width: RewardsUX.preferredPanelSize.width, height: 750)
    
    state.ads.updateAdRewards(false)
    
    settingsView.do {
      $0.rewardsToggleSection.toggleSwitch.addTarget(self, action: #selector(rewardsSwitchValueChanged), for: .valueChanged)
      $0.adsSection.viewDetailsButton.addTarget(self, action: #selector(tappedAdsViewDetails), for: .touchUpInside)
      $0.adsSection.toggleSwitch.addTarget(self, action: #selector(adsToggleValueChanged), for: .valueChanged)
      $0.monthlyTipsSection.viewDetailsButton.addTarget(self, action: #selector(tappedMonthlyTipsViewDetails), for: .touchUpInside)
      $0.tipsSection.viewDetailsButton.addTarget(self, action: #selector(tappedTipsViewDetails), for: .touchUpInside)
      $0.autoContributeSection.viewDetailsButton.addTarget(self, action: #selector(tappedAutoContributeViewDetails), for: .touchUpInside)
      $0.autoContributeSection.toggleSwitch.addTarget(self, action: #selector(autoContributeToggleValueChanged), for: .valueChanged)
      
      if !BraveAds.isCurrentLocaleSupported() {
         $0.adsSection.status = .unsupportedRegion
      }
      $0.adsSection.toggleSwitch.isOn = state.ads.isEnabled
      $0.rewardsToggleSection.toggleSwitch.isOn = state.ledger.isEnabled
      $0.autoContributeSection.toggleSwitch.isOn = state.ledger.isAutoContributeEnabled
    }
    
    updateVisualStateOfSections(animated: false)
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    // Not sure why this has to be set on the nav controller specifically instead of just this controller
    preferredContentSize = CGSize(width: RewardsUX.preferredPanelSize.width, height: 1000)
    
    updateGrantsSection()
  }
  
  func updateGrantsSection() {
    // Reset pending promos
    settingsView.grantsSections = state.ledger.pendingPromotions.map {
      let section = SettingsGrantSectionView(promotion: $0)
      section.claimGrantTapped = { [weak self] section in
        guard let self = self else { return }
        self.tappedClaimButton(section)
      }
      return section
    }
  }
  
  // MARK: -
  
  private func updateVisualStateOfSections(animated: Bool) {
    let ledger = state.ledger
    settingsView.do {
      $0.rewardsToggleSection.setRewardsEnabled(ledger.isEnabled, animated: animated)
      $0.adsSection.setSectionEnabled(
        ledger.isEnabled && state.ads.isEnabled,
        hidesToggle: !ledger.isEnabled,
        animated: animated
      )
      $0.autoContributeSection.setSectionEnabled(
        ledger.isEnabled && ledger.isAutoContributeEnabled,
        hidesToggle: !ledger.isEnabled,
        animated: animated
      )
      $0.monthlyTipsSection.setSectionEnabled(ledger.isEnabled, animated: animated)
      $0.tipsSection.setSectionEnabled(ledger.isEnabled, animated: animated)
    }
  }
  
  // MARK: - Actions
  
  @objc private func tappedDone() {
    dismiss(animated: true)
  }
  
  @objc private func tappedAdsViewDetails() {
    let controller = AdsDetailsViewController(state: state)
    controller.preferredContentSize = preferredContentSize
    navigationController?.pushViewController(controller, animated: true)
  }
  
  @objc private func tappedMonthlyTipsViewDetails() {
    let controller = MonthlyTipsDetailViewController(state: state)
    controller.preferredContentSize = preferredContentSize
    navigationController?.pushViewController(controller, animated: true)
  }
  
  @objc private func tappedTipsViewDetails() {
    let controller = TipsDetailViewController(state: state)
    controller.preferredContentSize = preferredContentSize
    navigationController?.pushViewController(controller, animated: true)
  }
  
  @objc private func tappedAutoContributeViewDetails() {
    let controller = AutoContributeDetailViewController(state: state)
    controller.preferredContentSize = preferredContentSize
    navigationController?.pushViewController(controller, animated: true)
  }
  
  @objc private func rewardsSwitchValueChanged() {
    state.ledger.isEnabled = settingsView.rewardsToggleSection.toggleSwitch.isOn
    updateVisualStateOfSections(animated: true)
    NotificationCenter.default.post(name: .adsOrRewardsToggledInSettings, object: nil)
  }
  
  @objc private func adsToggleValueChanged() {
    state.ads.isEnabled = settingsView.adsSection.toggleSwitch.isOn
    updateVisualStateOfSections(animated: true)
    NotificationCenter.default.post(name: .adsOrRewardsToggledInSettings, object: nil)
  }
  
  @objc private func autoContributeToggleValueChanged() {
    state.ledger.isAutoContributeEnabled = settingsView.autoContributeSection.toggleSwitch.isOn
    updateVisualStateOfSections(animated: true)
  }
  
  func setupLedgerObservers() {
    ledgerObserver.promotionsAdded = { [weak self] _ in
      self?.updateGrantsSection()
    }
  }
  
  private func tappedClaimButton(_ section: SettingsGrantSectionView) {
    let promotion = section.promotion
    
    section.claimGrantButton.isLoading = true
    
    state.ledger.claimPromotion(promotion) { [weak self] success in
      guard let self = self else { return }
      section.claimGrantButton.isLoading = false
      if !success {
        let alert = UIAlertController(title: Strings.genericErrorTitle, message: Strings.genericErrorBody, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.ok, style: .default, handler: nil))
        self.present(alert, animated: true)
        return
      }
      
      let amount = BATValue(promotion.approximateValue).displayString
      let isAdGrant = promotion.type == .ads
      
      let claimedVC = GrantClaimedViewController(
        grantAmount: amount,
        kind: isAdGrant ? .ads : .ugp(expirationDate: Date(timeIntervalSince1970: TimeInterval(promotion.expiresAt)))
      )
      self.present(claimedVC, animated: true)
    }
  }
}
