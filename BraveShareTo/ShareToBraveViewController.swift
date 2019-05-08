// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Social
import MobileCoreServices

class ShareToBraveViewController: SLComposeServiceViewController {
    
    // TODO: Separate scheme for debug builds, so it can be tested without need to uninstall production app.
    private func urlScheme(for url: String) -> URL? {
        return URL(string: "brave://open-url?url=\(url)")
    }
    
    override func configurationItems() -> [Any]! {
        guard let inputItems = extensionContext?.inputItems as? [NSExtensionItem] else {
            return []
        }
        
        // Reduce all input items down to a single list of item providers
        let attachments: [NSItemProvider] = inputItems
            .compactMap { $0.attachments }
            .flatMap { $0 }
        
        // Look for the first URL the host application is sharing.
        // If there isn't a URL grab the first text item
        guard let provider = attachments.first(where: { $0.isUrl }) ?? attachments.first(where: { $0.isText }) else {
            // If no item was processed. Cancel the share action to prevent the extension from locking the host application
            // due to the hidden ViewController.
            cancel()
            return []
        }
        
        provider.loadItem(of: provider.isUrl ? kUTTypeURL : kUTTypeText) { item, error in
            var urlItem: URL!
            
            // We can get urls from other apps as a kUTTypeText type, for example from Apple's mail.app.
            if let text = item as? String {
                urlItem = URL(string: text)
            } else if let url = item as? URL {
                urlItem = url
            } else {
                self.cancel()
                return
            }
            
            if let braveUrl = urlItem.absoluteString.addingPercentEncoding(withAllowedCharacters: .alphanumerics).flatMap(self.urlScheme) {
                self.handleUrl(braveUrl)
            }
        }
        
        return []
    }
    
    private func handleUrl(_ url: URL) {
        // From http://stackoverflow.com/questions/24297273/openurl-not-work-in-action-extension
        var responder = self as UIResponder?
        while let strongResponder = responder {
            let selector = sel_registerName("openURL:")
            if strongResponder.responds(to: selector) {
                strongResponder.callSelector(selector, object: url as NSURL, delay: 0)
            }
            responder = strongResponder.next
        }
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            self.cancel()
        }
    }
    
    override func viewDidAppear(_ animated: Bool) {
        // Stop keyboard from showing
        textView.resignFirstResponder()
        textView.isEditable = false
        
        super.viewDidAppear(animated)
    }
    
    override func willMove(toParent parent: UIViewController?) {
        view.alpha = 0
    }
}

extension NSItemProvider {
    var isText: Bool {
        return hasItemConformingToTypeIdentifier(String(kUTTypeText))
    }
    
    var isUrl: Bool {
        return hasItemConformingToTypeIdentifier(String(kUTTypeURL))
    }
    
    func loadItem(of type: CFString, completion: CompletionHandler?) {
        loadItem(forTypeIdentifier: String(type), options: nil, completionHandler: completion)
    }
}

extension NSObject {
    func callSelector(_ selector: Selector, object: AnyObject?, delay: TimeInterval) {
        DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
            Thread.detachNewThreadSelector(selector, toTarget: self, with: object)
        }
    }
}

