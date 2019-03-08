// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class NetworkSessionMock: NetworkSession {
    var data: Data?
    var response: URLResponse?
    var error: Error?
    
    func dataRequest(with url: URL, completion: @escaping NetworkSessionDataResponse) {
        completion(data, response, error)
    }
    
    func dataRequest(with urlRequest: URLRequest, completion: @escaping NetworkSessionDataResponse) {
        completion(data, response, error)
    }
}
