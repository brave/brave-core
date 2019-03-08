// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

protocol LocalAdblockResourceProtocol {
    func loadLocalData(name: String, type: String, completion: ((Data) -> Void))
}

extension LocalAdblockResourceProtocol {
    func loadLocalData(name: String, type: String, completion: ((Data) -> Void)) {
        guard let path = Bundle.main.path(forResource: name, ofType: type) else {
            log.error("Could not find local file with name: \(name) and type :\(type)")
            return
        }
        
        let url = URL(fileURLWithPath: path)
        
        do {
            let data = try Data(contentsOf: url)
            completion(data)
        } catch {
            log.error(error)
        }
    }
}
