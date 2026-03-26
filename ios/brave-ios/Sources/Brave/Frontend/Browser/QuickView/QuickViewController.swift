// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Shared
import SnapKit
import UIKit
import Web
import WebKit

class QuickViewController: UIViewController {
  private let url: URL
  private weak var parentTab: (any TabState)?
  private var currentTab: (any TabState)?
  private let toolbar = QuickViewToolbar()

  init(url: URL, for tab: some TabState) {
    self.url = url
    self.parentTab = tab
    super.init(nibName: nil, bundle: nil)
    modalPresentationStyle = .fullScreen
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

    // Setup UI layout first
    setupUI()

    currentTab.loadRequest(URLRequest(url: url))
  }

  private func setupUI() {
    guard let currentTab = currentTab else {
      return
    }
    view.backgroundColor = .braveGroupedBackground
    view.addSubview(currentTab.view)
    view.addSubview(toolbar)
    currentTab.view.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(view)
      $0.bottom.equalTo(toolbar.snp.top)
    }
    toolbar.snp.makeConstraints {
      $0.leading.trailing.equalTo(view)
      $0.bottom.equalTo(view.safeAreaLayoutGuide.snp.bottom)
      $0.height.equalTo(120)
    }

    toolbar.onClose = { [weak self] in
      self?.dismiss(animated: true)
    }

    toolbar.updateURL(url)
  }
}
