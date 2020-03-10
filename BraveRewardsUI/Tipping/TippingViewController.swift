/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import BraveShared

enum PublisherMediaType: String {
  case youtube
  case twitter
  case twitch
  
  var iconName: String {
    switch self {
    case .youtube:
      return "pub-youtube"
      
    case .twitter:
      return "pub-twitter"
      
    case .twitch:
      return "pub-twitch"
    }
  }
}

class TippingViewController: UIViewController, UIViewControllerTransitioningDelegate {
  
  let state: RewardsState
  let publisherInfo: PublisherInfo
  static let defaultTippingAmounts = [1.0, 5.0, 10.0]
  
  init(state: RewardsState, publisherInfo: PublisherInfo) {
    self.state = state
    self.publisherInfo = publisherInfo
    
    super.init(nibName: nil, bundle: nil)
    
    modalPresentationStyle = .overCurrentContext
    if #available(iOS 13.0, *) {
      isModalInPresentation = true
    }
    transitioningDelegate = self
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  var tippingView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    // Not actually visible, but good for accessibility
    title = Strings.tippingTitle
    
    tippingView.overviewView.dismissButton.addTarget(self, action: #selector(tappedDismissButton), for: .touchUpInside)
    tippingView.optionSelectionView.sendTipButton.addTarget(self, action: #selector(tappedSendTip), for: .touchUpInside)
    tippingView.optionSelectionView.optionChanged = { [weak self] option in
      guard let self = self else { return }
      if let balanceTotal = self.state.ledger.balance?.total {
        let hasEnoughBalanceForTip = option.value.doubleValue <= balanceTotal
        self.tippingView.optionSelectionView.setEnoughFundsAvailable(hasEnoughBalanceForTip)
      } else {
        self.tippingView.optionSelectionView.setEnoughFundsAvailable(false)
      }
    }
    tippingView.gesturalDismissExecuted = { [weak self] in
      self?.dismiss(animated: true)
    }
    tippingView.overviewView.disclaimerView.onLinkedTapped = { [weak self] _ in
      let url = URL(string: "https://brave.com/faq-rewards/#unclaimed-funds")!
      self?.state.delegate?.loadNewTabWithURL(url)
    }
    
    tippingView.overviewView.disclaimerView.isHidden = publisherInfo.status == .verified
    tippingView.optionSelectionView.setWalletBalance(state.ledger.balanceString, crypto: Strings.walletBalanceType)
    tippingView.optionSelectionView.options = TippingViewController.defaultTippingAmounts.map {
      TippingOption.batAmount(BATValue($0), dollarValue: state.ledger.dollarStringForBATAmount($0) ?? "")
    }
    
    loadPublisherBanner()
  }
  
  // MARK: - Networking
  private func loadPublisherBanner() {
    state.ledger.publisherBanner(forId: self.publisherInfo.id) { [weak self] banner in
      guard let self = self, let banner = banner else { return }
      
      if self.publisherInfo.provider.isEmpty {
        self.tippingView.overviewView.publisherNameLabel.text = self.publisherInfo.name
      } else {
        self.tippingView.overviewView.publisherNameLabel.text = "\(self.publisherInfo.name) \(String(format: Strings.onProviderText, self.publisherInfo.providerDisplayString))"
      }
      self.tippingView.overviewView.verifiedImageView.isHidden = self.publisherInfo.status == .notVerified
      self.tippingView.overviewView.titleLabel.text = banner.title.isEmpty ? Strings.tippingOverviewTitle : banner.title
      self.tippingView.overviewView.bodyLabel.text = banner.desc.isEmpty ? Strings.tippingOverviewBody : banner.desc
      
      self.downloadImage(url: banner.background, { image in
        if image != nil {
          self.tippingView.overviewView.headerView.image = image
          self.tippingView.overviewView.watermarkImageView.isHidden = true
          self.tippingView.overviewView.publisherNameLabel.isHidden = true
        }
      })
      
      if let dataSource = self.state.dataSource,
        let pageURL = URL(string: self.publisherInfo.url),
        let faviconURL = self.state.faviconURL {
        dataSource.retrieveFavicon(for: pageURL, faviconURL: faviconURL, completion: { faviconData in
          guard let faviconData = faviconData else { return }
          self.tippingView.overviewView.faviconImageView.image = faviconData.image
          self.tippingView.overviewView.faviconImageView.backgroundColor = faviconData.backgroundColor
        })
      }
      
      self.tippingView.overviewView.socialStackView.arrangedSubviews.forEach({ $0.removeFromSuperview() })
      
      if !banner.links.isEmpty {
        let orderedIcons: [PublisherMediaType] = [.youtube, .twitter, .twitch]
        let medias = banner.links.keys.compactMap({ PublisherMediaType(rawValue: $0) })
        
        orderedIcons.filter(medias.contains).forEach({
          self.tippingView.overviewView.socialStackView.addArrangedSubview(UIImageView(image: UIImage(frameworkResourceNamed: $0.iconName)))
        })
      }
    
      if self.state.ledger.walletContainsBraveFunds {
        // Use that balance first, therefore not showing any differently
        self.tippingView.overviewView.disclaimerView.isHidden = banner.status != .notVerified
      } else {
        if self.state.ledger.upholdWalletStatus == .notConnected {
          self.tippingView.overviewView.disclaimerView.isHidden = banner.status != .notVerified
        } else {
          self.tippingView.overviewView.disclaimerView.isHidden = banner.status == .verified
          if banner.status == .connected {
            self.tippingView.overviewView.disclaimerView.text = "\(Strings.tippingNotConnectedDisclaimer) \(Strings.disclaimerLearnMore)"
            self.tippingView.overviewView.disclaimerView.setURLInfo([Strings.disclaimerLearnMore: "learn-more"])
          }
        }
      }
      
      let bannerAmounts = banner.amounts.isEmpty ?
        TippingViewController.defaultTippingAmounts :
        banner.amounts.compactMap { $0.doubleValue }.sorted(by: <)
      self.tippingView.optionSelectionView.options = bannerAmounts.map {
        TippingOption.batAmount(BATValue($0), dollarValue: self.state.ledger.dollarStringForBATAmount($0) ?? "")
      }
    }
  }
  
  private func downloadImage(url: String, _ completion: @escaping (UIImage?) -> Void) {
    guard !url.isEmpty, let url = URL(string: url) else {
      completion(nil)
      return
    }
    
    DispatchQueue.global().async {
      if let data = try? Data(contentsOf: url), let image = UIImage(data: data) {
        DispatchQueue.main.async {
          completion(image)
        }
        return
      }
      
      DispatchQueue.main.async {
        completion(nil)
      }
    }
  }
  
  // MARK: - Actions
  
  @objc private func tappedDismissButton() {
    dismiss(animated: true)
  }
  
  @objc private func tappedSendTip() {
    if let selectedIndex = self.tippingView.optionSelectionView.selectedOptionIndex {
      let amount = self.tippingView.optionSelectionView.options[selectedIndex].value.doubleValue
      
      // Add recurring tips if isMonthly..
      if self.tippingView.optionSelectionView.isMonthly {
        self.state.ledger.addRecurringTip(publisherId: self.publisherInfo.id, amount: amount) { _ in
          // TODO: Handle started tip process
        }
      } else {
        self.state.ledger.tipPublisherDirectly(self.publisherInfo, amount: Double(amount), currency: "BAT") { _ in
          // TODO: Handle started tip process
        }
      }
      
      let displayConfirmationView = { (recurringDate: String?) in
        let provider = " \(self.publisherInfo.provider.isEmpty ? "" : String(format: Strings.onProviderText, self.publisherInfo.providerDisplayString))"
        
        self.tippingView.updateConfirmationInfo(name: "\(self.publisherInfo.name)\(provider)", tipAmount: amount, recurringDate: recurringDate)
        self.tippingView.setTippingConfirmationVisible(true, animated: true)
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
          self.dismiss(animated: true)
        }
      }
      
      if self.tippingView.optionSelectionView.isMonthly {
        let date = Date(timeIntervalSince1970: TimeInterval(self.state.ledger.autoContributeProps.reconcileStamp))
        let dateString = DateFormatter().then {
          $0.dateStyle = .short
          $0.timeStyle = .none
        }.string(from: date)
        
        displayConfirmationView(dateString)
      } else {
        displayConfirmationView(nil)
      }
    }
  }
  
  // MARK: -
  
  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    if UIDevice.current.userInterfaceIdiom == .phone {
      return .portrait
    }
    return .all
  }
  
  // MARK: - UIViewControllerTransitioningDelegate
  
  func animationController(forPresented presented: UIViewController, presenting: UIViewController, source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: tippingView, direction: .presenting)
  }
  
  func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: tippingView, direction: .dismissing)
  }
}
