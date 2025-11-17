// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import Foundation
import PhotosUI
import UIKit
import Web

extension BrowserViewController {
  func handleAIChatWebUIPageAction(_ tab: any TabState, action: AIChatWebUIPageAction) {
    switch action {
    case .handleVoiceRecognitionRequest(let completion):
      completion(nil)
    case .handleFileUploadRequest(_, let completion):
      completion(nil)
    case .presentSettings:
      break
    case .presentPremiumPaywall:
      break
    case .presentManagePremium:
      break
    case .closeTab:
      break
    case .openURL:
      break
    }
  }
}
