// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import os.log

protocol LocalAdblockResourceProtocol {
  func loadLocalData(name: String, type: String, bundle: Bundle, completion: ((Data) -> Void))
}

extension LocalAdblockResourceProtocol {
  func loadLocalData(name: String, type: String, completion: ((Data) -> Void)) {
    self.loadLocalData(name: name, type: type, bundle: Bundle.module, completion: completion)
  }
  
  func loadLocalData(name: String, type: String, bundle: Bundle, completion: ((Data) -> Void)) {
    guard let path = bundle.path(forResource: name, ofType: type) else {
      Logger.module.error("Could not find local file with name: \(name) and type :\(type)")
      return
    }

    let url = URL(fileURLWithPath: path)

    do {
      let data = try Data(contentsOf: url)
      completion(data)
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }
  }
}
