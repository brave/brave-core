// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Preferences
import Shared
import SnapKit
import UIKit

class BottomToolbarView: UIView, ToolbarProtocol {
  weak var tabToolbarDelegate: ToolbarDelegate?

  let tabsButton = TabsButton()
  let forwardButton = ToolbarButton()
  let backButton = ToolbarButton()
  let shareButton = ToolbarButton()
  let addTabButton = ToolbarButton()
  let searchButton = ToolbarButton()
  let menuButton = ToolbarButton()
  let actionButtons: [UIButton]

  var helper: ToolbarHelper?
  private let contentView = UIStackView()
  private var cancellables: Set<AnyCancellable> = []
  let line = UIView.separatorLine
  private let privateBrowsingManager: PrivateBrowsingManager
  private let toolbarState: BrowserToolbarState

  private let forwardOrShareView = SharedSlotView()
  private let addOrSearchView = SharedSlotView()

  init(privateBrowsingManager: PrivateBrowsingManager, toolbarState: BrowserToolbarState) {
    self.privateBrowsingManager = privateBrowsingManager
    self.toolbarState = toolbarState
    actionButtons = [
      backButton, shareButton, forwardButton, addTabButton, searchButton, tabsButton, menuButton,
    ]
    super.init(frame: .zero)
    setupAccessibility()

    backgroundColor = privateBrowsingManager.browserColors.chromeBackground

    addSubview(contentView)
    addSubview(line)
    helper = ToolbarHelper(toolbar: self)

    contentView.addArrangedSubview(backButton)
    contentView.addArrangedSubview(forwardOrShareView)
    contentView.addArrangedSubview(addOrSearchView)
    contentView.addArrangedSubview(tabsButton)
    contentView.addArrangedSubview(menuButton)

    contentView.axis = .horizontal
    contentView.distribution = .fillEqually
    contentView.snp.makeConstraints { make in
      make.leading.trailing.top.equalTo(self.safeAreaLayoutGuide)
      make.bottom.equalTo(self.safeArea.bottom)
      make.height.equalTo(UIConstants.toolbarHeight)
    }

    line.snp.makeConstraints {
      $0.bottom.equalTo(self.snp.top)
      $0.leading.trailing.equalToSuperview()
    }

    privateModeCancellable = privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] isPrivateBrowsing in
        guard let self = self else { return }
        self.updateColors()
        self.helper?.updateForTraitCollection(
          self.traitCollection,
          browserColors: privateBrowsingManager.browserColors,
          isBottomToolbar: true
        )
      })

    helper?.updateForTraitCollection(
      traitCollection,
      browserColors: privateBrowsingManager.browserColors,
      isBottomToolbar: true
    )

    updateColors()
    configureToolbarMenus(state: toolbarState)

    if #unavailable(iOS 26.0) {
      startObservingProperties()
    }

    registerForTraitChanges([UITraitPreferredContentSizeCategory.self]) { (self: Self, _) in
      self.helper?.updateForTraitCollection(
        self.traitCollection,
        browserColors: self.privateBrowsingManager.browserColors,
        isBottomToolbar: true
      )
    }
  }

  private var privateModeCancellable: AnyCancellable?
  private func updateColors() {
    backgroundColor = privateBrowsingManager.browserColors.chromeBackground
    line.backgroundColor = privateBrowsingManager.browserColors.dividerStrong
  }

  @available(iOS 26.0, *)
  override func updateProperties() {
    super.updateProperties()
    updateObservedProperties()
  }

  func updateObservedProperties() {
    backButton.isEnabled = toolbarState.canGoBack
    shareButton.isEnabled = toolbarState.isPageSharable
    tabsButton.updateTabCount(toolbarState.tabCount)

    // The forward button replaces the share button when available
    let canGoForward = toolbarState.canGoForward
    forwardOrShareView.view = canGoForward ? forwardButton : shareButton

    // The search button replaces the add tab button on the new tab page
    let isNewTabPage = toolbarState.isNewTabPage
    addOrSearchView.view = isNewTabPage ? searchButton : addTabButton
  }

  @available(iOS, introduced: 18, obsoleted: 26, message: "Use updateProperties directly")
  private func startObservingProperties() {
    withObservationTracking {
      updateObservedProperties()
    } onChange: { [weak self] in
      DispatchQueue.main.async {
        MainActor.assumeIsolated {
          self?.startObservingProperties()
        }
      }
    }
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

  /// A slot in the toolbar that may be shared with more than one action depending on state
  private class SharedSlotView: UIView {
    var view: UIView? {
      didSet {
        oldValue?.removeFromSuperview()
        if let view {
          addSubview(view)
          view.snp.makeConstraints {
            $0.edges.equalToSuperview()
          }
        }
      }
    }
  }
}
