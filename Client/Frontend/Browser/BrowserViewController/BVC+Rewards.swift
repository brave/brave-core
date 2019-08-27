// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import BraveRewardsUI
import Data
import Shared

private let log = Logger.rewardsLogger

struct RewardsHelper {
    static func configureRewardsLogs(showFileName: Bool = true, showLine: Bool = true) {
        RewardsLogger.configure(logCallback: { logLevel, line, file, data in
            if data.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty { return }
            
            var extraInfo = ""
            
            if showFileName {
                // Rewards logger gives us full file path, extracting filename from it.
                let fileName = (file as NSString).lastPathComponent
                extraInfo = showLine ? "[\(fileName).\(line)]" : "[\(fileName)]"
            }
            
            let logOutput = extraInfo.isEmpty ? data : "\(extraInfo) \(data)"
            
            switch logLevel {
            case .logDebug: log.debug(logOutput)
            // Response and request log levels are ledger-specific.
            case .logInfo, .logResponse, .logRequest: log.info(logOutput)
            case .logWarning: log.warning(logOutput)
            case .logError: log.error(logOutput)
            @unknown default:
                assertionFailure()
                log.debug(logOutput)
            }
        }, withFlush: nil)
    }
}

// Since BraveRewardsUI is a separate framework, we have to implement Popover conformance here.
extension RewardsPanelController: PopoverContentComponent {
    var extendEdgeIntoArrow: Bool {
        return true
    }
    var isPanToDismissEnabled: Bool {
        return self.visibleViewController === self.viewControllers.first
    }
}

extension BrowserViewController {
    func updateRewardsButtonState() {
        if !isViewLoaded { return }
        self.topToolbar.locationView.rewardsButton.isVerified = self.publisher?.verified ?? false
        self.topToolbar.locationView.rewardsButton.notificationCount = self.rewards?.ledger.notifications.count ?? 0
    }

    func showBraveRewardsPanel() {
        if UIDevice.current.userInterfaceIdiom != .pad && UIApplication.shared.statusBarOrientation.isLandscape {
            let value = UIInterfaceOrientation.portrait.rawValue
            UIDevice.current.setValue(value, forKey: "orientation")
        }
        
        guard let tab = tabManager.selectedTab, let url = tab.webView?.url, let rewards = rewards else { return }
        let braveRewardsPanel = RewardsPanelController(
            rewards,
            tabId: UInt64(tab.rewardsId),
            url: url,
            faviconURL: url,
            delegate: self,
            dataSource: self
        )
        
        let popover = PopoverController(contentController: braveRewardsPanel, contentSizeBehavior: .preferredContentSize)
        popover.addsConvenientDismissalMargins = false
        popover.present(from: topToolbar.locationView.rewardsButton, on: self)
        popover.popoverDidDismiss = { _ in
            if let tabId = self.tabManager.selectedTab?.rewardsId, self.rewards?.ledger.selectedTabId == 0 {
                // Show the tab currently visible
                self.rewards?.ledger.selectedTabId = tabId
            }
        }
        // Hide the current tab
        rewards.ledger.selectedTabId = 0
    }
}

extension BrowserViewController: RewardsUIDelegate {
    func presentBraveRewardsController(_ controller: UIViewController) {
        self.presentedViewController?.dismiss(animated: true) {
            self.present(controller, animated: true)
        }
    }
    
    func loadNewTabWithURL(_ url: URL) {
        let request = URLRequest(url: url)
        let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
        tabManager.addTabAndSelect(request, isPrivate: isPrivate)
    }
}

extension BrowserViewController: RewardsDataSource {
    func displayString(for url: URL) -> String? {
        return url.host
    }
    
    func retrieveFavicon(for pageURL: URL, faviconURL: URL?, completion: @escaping (FaviconData?) -> Void) {
        let favicon = UIImageView()
        DispatchQueue.main.async {
            favicon.setIconMO(nil, forURL: faviconURL ?? pageURL, completed: { color, url in
                guard let image = favicon.image else { return }
                completion(FaviconData(image: image, backgroundColor: color))
            })
        }
    }
    
    func pageHTML(for tabId: UInt64, completionHandler: @escaping (String?) -> Void) {
        guard let tab = tabManager.tabsForCurrentMode.first(where: { $0.rewardsId == tabId }),
            let webView = tab.webView else {
                completionHandler(nil)
                return
        }
        
        let getHtmlToStringJSCall = "document.documentElement.outerHTML.toString()"
        
        DispatchQueue.main.async {
            webView.evaluateJavaScript(getHtmlToStringJSCall, completionHandler: { html, error in
                if let htmlString = html as? String {
                    completionHandler(htmlString)
                } else {
                    if let error = error {
                        log.error("Failed to get page HTML with JavaScript: \(error)")
                    }
                    completionHandler(nil)
                }
            })
        }
    }
}
