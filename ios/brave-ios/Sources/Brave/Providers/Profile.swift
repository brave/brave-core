// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
// IMPORTANT!: Please take into consideration when adding new imports to
// this file that it is utilized by external components besides the core
// application (i.e. App Extensions). Introducing new dependencies here
// may have unintended negative consequences for App Extensions such as
// increased startup times which may lead to termination by the OS.
import Shared
import Storage
import os.log

/// A Profile manages access to the user's data.
public protocol Profile: AnyObject {
  var searchEngines: SearchEngines { get }
  var certStore: CertStore { get }
}

open class BrowserProfile: Profile {
  public lazy var searchEngines: SearchEngines = {
    return SearchEngines()
  }()

  public lazy var certStore: CertStore = {
    return CertStore()
  }()

  public init() {}
}
