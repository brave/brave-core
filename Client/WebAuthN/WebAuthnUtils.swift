// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

extension String {
    func websafeBase64String() -> String? {
        guard let base64Data = self.data(using: .utf8) else {
            return nil
        }
        return (base64Data as NSData).ykf_websafeBase64EncodedString()
    }
}

extension Data {
    func websafeBase64String() -> String? {
        return (self as NSData).ykf_websafeBase64EncodedString()
    }
}
