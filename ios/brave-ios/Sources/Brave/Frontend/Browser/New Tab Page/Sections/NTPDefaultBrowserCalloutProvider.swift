// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared
import Preferences
import UIKit

class NTPDefaultBrowserCalloutProvider: NSObject, NTPObservableSectionProvider {
  var sectionDidChange: (() -> Void)?
  private var defaultCalloutView = DefaultBrowserCalloutView()
  private let isBackgroundNTPSI: Bool
  
  private typealias DefaultBrowserCalloutCell = NewTabCenteredCollectionViewCell<DefaultBrowserCalloutView>
  
  // MARK: Lifecycle
  init(isBackgroundNTPSI: Bool) {
    self.isBackgroundNTPSI = isBackgroundNTPSI
  }
  
  // MARK: UICollectionViewDelegate
  
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    shouldShowCallout() ? 1 : 0
  }
  
  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(for: indexPath) as DefaultBrowserCalloutCell
    cell.view.addTarget(self, action: #selector(openSettings), for: .touchUpInside)
    cell.view.closeHaandler = { [weak self] in
      Preferences.General.defaultBrowserCalloutDismissed.value = true
      self?.sectionDidChange?()
      
    }
    return cell
  }
  
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    
    var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
    size.height = defaultCalloutView.systemLayoutSizeFitting(size).height
    
    return size
  }
  
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
    if !shouldShowCallout() {
      return .zero
    }
    
    return UIEdgeInsets(top: 12, left: 16, bottom: 0, right: 16)
  }
  
  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(DefaultBrowserCalloutCell.self)
  }
  
  func shouldShowCallout() -> Bool {
    // Never show Default Browser Notification over an NPT SI
    if isBackgroundNTPSI {
      return false
    }
    
    let defaultBrowserDisplayCriteria = !Preferences.General.defaultBrowserCalloutDismissed.value &&
    !Preferences.Onboarding.basicOnboardingDefaultBrowserSelected.value && AppConstants.buildChannel == .release
    
    guard let appRetentionLaunchDate = Preferences.DAU.appRetentionLaunchDate.value else {
      return defaultBrowserDisplayCriteria
    }
    
    // User should not see default browser first 7 days
    // also after 14 days
    var defaultBrowserTimeConstraintCriteria = false
    
    let rightNow = Date()
    let first7DayPeriod = appRetentionLaunchDate.addingTimeInterval(7.days)
    let first14DayPeriod = appRetentionLaunchDate.addingTimeInterval(14.days)
    
    if rightNow > first7DayPeriod, rightNow < first14DayPeriod {
      defaultBrowserTimeConstraintCriteria = true
    }
    
    return defaultBrowserDisplayCriteria && defaultBrowserTimeConstraintCriteria
  }
  
  @objc func openSettings() {
    guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
      return
    }
    
    Preferences.General.defaultBrowserCalloutDismissed.value = true
    UIApplication.shared.open(settingsUrl)
  }
}

private class DefaultBrowserCalloutView: SpringButton {
  
  var closeHaandler: (() -> Void)?
  
  private let closeButton = UIButton().then {
    $0.setImage(UIImage(named: "close_tab_bar", in: .module, compatibleWith: nil)!.template, for: .normal)
    $0.tintColor = .lightGray
    $0.contentEdgeInsets = UIEdgeInsets(equalInset: 4)
    $0.accessibilityLabel = Strings.defaultBrowserCalloutCloseAccesabilityLabel
  }
  
  private let label = UILabel().then {
    $0.text = Strings.setDefaultBrowserCalloutTitle
    $0.textColor = .black
    $0.font = UIFont.systemFont(ofSize: 14.0, weight: .medium)
    $0.numberOfLines = 0
    $0.preferredMaxLayoutWidth = 280
    $0.textAlignment = .center
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    clipsToBounds = true
    layer.cornerRadius = 8
    layer.cornerCurve = .continuous
    backgroundColor = .braveSuccessBackground
    
    addSubview(label)
    addSubview(closeButton)
    
    closeButton.addTarget(self, action: #selector(closeTab), for: .touchUpInside)
    
    label.snp.makeConstraints {
      $0.top.bottom.equalToSuperview().inset(10)
      $0.leading.equalToSuperview().offset(24)
      $0.trailing.equalTo(closeButton).inset(20)
    }
    
    closeButton.snp.makeConstraints {
      $0.top.equalToSuperview().inset(2)
      $0.trailing.equalToSuperview().inset(8)
    }
  }
  
  @objc func closeTab() {
    closeHaandler?()
  }
  
}
