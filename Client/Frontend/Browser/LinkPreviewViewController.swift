// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import WebKit
import Data

class LinkPreviewViewController: UIViewController {
    
    let url: URL
    
    init(url: URL) {
        self.url = url
        super.init(nibName: nil, bundle: nil)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) { fatalError() }
    
    override func viewDidLoad() {
        let wk = WKWebView(frame: view.frame)
        
        let domain = Domain.getOrCreate(forUrl: url, persistent: !PrivateBrowsingManager.shared.isPrivateBrowsing)
        
        BlocklistName.blocklists(forDomain: domain).on.forEach {
            ContentBlockerHelper.ruleStore.lookUpContentRuleList(forIdentifier: $0.filename) { rule, _ in
                guard let rule = rule else { return }
                wk.configuration.userContentController.add(rule)
            }
        }
        
        view.addSubview(wk)
        wk.load(URLRequest(url: url))
    }
}
