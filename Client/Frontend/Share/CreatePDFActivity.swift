// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

private let log = Logger.browserLogger

#if compiler(>=5.3)

/// An activity that will create a PDF of a given web page
class CreatePDFActivity: UIActivity {
    private let callback: (Data) -> Void
    private let webView: WKWebView
    
    init(webView: WKWebView, _ callback: @escaping (Data) -> Void) {
        self.webView = webView
        self.callback = callback
        super.init()
    }
    
    override var activityTitle: String? {
        Strings.createPDF
    }
    
    override var activityImage: UIImage? {
        UIImage(systemName: "doc", withConfiguration: UIImage.SymbolConfiguration(scale: .large))
    }
    
    override func perform() {
        webView.createPDF { [weak self] result in
            dispatchPrecondition(condition: .onQueue(.main))
            guard let self = self else {
                return
            }
            switch result {
            case .success(let data):
                self.callback(data)
                self.activityDidFinish(true)
            case .failure(let error):
                log.error("Failed to create PDF with error: \(error)")
                self.activityDidFinish(false)
            }
        }
    }
    
    override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
        return true
    }
}

#endif
