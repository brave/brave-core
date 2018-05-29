/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

@objc protocol WebsitePresentable {
    var title: String? { get }
    var url: String? { get }
}
