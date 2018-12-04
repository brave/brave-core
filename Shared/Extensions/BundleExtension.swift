// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

public extension Bundle {
    public static let shared: Bundle = Bundle(identifier: "com.brave.Shared")!
    public static let braveShared: Bundle = Bundle(identifier: "com.brave.BraveShared")!
    public static let storage: Bundle = Bundle(identifier: "com.brave.Storage")!
}
