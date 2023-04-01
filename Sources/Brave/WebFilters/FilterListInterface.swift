// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data

protocol FilterListInterface {
  @MainActor var uuid: String { get }
  @MainActor var debugTitle: String { get }
}

extension FilterListInterface {
  @MainActor func makeResource(componentId: String) -> BraveS3Resource {
    return .filterListContentBlockingBehaviors(
      uuid: uuid, componentId: componentId
    )
  }
}

extension FilterListSetting: FilterListInterface {
  var debugTitle: String {
    return "\(uuid) \(componentId ?? "unknown")"
  }
}

extension FilterList: FilterListInterface {
  var uuid: String { entry.uuid }
  
  var debugTitle: String {
    return "\(entry.title) \(entry.componentId)"
  }
}
