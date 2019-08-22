/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Storage
import WebKit

protocol TabPeekDelegate: class {
    func tabPeekDidAddBookmark(_ tab: Tab)
    func tabPeekRequestsPresentationOf(_ viewController: UIViewController)
    func tabPeekDidCloseTab(_ tab: Tab)
}

class TabPeekViewController: UIViewController, WKNavigationDelegate {

    weak var tab: Tab?

    fileprivate weak var delegate: TabPeekDelegate?
    fileprivate var clientPicker: UINavigationController?
    fileprivate var isBookmarked: Bool = false
    fileprivate var ignoreURL: Bool = false

    fileprivate var screenShot: UIImageView?
    fileprivate var previewAccessibilityLabel: String!

    // Preview action items.
    lazy var previewActions: [UIPreviewActionItem] = {
        var actions = [UIPreviewActionItem]()

        let urlIsTooLongToSave = self.tab?.urlIsTooLong ?? false
        if !self.ignoreURL && !urlIsTooLongToSave {
            if !self.isBookmarked {
                actions.append(UIPreviewAction(title: Strings.PreviewActionAddToBookmarksActionTitle, style: .default) { previewAction, viewController in
                    guard let tab = self.tab else { return }
                    self.delegate?.tabPeekDidAddBookmark(tab)
                    })
            }
            // only add the copy URL action if we don't already have 3 items in our list
            // as we are only allowed 4 in total and we always want to display close tab
            if actions.count < 3 {
                actions.append(UIPreviewAction(title: Strings.PreviewActionCopyURLActionTitle, style: .default) { previewAction, viewController in
                    guard let url = self.tab?.canonicalURL else { return }
                    UIPasteboard.general.url = url
                    SimpleToast().showAlertWithText(Strings.AppMenuCopyURLConfirmMessage, bottomContainer: self.view)
                })
            }
        }
        actions.append(UIPreviewAction(title: Strings.PreviewActionCloseTabActionTitle, style: .destructive) { previewAction, viewController in
            guard let tab = self.tab else { return }
            self.delegate?.tabPeekDidCloseTab(tab)
            })

        return actions
    }()

    init(tab: Tab, delegate: TabPeekDelegate?) {
        self.tab = tab
        self.delegate = delegate
        super.init(nibName: nil, bundle: nil)
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        if let webViewAccessibilityLabel = tab?.webView?.accessibilityLabel {
            previewAccessibilityLabel = String(format: Strings.PreviewFormatAccessibilityLabel, webViewAccessibilityLabel)
        }
        // if there is no screenshot, load the URL in a web page
        // otherwise just show the screenshot
        setupWebView(tab?.webView)
        guard let screenshot = tab?.screenshot else { return }
        setupWithScreenshot(screenshot)
    }

    fileprivate func setupWithScreenshot(_ screenshot: UIImage) {
        let imageView = UIImageView(image: screenshot)
        self.view.addSubview(imageView)

        imageView.snp.makeConstraints { make in
            make.edges.equalTo(self.view)
        }

        screenShot = imageView
        screenShot?.accessibilityLabel = previewAccessibilityLabel
    }

    fileprivate func setupWebView(_ webView: BraveWebView?) {
        guard let webView = webView, let url = webView.url, !isIgnoredURL(url) else { return }
        let clonedWebView = WKWebView(frame: webView.frame, configuration: webView.configuration)
        clonedWebView.allowsLinkPreview = false
        clonedWebView.accessibilityLabel = previewAccessibilityLabel
        self.view.addSubview(clonedWebView)

        clonedWebView.snp.makeConstraints { make in
            make.edges.equalTo(self.view)
        }

        clonedWebView.navigationDelegate = self

        clonedWebView.load(URLRequest(url: url))
    }

    func setState(withProfile browserProfile: BrowserProfile) {
        assert(Thread.current.isMainThread)

        guard let tab = self.tab else {
            return
        }

        guard let displayURL = tab.url?.absoluteString, !displayURL.isEmpty else {
            return
        }

        self.ignoreURL = isIgnoredURL(displayURL)
    }

    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        screenShot?.removeFromSuperview()
        screenShot = nil
    }
}
