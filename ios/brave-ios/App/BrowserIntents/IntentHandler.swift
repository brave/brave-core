// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Intents
import BrowserIntentsModels
import BraveWidgetsModels

class IntentHandler: INExtension {

  override func handler(for intent: INIntent) -> Any {
    if intent is OpenWebsiteIntent {
      return OpenWebsiteIntentHandler()
    }

    if intent is OpenHistoryWebsiteIntent {
      return OpenHistoryWebsiteIntentHandler()
    }

    if intent is OpenBookmarkWebsiteIntent {
      return OpenBookmarkWebsiteIntent()
    }

    if intent is LockScreenFavoriteConfigurationIntent {
      return LockScreenFavoriteIntentHandler()
    }
    
    return self
  }
}
