// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Shared
import UIKit
import Web
import WebKit

class LinkPreviewViewController: UIViewController {

  private let url: URL
  private weak var parentTab: (any TabState)?
  private var currentTab: (any TabState)?
  private weak var browserController: BrowserViewController?

  init(url: URL, for tab: some TabState, browserController: BrowserViewController) {
    self.url = url
    self.parentTab = tab
    self.browserController = browserController
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }

  override func viewDidLoad() {
    guard let browserController else {
      return
    }

    let isPrivate = parentTab?.isPrivate ?? false
    let tab = TabStateFactory.create(
      with: .init(
        initialConfiguration: isPrivate
          ? TabManager.privateConfiguration : TabManager.defaultConfiguration,
        braveCore: browserController.profileController
      )
    )
    tab.miscDelegate = browserController
    tab.createWebView()
    tab.addPolicyDecider(browserController)
    tab.delegate = browserController
    tab.downloadDelegate = browserController
    tab.webViewProxy?.scrollView?.layer.masksToBounds = true
    tab.isVisible = true
    self.currentTab = tab

    guard let currentTab = currentTab else {
      return
    }

    // Add rule lists for this page
    let domain = Domain.getOrCreate(forUrl: url, persistent: !currentTab.isPrivate)

    Task(priority: .userInitiated) {
      // TODO: Use BraveShieldsUtilsIOS
      let ruleLists = await AdBlockGroupsManager.shared.ruleLists(
        isShieldsEnabled: !domain.areAllShieldsOff,
        adBlockMode: domain.globalBlockAdsAndTrackingLevel.adBlockMode
      )
      for ruleList in ruleLists {
        currentTab.configuration.userContentController.add(ruleList)
      }
    }

    currentTab.view.frame = view.bounds
    view.addSubview(currentTab.view)

    currentTab.loadRequest(URLRequest(url: url))
  }

  deinit {
    if let browserController {
      currentTab?.removePolicyDecider(browserController)
    }
    self.currentTab = nil
  }
}
