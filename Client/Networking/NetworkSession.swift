// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

typealias NetworkSessionDataResponse = (Data?, URLResponse?, Error?) -> Void

protocol NetworkSession {
    func dataRequest(with url: URL, completion: @escaping NetworkSessionDataResponse)
    func dataRequest(with urlRequest: URLRequest, completion: @escaping NetworkSessionDataResponse)
}

extension URLSession: NetworkSession {
    func dataRequest(with url: URL, completion: @escaping NetworkSessionDataResponse) {
        let task = dataTask(with: url) { data, response, error in
            completion(data, response, error)
        }
        
        task.resume()
    }
    
    func dataRequest(with urlRequest: URLRequest, completion: @escaping NetworkSessionDataResponse) {
        let task = dataTask(with: urlRequest) { data, response, error in
            completion(data, response, error)
        }
        
        task.resume()
    }
}
