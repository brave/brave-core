/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import Shared
import Preferences
import Combine

protocol ReaderModeBarViewDelegate: AnyObject {
  func readerModeSettingsTapped(_ view: UIView)
}

class ReaderModeBarView: UIView {
  weak var delegate: ReaderModeBarViewDelegate?

  private let readerModeButton = UIButton(type: .system).then {
    $0.setTitle(Strings.readerModeButtonTitle, for: .normal)
    $0.titleLabel?.font = .preferredFont(forTextStyle: .subheadline)
    $0.accessibilityIdentifier = "ReaderModeBarView.readerModeSettingsButton"
  }

  private let settingsButton = UIButton(type: .system).then {
    $0.setImage(UIImage(braveSystemNamed: "leo.tune"), for: .normal)
    $0.accessibilityIdentifier = "ReaderModeBarView.settingsButton"
  }
  
  private var privateBrowsingManager: PrivateBrowsingManager

  private var cancellables: Set<AnyCancellable> = []
  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    backgroundColor = browserColors.chromeBackground
    settingsButton.tintColor = browserColors.iconDefault
    readerModeButton.setTitleColor(browserColors.textPrimary, for: .normal)
  }
  
  init(privateBrowsingManager: PrivateBrowsingManager) {
    self.privateBrowsingManager = privateBrowsingManager
    
    super.init(frame: .zero)
    
    addSubview(readerModeButton)
    readerModeButton.addTarget(self, action: #selector(tappedSettingsButton), for: .touchUpInside)
    readerModeButton.snp.makeConstraints {
      $0.centerX.centerY.equalToSuperview()
    }

    let borderView = UIView.separatorLine
    addSubview(borderView)
    borderView.snp.makeConstraints {
      $0.top.equalTo(snp.bottom)
      $0.leading.trailing.equalToSuperview()
    }

    addSubview(settingsButton)
    settingsButton.addTarget(self, action: #selector(tappedSettingsButton), for: .touchUpInside)
    settingsButton.snp.makeConstraints {
      $0.trailing.equalToSuperview().inset(16)
      $0.centerY.equalToSuperview()
    }
    
    privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        self?.updateColors()
      })
      .store(in: &cancellables)
    
    updateColors()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc func tappedSettingsButton(_ sender: UIButton!) {
    delegate?.readerModeSettingsTapped(sender)
  }
}
