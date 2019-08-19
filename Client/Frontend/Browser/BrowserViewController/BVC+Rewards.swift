// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import BraveRewardsUI
import Data
import Shared

struct RewardsHelper {
    static func configureRewardsLogs(showFileName: Bool = true, showLine: Bool = true) {
        RewardsLogger.configure(logCallback: { logLevel, line, file, data in
            if data.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty { return }
            
            let logger = Logger.rewardsLogger
            
            var extraInfo = ""
            
            if showFileName {
                // Rewards logger gives us full file path, extracting filename from it.
                let fileName = (file as NSString).lastPathComponent
                extraInfo = showLine ? "[\(fileName).\(line)]" : "[\(fileName)]"
            }
            
            let logOutput = extraInfo.isEmpty ? data : "\(extraInfo) \(data)"
            
            switch logLevel {
            case .logDebug: logger.debug(logOutput)
            // Response and request log levels are ledger-specific.
            case .logInfo, .logResponse, .logRequest: logger.info(logOutput)
            case .logWarning: logger.warning(logOutput)
            case .logError: logger.error(logOutput)
            @unknown default:
                assertionFailure()
                logger.debug(logOutput)
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
    func showBraveRewardsPanel() {
        if UIDevice.current.userInterfaceIdiom != .pad && UIApplication.shared.statusBarOrientation.isLandscape {
            let value = UIInterfaceOrientation.portrait.rawValue
            UIDevice.current.setValue(value, forKey: "orientation")
        }
        
        guard let url = tabManager.selectedTab?.url, let rewards = rewards else { return }
        let braveRewardsPanel = RewardsPanelController(
            rewards,
            url: url,
            faviconURL: url,
            delegate: self,
            dataSource: self
        )
        
        let popover = PopoverController(contentController: braveRewardsPanel, contentSizeBehavior: .preferredContentSize)
        popover.addsConvenientDismissalMargins = false
        popover.present(from: topToolbar.locationView.rewardsButton, on: self)
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
    func retrieveFavicon(for pageURL: URL, faviconURL: URL?, completion: @escaping (FaviconData?) -> Void) {
        let favicon = UIImageView()
        DispatchQueue.main.async {
            favicon.setIconMO(nil, forURL: faviconURL ?? pageURL, completed: { color, url in
                guard let image = favicon.image else { return }
                completion(FaviconData(image: image, backgroundColor: color))
            })
        }
    }
    
    func displayString(for url: URL) -> String? {
        return url.host
    }
}
