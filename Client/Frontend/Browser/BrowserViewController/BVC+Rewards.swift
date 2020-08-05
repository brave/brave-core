// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import BraveRewardsUI
import Data
import Shared
import BraveShared
import BraveUI
import Storage
import XCGLogger

private let log = Logger.rewardsLogger

private extension Int32 {
    var loggerLevel: XCGLogger.Level {
        switch self {
        case 0: return .error
        case 1: return .info
        case 2..<7: return .debug
        default: return .verbose
        }
    }
}

extension BrowserViewController {
    func updateRewardsButtonState() {
        if !isViewLoaded { return }
        if !BraveRewards.isAvailable {
            self.topToolbar.locationView.rewardsButton.isHidden = true
            return
        }
        let isRewardsEnabled = rewards.ledger.isEnabled
        self.topToolbar.locationView.rewardsButton.isHidden = (!isRewardsEnabled && Preferences.Rewards.hideRewardsIcon.value) || PrivateBrowsingManager.shared.isPrivateBrowsing
        let isVerifiedBadgeVisible = self.publisher?.status == .verified || self.publisher?.status == .connected
        let isLocal = self.tabManager.selectedTab?.url?.isLocal == true
        self.topToolbar.locationView.rewardsButton.isVerified = isRewardsEnabled && !isLocal && isVerifiedBadgeVisible
        self.topToolbar.locationView.rewardsButton.notificationCount = self.rewards.ledger.notifications.count
        self.topToolbar.locationView.rewardsButton.forceShowBadge = !Preferences.Rewards.panelOpened.value
    }

    func showBraveRewardsPanel(initialPage: RewardsPanelController.InitialPage = .default) {
        Preferences.Rewards.panelOpened.value = true
        updateRewardsButtonState()
        
        UIDevice.current.forcePortraitIfIphone(for: UIApplication.shared)
        
        guard let tab = tabManager.selectedTab, let url = tab.webView?.url else { return }
        let braveRewardsPanel = RewardsPanelController(
            rewards,
            tabId: UInt64(tab.rewardsId),
            url: url,
            faviconURL: url,
            delegate: self,
            dataSource: self,
            initialPage: initialPage
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
    
    // MARK: - SKUS
    
    func paymentRequested(_ request: PaymentRequest, _ completionHandler: @escaping (_ response: PaymentRequestResponse) -> Void) {
        UIDevice.current.forcePortraitIfIphone(for: UIApplication.shared)
        
        if !rewards.ledger.isEnabled {
            let enableRewards = SKUEnableRewardsViewController(
                rewards: rewards,
                termsURLTapped: { [weak self] in
                    if let url = URL(string: DisclaimerLinks.termsOfUseURL) {
                        self?.loadNewTabWithURL(url)
                    }
                }
            )
            present(enableRewards, animated: true)
            completionHandler(.cancelled)
            return
        }
        
        guard let publisher = publisher else { return }
        let controller = SKUPurchaseViewController(
            rewards: self.rewards,
            publisher: publisher,
            request: request,
            responseHandler: completionHandler,
            openBraveTermsOfSale: { [weak self] in
                if let url = URL(string: DisclaimerLinks.termsOfSaleURL) {
                    self?.loadNewTabWithURL(url)
                }
            }
        )
        present(controller, animated: true)
        
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
        
        if let tab = tabManager.getTabForURL(url) {
            tabManager.selectTab(tab)
        } else {
            let request = URLRequest(url: url)
            let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
            tabManager.addTabAndSelect(request, isPrivate: isPrivate)
        }
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
    
    func retrieveFavicon(for pageURL: URL, on imageView: UIImageView) {
        imageView.loadFavicon(for: pageURL)
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

extension BrowserViewController: BraveRewardsDelegate {
    func faviconURL(fromPageURL pageURL: URL, completion: @escaping (URL?) -> Void) {
        // Currently unused, may be removed in the future
    }
    
    func logMessage(withFilename file: String, lineNumber: Int32, verbosity: Int32, message: String) {
        if message.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty { return }
        log.logln(verbosity.loggerLevel, fileName: file, lineNumber: Int(lineNumber), closure: { message })
    }
}
