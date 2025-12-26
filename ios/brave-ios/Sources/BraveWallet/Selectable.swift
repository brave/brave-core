// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

struct Selectable<T: Identifiable & Equatable>: Equatable, Identifiable {
  let isSelected: Bool
  let model: T

  var id: T.ID { model.id }
}
