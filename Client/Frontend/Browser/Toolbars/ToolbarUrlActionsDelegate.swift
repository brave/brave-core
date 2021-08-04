// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Storage

protocol ToolbarUrlActionsDelegate: AnyObject {
    func openInNewTab(_ url: URL, isPrivate: Bool)
    func copy(_ url: URL)
    func share(_ url: URL)
    func batchOpen(_ urls: [URL])
    func select(url: URL, visitType: VisitType)
}
