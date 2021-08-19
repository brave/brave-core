/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import OnePasswordExtension

private let log = Logger.browserLogger

class ShareExtensionHelper: NSObject {
    
    enum ShareActivityType {
        case password
        case iBooks
        case openByCopy
        case `default`
    }
    
    fileprivate weak var selectedTab: Tab?

    fileprivate let selectedURL: URL
    fileprivate var onePasswordExtensionItem: NSExtensionItem!
    fileprivate let browserFillIdentifier = "org.appextension.fill-browser-action"

//    fileprivate func isFile(url: URL) -> Bool { url.scheme == "file" }

    init(url: URL, tab: Tab?) {
        self.selectedURL = tab?.shareURL?.displayURL ?? url
        self.selectedTab = tab
    }

    func createActivityViewController(
        items: [UIActivity] = [],
        _ completionHandler: @escaping (_ completed: Bool, _ activityType: UIActivity.ActivityType?, _ documentURL: URL? ) -> Void) -> UIActivityViewController {
        
        var activityItems = [AnyObject]()

        let printInfo = UIPrintInfo(dictionary: nil)

        let absoluteString = selectedTab?.url?.absoluteString ?? selectedURL.absoluteString
        printInfo.jobName = absoluteString
        printInfo.outputType = .general
        activityItems.append(printInfo)

        if let tab = selectedTab {
            activityItems.append(TabPrintPageRenderer(tab: tab))
        }
        
        if let title = selectedTab?.title {
            activityItems.append(TitleActivityItemProvider(title: title))
        }
        activityItems.append(self)

        let activityViewController = UIActivityViewController(activityItems: activityItems, applicationActivities: items)

        // Hide 'Add to Reading List' which currently uses Safari.
        // We would also hide View Later, if possible, but the exclusion list doesn't currently support
        // third-party activity types (rdar://19430419).
        activityViewController.excludedActivityTypes = [
            UIActivity.ActivityType.addToReadingList,
        ]
        
        // This needs to be ready by the time the share menu has been displayed and
        // activityViewController(activityViewController:, activityType:) is called,
        // which is after the user taps the button. So a million cycles away.
        findLoginExtensionItem()

        activityViewController.completionWithItemsHandler = { [weak self] activityType, completed, returnedItems, activityError in
            guard let self = self else { return }
            
            print("Share type \(self.shareActivityType(activityType.map { $0.rawValue }))")
            
            if self.shareActivityType(activityType.map { $0.rawValue }) != .iBooks {
                if !completed {
                    completionHandler(completed, activityType, nil)
                    return
                }
                // Bug 1392418 - When copying a url using the share extension there are 2 urls in the pasteboard.
                // This is a iOS 11.0 bug. Fixed in 11.2
                if UIPasteboard.general.hasURLs, let url = UIPasteboard.general.urls?.first {
                    UIPasteboard.general.urls = [url]
                }
                
                if self.shareActivityType(activityType.map { $0.rawValue }) == .password {
                    if let logins = returnedItems {
                        self.fillPasswords(logins as [AnyObject])
                    }
                }

                completionHandler(completed, activityType, nil)
            } else {
                self.writeWebPagePDFDataToURL { url, error in
                    completionHandler(completed, activityType, url)
                }
            }
        }
        
        return activityViewController
    }
    
    func writeWebPagePDFDataToURL(_ completion: @escaping (URL?, Error?) -> Void) {
        #if compiler(>=5.3)
        if #available(iOS 14.0, *), let webView = selectedTab?.webView, selectedTab?.temporaryDocument == nil {
            
            webView.createPDF { result in
                dispatchPrecondition(condition: .onQueue(.main))
                
                switch result {
                case .success(let data):
                    let validFilenameSet = CharacterSet(charactersIn: ":/")
                        .union(.newlines)
                        .union(.controlCharacters)
                        .union(.illegalCharacters)
                    let filename = webView.title?.components(separatedBy: validFilenameSet).joined()

                    let url = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent("\(filename ?? "Untitled").pdf")
                    
                    do {
                        try data.write(to: url)
                        completion(url, nil)
                    } catch {
                        completion(nil, error)
                        log.error("Failed to write PDF to disk: \(error)")
                    }
                case .failure(let error):
                    completion(nil, error)
                    log.error("Failed to write PDF to disk: \(error)")
                }
            }
        }
        #endif
    }
}

