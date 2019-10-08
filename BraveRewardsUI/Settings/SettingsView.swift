/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

extension SettingsViewController {
  class View: UIView {
    
    let rewardsToggleSection = SettingsRewardsSectionView()
    let walletSection = SettingsWalletSectionView(buttonType: .viewDetails)
    let adsSection = SettingsAdSectionView()
    let autoContributeSection = SettingsAutoContributeSectionView()
    let tipsSection = SettingsTipsSectionView()
    
    var grantsSections: [SettingsGrantSectionView] = [] {
      willSet {
        grantsSections.forEach { $0.removeFromSuperview() }
      }
      didSet {
        if let index = stackView.arrangedSubviews.firstIndex(of: walletSection) {
          grantsSections.reversed().forEach {
            stackView.insertArrangedSubview($0, at: index)
          }
        }
      }
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      backgroundColor = SettingsUX.backgroundColor
      
      addSubview(scrollView)
      scrollView.addSubview(stackView)
      stackView.addArrangedSubview(rewardsToggleSection)
      stackView.addArrangedSubview(walletSection)
      stackView.addArrangedSubview(adsSection)
      stackView.addArrangedSubview(autoContributeSection)
      stackView.addArrangedSubview(tipsSection)
      
      scrollView.snp.makeConstraints {
        $0.edges.equalTo(self)
      }
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalTo(self)
      }
      stackView.snp.makeConstraints {
        $0.edges.equalTo(self.scrollView.contentLayoutGuide).inset(10.0)
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    
    // MARK: - Private UI
    
    private let scrollView = UIScrollView().then {
      $0.alwaysBounceVertical = true
      $0.delaysContentTouches = false
    }
    
    private let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 10.0
    }
  }
}
