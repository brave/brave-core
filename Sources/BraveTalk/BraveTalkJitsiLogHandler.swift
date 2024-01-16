// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import JitsiMeetSDK
import os.log

class BraveTalkJitsiLogHandler: JitsiMeetBaseLogHandler {
  
  override func logVerbose(_ msg: String!) {
    Logger.module.debug("[verbose]: \(msg, privacy: .public)")
  }
  
  override func logDebug(_ msg: String!) {
    Logger.module.debug("\(msg, privacy: .public)")
  }
  
  override func logInfo(_ msg: String!) {
    Logger.module.info("\(msg, privacy: .public)")
  }
  
  override func logWarn(_ msg: String!) {
    Logger.module.warning("\(msg, privacy: .public)")
  }
  
  override func logError(_ msg: String!) {
    Logger.module.error("\(msg, privacy: .public)")
  }
}
