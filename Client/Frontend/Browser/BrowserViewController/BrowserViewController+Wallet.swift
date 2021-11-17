// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveWallet
import struct Shared.InternalURL

extension BrowserViewController: BraveWalletDelegate {
    func openWalletURL(_ destinationURL: URL) {
        if let url = tabManager.selectedTab?.url, InternalURL.isValid(url: url) {
            select(url: destinationURL, visitType: .link)
        } else {
            _ = tabManager.addTabAndSelect(
                URLRequest(url: destinationURL),
                isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing
            )
        }
    }
}
