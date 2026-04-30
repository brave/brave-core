// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import BraveUI
import Data
import Shared
import SnapKit
import SwiftUI
import UIKit
import Web
import WebKit

class QuickViewController: UIViewController {
  private let url: URL
  private weak var parentTab: (any TabState)?
  private var currentTab: (any TabState)?
  private let privateBrowsingManager: PrivateBrowsingManager
  private let toolbarViewModel: QuickViewToolbarModel
  private lazy var toolbarHostingController = UIHostingController(
    rootView: QuickViewToolbarView(viewModel: toolbarViewModel)
  )
  private let onOpenInNewTab: ((URLRequest) -> Void)?

  init(
    url: URL,
    for tab: some TabState,
    privateBrowsingManager: PrivateBrowsingManager,
    onOpenInNewTab: ((URLRequest) -> Void)?
  ) {
    self.url = url
    self.parentTab = tab
    self.toolbarViewModel = QuickViewToolbarModel(url: url)
    self.privateBrowsingManager = privateBrowsingManager
    self.onOpenInNewTab = onOpenInNewTab
    super.init(nibName: nil, bundle: nil)
    modalPresentationStyle = .fullScreen

    updateViewModel()
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }

  override func viewDidLoad() {
    super.viewDidLoad()

    guard let parentTab else {
      return
    }

    var initialConfiguration: WKWebViewConfiguration?
    if !FeatureList.kUseProfileWebViewConfiguration.enabled {
      initialConfiguration =
        parentTab.isPrivate
        ? TabManager.privateConfiguration : TabManager.defaultConfiguration
    }
    let tab = TabStateFactory.create(
      with: .init(profile: parentTab.profile, initialConfiguration: initialConfiguration)
    )
    tab.createWebView()
    tab.addObserver(toolbarViewModel)
    tab.delegate = self
    let braveShieldsTabHelper: BraveShieldsTabHelper = .init(
      tab: tab,
      braveShieldsSettings: BraveShieldsSettingsServiceFactory.get(profile: tab.profile)
    )
    tab.braveShieldsHelper = braveShieldsTabHelper
    tab.addPolicyDecider(braveShieldsTabHelper)
    tab.webViewProxy?.scrollView?.layer.masksToBounds = true
    tab.isVisible = true
    self.currentTab = tab

    guard let currentTab = currentTab else {
      return
    }

    // Add rule lists for this page
    Task(priority: .userInitiated) {
      let isBraveShieldsEnabled = braveShieldsTabHelper.isBraveShieldsEnabled(for: url)
      let shieldLevel = braveShieldsTabHelper.shieldLevel(for: url, considerAllShieldsOption: true)
      let ruleLists = await AdBlockGroupsManager.shared.ruleLists(
        isBraveShieldsEnabled: isBraveShieldsEnabled,
        shieldLevel: shieldLevel
      )
      for ruleList in ruleLists {
        currentTab.configuration?.userContentController.add(ruleList)
      }
    }

    setupUI()

    currentTab.loadRequest(URLRequest(url: url))
  }

  private func updateViewModel() {
    toolbarViewModel.onActionButton = { [weak self] button in
      switch button {
      case .close:
        self?.dismiss(animated: true)
      case .back:
        guard let currentTab = self?.currentTab else { return }
        currentTab.goBack()
      case .forward:
        guard let currentTab = self?.currentTab else { return }
        currentTab.goForward()
      case .shield, .refresh, .playlist, .readerMode,
        .translate, .share, .openTab:
        break
      }
    }
    // TODO: https://github.com/brave/brave-browser/issues/53567
    toolbarViewModel.secondaryTopButton = .playlist
  }

  private func setupUI() {
    guard let currentTab = currentTab else {
      return
    }

    view.backgroundColor = privateBrowsingManager.browserColors.chromeBackground
    view.addSubview(currentTab.view)

    toolbarHostingController.view.backgroundColor = .clear
    addChild(toolbarHostingController)
    view.addSubview(toolbarHostingController.view)
    toolbarHostingController.didMove(toParent: self)

    currentTab.view.snp.makeConstraints {
      $0.top.equalTo(view.safeAreaLayoutGuide.snp.top)
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(toolbarHostingController.view.snp.top)
    }
    toolbarHostingController.view.snp.makeConstraints {
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(view.safeAreaLayoutGuide.snp.bottom)
    }
  }
}

// MARK: - TabDelegate
extension QuickViewController: TabDelegate {
  func tab(
    _ tab: some TabState,
    createNewTabWithRequest request: URLRequest,
    isUserInitiated: Bool
  ) -> (any TabState)? {
    // window.open should open in a regular tab
    dismiss(animated: true) { [weak self] in
      self?.onOpenInNewTab?(request)
    }
    return nil
  }
}
