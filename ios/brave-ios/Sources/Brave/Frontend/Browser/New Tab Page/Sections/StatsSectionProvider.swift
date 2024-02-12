// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveStrings
import BraveShields
import Preferences
import BraveUI
import UIKit

class StatsSectionProvider: NSObject, NTPSectionProvider {
  private let isPrivateBrowsing: Bool
  var openPrivacyHubPressed: () -> Void
  var hidePrivacyHubPressed: () -> Void

  init(isPrivateBrowsing: Bool, openPrivacyHubPressed: @escaping () -> Void, hidePrivacyHubPressed: @escaping () -> Void) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.openPrivacyHubPressed = openPrivacyHubPressed
    self.hidePrivacyHubPressed = hidePrivacyHubPressed
  }
  
  @objc private func tappedButton(_ gestureRecognizer: UIGestureRecognizer) {
    guard let cell = gestureRecognizer.view as? BraveShieldStatsView else {
      return
    }
    
    cell.isHighlighted = true
    
    Task.delayed(bySeconds: 0.1) { @MainActor in
      cell.isHighlighted = false
      self.openPrivacyHubPressed()
    }
  }
  
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return Preferences.NewTabPage.showNewTabPrivacyHub.value ? 1 : 0
  }
  
  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(NewTabCenteredCollectionViewCell<BraveShieldStatsView>.self)
  }
  
  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(for: indexPath) as NewTabCenteredCollectionViewCell<BraveShieldStatsView>
    
    let tap = UITapGestureRecognizer(target: self, action: #selector(tappedButton(_:)))
    let longPress = UILongPressGestureRecognizer(target: self, action: #selector(tappedButton(_:)))

    cell.view.do {
      $0.isPrivateBrowsing = self.isPrivateBrowsing
      $0.addGestureRecognizer(tap)
      $0.addGestureRecognizer(longPress)
      
      $0.openPrivacyHubPressed = { [weak self] in
        self?.openPrivacyHubPressed()
      }
      
      $0.hidePrivacyHubPressed = { [weak self] in
        self?.hidePrivacyHubPressed()
      }
    }

    return cell
  }
  
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
    size.height = 110
    return size
  }
  
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
    
    return UIEdgeInsets(top: 8, left: 16, bottom: 8, right: 16)
  }
}

class BraveShieldStatsView: SpringButton {
  var openPrivacyHubPressed: (() -> Void)?
  var hidePrivacyHubPressed: (() -> Void)?

  private lazy var adsStatView: StatView = {
    let statView = StatView(frame: CGRect.zero)
    statView.title = Strings.Shields.shieldsAdAndTrackerStats.capitalized
    statView.color = .statsAdsBlockedTint
    return statView
  }()
  
  private lazy var dataSavedStatView: StatView = {
    let statView = StatView(frame: .zero)
    statView.title = Strings.Shields.dataSavedStat
    statView.color = .statsDataSavedTint
    return statView
  }()
  
  private lazy var timeStatView: StatView = {
    let statView = StatView(frame: .zero)
    statView.title = Strings.Shields.shieldsTimeStats
    statView.color = .statsTimeSavedTint
    return statView
  }()
  
  private let statsStackView = UIStackView().then {
    $0.distribution = .fillEqually
    $0.spacing = 8
  }
  
  private let topStackView = UIStackView().then {
    $0.distribution = .equalSpacing
    $0.alignment = .center
    $0.isLayoutMarginsRelativeArrangement = true
    $0.directionalLayoutMargins = .init(.init(top: 8, leading: 0, bottom: -4, trailing: 0))
  }
  
