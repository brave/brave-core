// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import BraveRewardsUI
import Data
import Shared
import BraveShared

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
            // Response and request log levels are ledger-specific.
            case .logDebug, .logResponse, .logRequest: log.debug(logOutput)
            case .logInfo: log.info(logOutput)
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
        let isRewardsEnabled = rewards.ledger.isEnabled
        self.topToolbar.locationView.rewardsButton.isHidden = (!isRewardsEnabled && Preferences.Rewards.hideRewardsIcon.value) || PrivateBrowsingManager.shared.isPrivateBrowsing
        let isVerifiedBadgeVisible = self.publisher?.status == .verified || self.publisher?.status == .connected
        let isLocal = self.tabManager.selectedTab?.url?.isLocal == true
        self.topToolbar.locationView.rewardsButton.isVerified = isRewardsEnabled && !isLocal && isVerifiedBadgeVisible
        self.topToolbar.locationView.rewardsButton.notificationCount = self.rewards.ledger.notifications.count
        self.topToolbar.locationView.rewardsButton.forceShowBadge = !Preferences.Rewards.panelOpened.value
    }

    func showBraveRewardsPanel() {
        Preferences.Rewards.panelOpened.value = true
        updateRewardsButtonState()
        
        if UIDevice.current.userInterfaceIdiom != .pad && UIApplication.shared.statusBarOrientation.isLandscape {
            let value = UIInterfaceOrientation.portrait.rawValue
            UIDevice.current.setValue(value, forKey: "orientation")
        }
        
        guard let tab = tabManager.selectedTab, let url = tab.webView?.url else { return }
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
        popover.popoverDidDismiss = { [weak self] _ in
            guard let self = self else { return }
            if let tabId = self.tabManager.selectedTab?.rewardsId, self.rewards.ledger.selectedTabId == 0 {
                // Show the tab currently visible
                self.rewards.ledger.selectedTabId = tabId
            }
            self.displayMyFirstAdIfAvailable()
        }
        // Hide the current tab
        rewards.ledger.selectedTabId = 0
        // Fetch new promotions
        rewards.ledger.fetchPromotions(nil)
    }
    
    func authorizeUpholdWallet(from tab: Tab, queryItems items: [String: String]) {
        rewards.ledger.authorizeExternalWallet(
            ofType: .uphold,
            queryItems: items) { result, redirectURL in
                switch result {
                case .ledgerOk:
                    // Fetch the wallet
                    self.rewards.ledger.fetchExternalWallet(forType: .uphold) { _ in
                        if let redirectURL = redirectURL {
                            // Requires verification
                            let request = URLRequest(url: redirectURL)
                            tab.loadRequest(request)
                        } else {
                            // Done
                            self.tabManager.removeTab(tab)
                            self.showBraveRewardsPanel()
                        }
                    }
                case .batNotAllowed:
                    // Uphold account doesn't support BAT...
                    let popup = AlertPopupView(
                        imageView: nil,
                        title: Strings.userWalletBATNotAllowedTitle,
                        message: Strings.userWalletBATNotAllowedMessage,
                        titleWeight: .semibold,
                        titleSize: 18.0
                    )
                    popup.addButton(title: Strings.userWalletBATNotAllowedLearnMore, type: .link, fontSize: 14.0) { () -> PopupViewDismissType in
                        if let url = URL(string: "https://uphold.com/en/brave/support") {
                            tab.loadRequest(URLRequest(url: url))
                        }
                        return .flyDown
                    }
                    popup.addButton(title: Strings.userWalletCloseButtonTitle, type: .primary, fontSize: 14.0) { () -> PopupViewDismissType in
                        return .flyDown
                    }
                    popup.showWithType(showType: .flyUp)
                default:
                    // Some other issue occured with authorization
                    let popup = AlertPopupView(
                        imageView: nil,
                        title: Strings.userWalletGenericErrorTitle,
                        message: Strings.userWalletGenericErrorMessage,
                        titleWeight: .semibold,
                        titleSize: 18.0
                    )
                    popup.addButton(title: Strings.userWalletCloseButtonTitle, type: .primary, fontSize: 14.0) { () -> PopupViewDismissType in
                        return .flyDown
                    }
                    popup.showWithType(showType: .flyUp)
                }
        }
    }
    
    @objc func resetNTPNotification() {
        Preferences.NewTabPage.brandedImageShowed.value = false
        Preferences.NewTabPage.atleastOneNTPNotificationWasShowed.value = false
    }
}

extension BrowserViewController: RewardsUIDelegate {
    func presentBraveRewardsController(_ controller: UIViewController) {
        self.presentedViewController?.dismiss(animated: true) {
            self.present(controller, animated: true)
        }
    }
    
    func loadNewTabWithURL(_ url: URL) {
        self.presentedViewController?.dismiss(animated: true)
        
        let request = URLRequest(url: url)
        let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
        tabManager.addTabAndSelect(request, isPrivate: isPrivate)
    }
}

extension Tab {
    func reportPageLoad(to rewards: BraveRewards) {
        guard let webView = webView, let url = webView.url else { return }
        if url.isLocal || PrivateBrowsingManager.shared.isPrivateBrowsing { return }
                
        var htmlBlob: String?
        var classifierText: String?
        
        let group = DispatchGroup()
        group.enter()
        webView.evaluateJavaScript("document.documentElement.outerHTML.toString()", completionHandler: { html, _ in
            htmlBlob = html as? String
            group.leave()
        })
        
        if shouldClassifyLoadsForAds {
            group.enter()
            webView.evaluateJavaScript("document.body.innerText", completionHandler: { text, _ in
                // Get the list of words in the page and join them together with a space
                // to send to the classifier
                classifierText = (text as? String)?.words.joined(separator: " ")
                group.leave()
            })
        }
        
        group.notify(queue: .main) {
            let faviconURL = URL(string: self.displayFavicon?.url ?? "")
            if faviconURL == nil {
                log.warning("No favicon found in \(self) to report to rewards panel")
            }
            rewards.reportLoadedPage(url: url, faviconUrl: faviconURL, tabId: self.rewardsId, html: htmlBlob ?? "", adsInnerText: classifierText)
        }
    }
    
    func reportPageNaviagtion(to rewards: BraveRewards) {
        rewards.reportTabNavigation(tabId: self.rewardsId)
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
