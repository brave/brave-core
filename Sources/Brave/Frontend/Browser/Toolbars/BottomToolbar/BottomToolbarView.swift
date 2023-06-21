/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import Shared
import Preferences
import Combine

class BottomToolbarView: UIView, ToolbarProtocol {
  weak var tabToolbarDelegate: ToolbarDelegate?

  let tabsButton = TabsButton()
  let forwardButton = ToolbarButton()
  let backButton = ToolbarButton()
  let shareButton = ToolbarButton()
  let addTabButton = ToolbarButton()
  let searchButton = ToolbarButton().then {
    $0.isHidden = true
  }
  let menuButton = MenuButton()
  let actionButtons: [UIButton]

  var helper: ToolbarHelper?
  private let contentView = UIStackView()
  private var cancellables: Set<AnyCancellable> = []
  let line = UIView.separatorLine

  fileprivate override init(frame: CGRect) {
    actionButtons = [backButton, forwardButton, addTabButton, searchButton, tabsButton, menuButton]
    super.init(frame: frame)
    setupAccessibility()

    backgroundColor = Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground

    addSubview(contentView)
    addSubview(line)
    helper = ToolbarHelper(toolbar: self)
    addButtons(actionButtons)
    contentView.axis = .horizontal
    contentView.distribution = .fillEqually

    addGestureRecognizer(UIPanGestureRecognizer(target: self, action: #selector(didSwipeToolbar(_:))))
    
    line.snp.makeConstraints {
      $0.bottom.equalTo(self.snp.top)
      $0.leading.trailing.equalToSuperview()
    }

    privateModeCancellable = PrivateBrowsingManager.shared
      .$isPrivateBrowsing
      .removeDuplicates()
      .sink(receiveValue: { [weak self] isPrivateBrowsing in
        self?.updateColors(isPrivateBrowsing)
      })
    
    Preferences.General.nightModeEnabled.objectWillChange
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in
        self?.updateColors(PrivateBrowsingManager.shared.isPrivateBrowsing)
      }
      .store(in: &cancellables)
    
    helper?.updateForTraitCollection(traitCollection)
  }

  private var privateModeCancellable: AnyCancellable?
  private func updateColors(_ isPrivateBrowsing: Bool) {
    if isPrivateBrowsing {
      overrideUserInterfaceStyle = .dark
      backgroundColor = .privateModeBackground
    } else {
      overrideUserInterfaceStyle = DefaultTheme(
        rawValue: Preferences.General.themeNormalMode.value)?.userInterfaceStyleOverride ?? .unspecified
      backgroundColor = Preferences.General.nightModeEnabled.value ? .nightModeBackground : .urlBarBackground
    }
  }

  private var isSearchButtonEnabled: Bool = false {
    didSet {
      addTabButton.isHidden = isSearchButtonEnabled
      searchButton.isHidden = !addTabButton.isHidden
    }
  }

  func setSearchButtonState(url: URL?) {
    if let url = url {
      isSearchButtonEnabled = InternalURL(url)?.isAboutHomeURL == true
    } else {
      isSearchButtonEnabled = false
    }
  }

  override func updateConstraints() {
    contentView.snp.makeConstraints { make in
      make.leading.trailing.top.equalTo(self)
      make.bottom.equalTo(self.safeArea.bottom)
    }
    super.updateConstraints()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    helper?.updateForTraitCollection(traitCollection)
  }

  private func setupAccessibility() {
    backButton.accessibilityIdentifier = "TabToolbar.backButton"
    forwardButton.accessibilityIdentifier = "TabToolbar.forwardButton"
    tabsButton.accessibilityIdentifier = "TabToolbar.tabsButton"
    shareButton.accessibilityIdentifier = "TabToolbar.shareButton"
    addTabButton.accessibilityIdentifier = "TabToolbar.addTabButton"
    searchButton.accessibilityIdentifier = "TabToolbar.searchButton"
    accessibilityNavigationStyle = .combined
    accessibilityLabel = Strings.tabToolbarAccessibilityLabel
  }

  required init(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func addButtons(_ buttons: [UIButton]) {
    buttons.forEach { contentView.addArrangedSubview($0) }
  }

  private var previousX: CGFloat = 0.0
  @objc private func didSwipeToolbar(_ pan: UIPanGestureRecognizer) {
    switch pan.state {
    case .began:
      let velocity = pan.velocity(in: self)
      if velocity.x > 100 {
        tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .right)
      } else if velocity.x < -100 {
        tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .left)
      }
      previousX = pan.translation(in: self).x
    case .changed:
      let point = pan.translation(in: self)
      if point.x > previousX + 50 {
        tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .right)
        previousX = point.x
      } else if point.x < previousX - 50 {
        tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .left)
        previousX = point.x
      }
    default:
      break
    }
  }
}