  private let contentStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 8
    $0.isLayoutMarginsRelativeArrangement = true
    $0.directionalLayoutMargins = .init(.init(top: 0, leading: 16, bottom: 16, trailing: 16))
  }
  
  private let privacyReportLabel = UILabel().then {
    let image = UIImage(named: "privacy_reports_shield", in: .module, compatibleWith: nil)!.template
    $0.textColor = .white
    $0.textAlignment = .center
    
    $0.attributedText = {
      let imageAttachment = NSTextAttachment().then {
        $0.image = image
        if let image = $0.image {
          $0.bounds = .init(x: 0, y: -3, width: image.size.width, height: image.size.height)
        }
      }
      
      var string = NSMutableAttributedString(attachment: imageAttachment)
      
      let padding = NSTextAttachment()
      padding.bounds = CGRect(width: 6, height: 0)
      
      string.append(NSAttributedString(attachment: padding))
      
      string.append(NSMutableAttributedString(
        string: Strings.PrivacyHub.privacyReportsTitle,
        attributes: [.font: UIFont.systemFont(ofSize: 14.0, weight: .medium)]
      ))
      return string
    }()
  }
  
  private let backgroundView = UIView()
  
  override init(frame: CGRect) {
    super.init(frame: .zero)
    
    statsStackView.addStackViewItems(.view(adsStatView), .view(dataSavedStatView), .view(timeStatView))
    contentStackView.addStackViewItems(.view(topStackView), .view(statsStackView))
    addSubview(contentStackView)
    
    update()
    
    contentStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
      $0.width.equalTo(640)
    }
    
    NotificationCenter.default.addObserver(self, selector: #selector(update), name: NSNotification.Name(rawValue: BraveGlobalShieldStats.didUpdateNotification), object: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  var isPrivateBrowsing: Bool = false {
    didSet {
      if backgroundView.superview != nil {
        return
      }

      if !isPrivateBrowsing && Preferences.NewTabPage.showNewTabPrivacyHub.value {
        backgroundView.backgroundColor = .init(white: 0, alpha: 0.25)
        backgroundView.layer.cornerRadius = 12
        backgroundView.layer.cornerCurve = .continuous
        backgroundView.isUserInteractionEnabled = false
        insertSubview(backgroundView, at: 0)
        backgroundView.snp.makeConstraints {
          $0.edges.equalToSuperview()
        }
        
        let settingsButton = BraveButton(type: .system).then {
          $0.setImage(UIImage(named: "privacy_reports_3dots", in: .module, compatibleWith: nil)!.template, for: .normal)
          $0.tintColor = .white
          $0.hitTestSlop = UIEdgeInsets(equalInset: -25)
        }
              
        let hidePrivacyHub = UIAction(
          title: Strings.PrivacyHub.hidePrivacyHubWidgetActionTitle,
          image: UIImage(braveSystemNamed: "leo.eye.off"),
          handler: UIAction.deferredActionHandler { [unowned self] _ in
            self.hidePrivacyHubPressed?()
          })
        
        let showPrivacyHub = UIAction(
          title: Strings.PrivacyHub.openPrivacyHubWidgetActionTitle,
          image: UIImage(named: "privacy_reports_shield", in: .module, compatibleWith: nil)?.template,
          handler: UIAction.deferredActionHandler { [unowned self] _ in
            self.openPrivacyHubPressed?()
          })

        settingsButton.menu = UIMenu(title: "", options: .displayInline, children: [hidePrivacyHub, showPrivacyHub])
        settingsButton.showsMenuAsPrimaryAction = true
        
        topStackView.addStackViewItems(.view(privacyReportLabel), .view(settingsButton))
      }
    }
  }
  
  deinit {
    NotificationCenter.default.removeObserver(self)
  }
  
  @objc private func update() {
    adsStatView.stat = (BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection).kFormattedNumber
    dataSavedStatView.stat = BraveGlobalShieldStats.shared.dataSaved
    timeStatView.stat = BraveGlobalShieldStats.shared.timeSaved
  }
}

private class StatView: UIView {
  var color: UIColor = .braveLabel {
    didSet {
      statLabel.textColor = color
    }
  }
  
  var stat: String = "" {
    didSet {
      statLabel.text = "\(stat)"
    }
  }
  
  var title: String = "" {
    didSet {
      titleLabel.text = "\(title)"
    }
  }
  
  fileprivate var statLabel: UILabel = {
    let label = UILabel()
    label.textAlignment = .center
    label.font = .systemFont(ofSize: 32, weight: UIFont.Weight.medium)
    label.minimumScaleFactor = 0.5
    label.adjustsFontSizeToFitWidth = true
    return label
  }()
  
  fileprivate var titleLabel: UILabel = {
    let label = UILabel()
    label.textColor = .white
    label.textAlignment = .center
    label.numberOfLines = 0
    label.font = UIFont.systemFont(ofSize: 10, weight: UIFont.Weight.medium)
    return label
  }()
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    let stackView = UIStackView()
    stackView.axis = .vertical
    stackView.alignment = .center
    
    stackView.addStackViewItems(.view(statLabel), .view(titleLabel))
    
    addSubview(stackView)
    
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
