// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension HTTPCookieStorage {
    /// The default cookie accept policy in Brave
    static let defaultBraveAcceptPolicy: HTTPCookie.AcceptPolicy = .onlyFromMainDocumentDomain
    
    /// Updates the cookie accept policy on this HTTPCookieStorage to a given policy. If nil is provided then it will
    /// set the accept policy to `defaultBraveAcceptPolicy`
    func updateCookieAcceptPolicy(to policy: HTTPCookie.AcceptPolicy?) {
        cookieAcceptPolicy = policy ?? HTTPCookieStorage.defaultBraveAcceptPolicy
    }
}
