/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared

// Used by the App to navigate to different views.
// To open a URL use /open-url or to open a blank tab use /open-url with no params
enum DeepLink: Equatable {
    init?(urlString: String) {
        // Currently unused for now
        return nil
    }
}

extension URLComponents {
    // Return the first query parameter that matches
    func valueForQuery(_ param: String) -> String? {
        return self.queryItems?.first { $0.name == param }?.value
    }
}
    
// The root navigation for the Router. Look at the tests to see a complete URL
enum NavigationPath: Equatable {
    case url(webURL: URL?, isPrivate: Bool)
    case deepLink(DeepLink)
    case text(String)
    
    init?(url: URL) {
        let urlString = url.absoluteString
        if url.scheme == "http" || url.scheme == "https" {
            self = .url(webURL: url, isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing)
            return
        }
        
        guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false) else {
            return nil
        }
        
        guard let urlTypes = Bundle.main.object(forInfoDictionaryKey: "CFBundleURLTypes") as? [AnyObject],
            let urlSchemes = urlTypes.first?["CFBundleURLSchemes"] as? [String] else {
            // Something very strange has happened; org.mozilla.Client should be the zeroeth URL type.
            return nil
        }

        guard let scheme = components.scheme, urlSchemes.contains(scheme) else {
            return nil
        }
        
        if urlString.starts(with: "\(scheme)://deep-link"), let deepURL = components.valueForQuery("url"), let link = DeepLink(urlString: deepURL) {
            self = .deepLink(link)
        } else if urlString.starts(with: "\(scheme)://open-url") {
            let urlText = components.valueForQuery("url")
            let url = URIFixup.getURL(urlText ?? "") ?? urlText?.asURL
            let forcedPrivate = Preferences.Privacy.privateBrowsingOnly.value || PrivateBrowsingManager.shared.isPrivateBrowsing
            let isPrivate = Bool(components.valueForQuery("private") ?? "") ?? forcedPrivate
            self = .url(webURL: url, isPrivate: isPrivate)
        } else if urlString.starts(with: "\(scheme)://open-text") {
            let text = components.valueForQuery("text")
            self = .text(text ?? "")
        } else {
            return nil
        }
    }

    static func handle(nav: NavigationPath, with bvc: BrowserViewController) {
        switch nav {
        case .deepLink(let link): NavigationPath.handleDeepLink(link, with: bvc)
        case .url(let url, let isPrivate): NavigationPath.handleURL(url: url, isPrivate: isPrivate, with: bvc)
        case .text(let text): NavigationPath.handleText(text: text, with: bvc)
        }
    }

    private static func handleDeepLink(_ link: DeepLink, with bvc: BrowserViewController) {
        // Handle any deep links we add
    }
    
    private static func handleURL(url: URL?, isPrivate: Bool, with bvc: BrowserViewController) {
        if let newURL = url {
            bvc.switchToTabForURLOrOpen(newURL, isPrivate: isPrivate, isPrivileged: false)
        } else {
            bvc.openBlankNewTab(attemptLocationFieldFocus: true, isPrivate: isPrivate)
        }
    }

    private static func handleText(text: String, with bvc: BrowserViewController) {
        bvc.openBlankNewTab(attemptLocationFieldFocus: true, searchFor: text)
    }
}
