/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Storage
import WebKit

class PreviewViewController: UIViewController {
    
    var openURLInNewTab: ((URL) -> Void)?
    var openURLInNewPrivateTab: ((URL) -> Void)?
    var copyURL: ((URL) -> Void)?
    var shareURL: ((URL) -> Void)?
    
    private let tab: Tab
    private(set) var url: URL
    
    override var previewActionItems: [UIPreviewActionItem] {
        let openInNewTabAction = UIPreviewAction(title: Strings.OpenNewTabButtonTitle, style: .default) { _, _ in
            self.openURLInNewTab?(self.url)
        }
        
        let openInNewPrivateTabAction = UIPreviewAction(title: Strings.OpenNewPrivateTabButtonTitle, style: .default) { _, _ in
            self.openURLInNewPrivateTab?(self.url)
        }
        
        let copyAction = UIPreviewAction(title: Strings.CopyLinkActionTitle, style: .default) { _, _ in
            self.copyURL?(self.url)
        }
        
        let shareAction = UIPreviewAction(title: Strings.ShareLinkActionTitle, style: .default) { _, _ in
            self.shareURL?(self.url)
        }
        
        if tab.type.isPrivate {
            return [openInNewPrivateTabAction, copyAction, shareAction]
        }
        
        return [openInNewTabAction, openInNewPrivateTabAction, copyAction, shareAction]
    }

    init(tab: Tab, url: URL) {
        self.tab = tab
        self.url = url
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupWebView(tab.webView)
    }
    
    fileprivate func setupWebView(_ webView: BraveWebView?) {
        guard let webView = webView, !isIgnoredURL(url) else { return }
        let clonedWebView = WKWebView(frame: webView.frame, configuration: webView.configuration)
        clonedWebView.allowsLinkPreview = true
        self.view.addSubview(clonedWebView)
        
        clonedWebView.snp.makeConstraints { make in
            make.edges.equalTo(self.view)
        }
        
        clonedWebView.load(URLRequest(url: url))
    }
}