extension ShareExtensionHelper: UIActivityItemSource {
    func activityViewControllerPlaceholderItem(_ activityViewController: UIActivityViewController) -> Any {
        return selectedURL
    }
  
    func activityViewController(_ activityViewController: UIActivityViewController, itemForActivityType activityType: UIActivity.ActivityType?) -> Any? {
        let selectedURLItem = selectedURL.isReaderModeURL ? selectedURL.decodeReaderModeURL : selectedURL
        
        guard let uiActivityType = activityType else { return selectedURLItem }
                
        switch shareActivityType(uiActivityType.rawValue) {
            case .password:
                return onePasswordExtensionItem
            case .openByCopy:
                return selectedURL
            default:
                // Return the URL for the selected tab. If we are in reader view then decode
                // it so that we copy the original and not the internal localhost one.
                return selectedURLItem
        }
    }

    func activityViewController(_ activityViewController: UIActivityViewController, dataTypeIdentifierForActivityType activityType: UIActivity.ActivityType?) -> String {
        let dataType = activityType == nil ? browserFillIdentifier : kUTTypeURL as String
        
        guard let uiActivityType = activityType else { return dataType }

        switch shareActivityType(uiActivityType.rawValue) {
            case .password:
                return browserFillIdentifier
            case .openByCopy:
                return selectedURL.isFileScheme ? kUTTypeFileURL as String : kUTTypeURL as String
            default:
                // Return the URL for the selected tab. If we are in reader view then decode
                // it so that we copy the original and not the internal localhost one.
                return dataType
        }
    }
}

private extension ShareExtensionHelper {
    
    private func shareActivityType(_ activityType: String?) -> ShareActivityType {
        // A 'password' substring covers the most cases, such as pwsafe and 1Password.
        // com.agilebits.onepassword-ios.extension
        // com.app77.ios.pwsafe2.find-login-action-password-actionExtension
        // If your extension's bundle identifier does not contain "password", simply submit a pull request by adding your bundle identifier.
        let isPasswordManagerType = (activityType?.range(of: "password") != nil)
            || (activityType == "com.lastpass.ilastpass.LastPassExt")
            || (activityType == "in.sinew.Walletx.WalletxExt")
            || (activityType == "com.8bit.bitwarden.find-login-action-extension")
            || (activityType == "me.mssun.passforios.find-login-action-extension")
        
        let isOpenInIBooksActivityType = (activityType?.range(of: "OpenInIBooks") != nil)
            || (activityType == "com.apple.UIKit.activity.OpenInIBooks")
        
        let isOpenByCopy = activityType?.lowercased().range(of: "remoteopeninapplication-bycopy") != nil
        
        var shareActivityType: ShareActivityType = .default
        
        if isPasswordManagerType {
            shareActivityType = .password
        } else if isOpenInIBooksActivityType {
            shareActivityType = .iBooks
        } else if isOpenByCopy {
            shareActivityType = .openByCopy
        }
        
        return shareActivityType
    }
    
    func findLoginExtensionItem() {
        guard let selectedWebView = selectedTab?.webView else {
            return
        }
        
        // Add 1Password to share sheet
        OnePasswordExtension.shared().createExtensionItem(for: selectedWebView, completion: {(extensionItem, error) -> Void in
            if extensionItem == nil {
                log.error("Failed to create the password manager extension item: \(error.debugDescription).")
                return
            }
            
            // Set the 1Password extension item property
            self.onePasswordExtensionItem = extensionItem
        })
    }
    
    func fillPasswords(_ returnedItems: [AnyObject]) {
        guard let selectedWebView = selectedTab?.webView else {
            return
        }
        
        OnePasswordExtension.shared().fillReturnedItems(returnedItems, into: selectedWebView, completion: { (success, returnedItemsError) -> Void in
            if !success {
                log.error("Failed to fill item into webview: \(returnedItemsError ??? "nil").")
            }
        })
    }
}
