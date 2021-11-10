// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

private let log = Logger.browserLogger

/// List of Find Options used by WebKit to `Find-In-Page`
/// Typically we use `caseInsensitive`, `wrapAround`, `backwards`, `showHighlight`
private struct WKFindOptions: OptionSet {
    let rawValue: UInt
    
    /// Case insensitive search option
    static let caseInsensitive = WKFindOptions(rawValue: 0x01)
    /// Searches for words "beginning with" option
    static let atWordStarts = WKFindOptions(rawValue: 0x02)
    /// Treats a Medial Captial as the beginning of a word option
    /// "PlayStation" for example would be Play & Station as two separate words.
    static let treatMedialCapitalAsWordStart = WKFindOptions(rawValue: 0x04)
    /// Searches backwards
    static let backwards = WKFindOptions(rawValue: 0x08)
    /// When the user reaches the end of all search results, wraps around back to the beginning option.
    static let wrapAround = WKFindOptions(rawValue: 0x10)
    /// Shows an overlay (brighter) highlight on top of the matched words option
    static let showOverlay = WKFindOptions(rawValue: 0x20)
    /// Show the "selection" indicator on top of the currently matched word option
    static let showFindIndicator = WKFindOptions(rawValue: 0x40)
    /// Shows a yellow highlight on top of the matches words option
    static let showHighlight = WKFindOptions(rawValue: 0x80)
    /// Used when the index of the search has not changed at all
    static let noIndexChange = WKFindOptions(rawValue: 0x100)
    /// Forces webkit to determine the match index
    static let determineMatchIndex = WKFindOptions(rawValue: 0x200)
}

/// A Delegate used to invoke `Find-In-Page` internally in WebKit
/// This is used to allow WebKit to do Find-In-Page on PDFs and all Web-Content
@objc
class WKWebViewFindStringFindDelegate: NSObject {
    /// Set to true when the constructor hasn't failed
    /// Used to guard against a developer accidentally calling the API when they can't
    private var didConstructorSucceed = false
    
    /// Not necessary, but just in case WebKit retains the delegate in the future,
    /// it would be a good idea to restore the original state of the WebView upon destruction
    private var originalDelegate: AnyObject?
    
    /// The WebView this delegate is installed in
    private weak var webView: WKWebView?
    
    /// A callback function that is called when `find(string:...)` returns a result
    private var findMatchesCallback: ((Int, Int) -> Void)?
    
    /// List of default options for `Find-In-Page`
    /// Additionally, when searching backwards, `.backwards` is appended
    private let options: WKFindOptions = [.caseInsensitive,
                                          .showHighlight,
                                          .wrapAround,
                                          .showFindIndicator,
                                          /*.showOverlay, // Options used by Safari
                                          .atWordStarts*/]
    
    /// List of selectors used. The only ones really needed is `set` and `find`.
    /// The `get` selector is just for extra security in case something changes in the future,
    /// with regards to retaining the delegate.
    private static let getFindDelegateSelector = Selector(("_findDelegate"))
    private static let setFindDelegateSelector = Selector(("_setFindDelegate:"))
    private static let findStringSelector = Selector(("_findString:options:maxCount:"))
    
    /// Determines if the selectors are valid and this class can be used
    /// IF this returns `false` do NOT attempt to instantiate this class.
    /// It will NOT work.
    private static var canFindInPagePrivate: Bool {
        let delegates = [getFindDelegateSelector,
                         setFindDelegateSelector,
                         findStringSelector]
        return delegates.allSatisfy { WKWebView.instancesRespond(to: $0) }
    }

    /// Returns the existing Find-In-Page delegate for the specified WKWebView.
    /// If none exists, returns nil.
    /// MUST only be called after a call to `canFindInPagePrivate`
    private static func findDelegate(for webView: WKWebView) -> WKWebViewFindStringFindDelegate? {
        var originalDelegate: AnyObject?
        if let delegate = webView.perform(WKWebViewFindStringFindDelegate.getFindDelegateSelector) {
            originalDelegate = delegate.takeRetainedValue()
        }
        
        if let originalDelegate = originalDelegate as? WKWebViewFindStringFindDelegate {
            return originalDelegate
        }
        return nil
    }
    
    /// Constructs a Delegate for a specified WebView
    /// Can fail if the WebView already contains an instance of this delegate
    /// Can fail if the Internal WebKit API changes
    init?(webView: WKWebView) {
        self.webView = webView
        super.init()
        
        if !WKWebViewFindStringFindDelegate.canFindInPagePrivate {
            log.error("CANNOT INSTANTIATE WKWebViewFindStringFindDelegate!")
            return nil
        }
        
        if WKWebViewFindStringFindDelegate.findDelegate(for: webView) != nil {
            log.error("CANNOT SET FIND DELEGATE TWICE!")
            return nil
        }
        
        if let delegate = webView.perform(WKWebViewFindStringFindDelegate.getFindDelegateSelector) {
            originalDelegate = delegate.takeRetainedValue()
        }
        
        webView.perform(WKWebViewFindStringFindDelegate.setFindDelegateSelector, with: self)
        didConstructorSucceed = true
    }
    
    deinit {
        if didConstructorSucceed {
            webView?.perform(WKWebViewFindStringFindDelegate.setFindDelegateSelector, with: originalDelegate)
            originalDelegate = nil
            webView = nil
        }
    }
    
    /// Finds a string on a Page or PDF or any searchable content
    /// `string` - The string to find
    /// `backwards` - Whether to search backwards (up the page) or fowards (down the page)
    /// `completion` -  Called asynchronously when the Find Results are updated.
    ///              - First parameter is the Index of the currently highlighted item
    ///              - Second parameter is the Total results found on the page
    func find(string: String, backwards: Bool, _ completion: @escaping (Int, Int) -> Void) {
        guard didConstructorSucceed, let webView = webView else { return }
        findMatchesCallback = completion
        
        var options = self.options
        if backwards {
            options.update(with: .backwards)
        }
        
        typealias _findString = @convention(c) (NSObject, Selector, NSString, UInt, UInt) -> Void
        let selector = WKWebViewFindStringFindDelegate.findStringSelector
        let findString = unsafeBitCast(webView.method(for: selector), to: _findString.self)
        findString(webView,
                   selector,
                   string as NSString,
                   options.rawValue,
                   UInt.max - 1)
    }
    
    @objc
    private func _webView(_ webView: WKWebView, didCountMatches matches: UInt, forString string: NSString) {
        log.debug("FIND-IN-PAGE COUNT-MATCHES: \(matches)")
    }
    
    @objc
    private func _webView(_ webView: WKWebView, didFindMatches matches: UInt, forString string: NSString, withMatchIndex matchIndex: NSInteger) {
        var matches = matches
        
        // kWKMoreThanMaximumMatchCount = static_cast<unsigned>(-1)
        if matches >= UInt.max - 1 {
            matches = UInt(Int.max)
        }
        
        findMatchesCallback?(matchIndex + 1, Int(matches))
    }
    
    @objc
    private func _webView(_ webView: WKWebView, didFailToFindString string: NSString) {
        findMatchesCallback?(0, 0)
    }
}
