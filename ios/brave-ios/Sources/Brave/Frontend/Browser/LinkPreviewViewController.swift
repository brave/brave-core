// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Shared
import UIKit
import WebKit

class LinkPreviewViewController: UIViewController {

  private let url: URL
  private weak var parentTab: Tab?
  private var currentTab: Tab?
  private weak var browserController: BrowserViewController?

  init(url: URL, for tab: Tab, browserController: BrowserViewController) {
    self.url = url
    self.parentTab = tab
    self.browserController = browserController
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }

  override func viewDidLoad() {
    guard let parentTab = parentTab,
      let tabWebView = parentTab.webView,
      let browserController
    else {
      return
    }

    currentTab = Tab(
      configuration: tabWebView.configuration,
      type: parentTab.isPrivate ? .private : .regular,
      tabGeneratorAPI: nil
    ).then {
      $0.tabDelegate = browserController
      $0.createWebview()
      $0.addPolicyDecider(browserController)
      $0.webDelegate = browserController
      $0.downloadDelegate = browserController
      $0.webView?.scrollView.layer.masksToBounds = true
    }

    guard let currentTab = currentTab,
      let webView = currentTab.webView
    else {
      return
    }

    // Add rule lists for this page
    let domain = Domain.getOrCreate(forUrl: url, persistent: !currentTab.isPrivate)

    Task(priority: .userInitiated) {
      let ruleLists = await AdBlockGroupsManager.shared.ruleLists(for: domain)
      for ruleList in ruleLists {
        webView.configuration.userContentController.add(ruleList)
      }
    }

    webView.frame = view.bounds
    view.addSubview(webView)

    webView.load(URLRequest(url: url))
  }

  deinit {
    self.currentTab?.webView?.navigationDelegate = nil
    if let browserController {
      currentTab?.removePolicyDecider(browserController)
    }
    self.currentTab = nil
  }
}
