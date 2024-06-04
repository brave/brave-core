// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct HistoryEmptyStateView: UIViewRepresentable {
  private let details: EmptyOverlayStateDetails

  init(details: EmptyOverlayStateDetails) {
    self.details = details
  }

  func makeUIView(context: Context) -> EmptyStateOverlayView {
    return EmptyStateOverlayView(overlayDetails: details)
  }

  func updateUIView(_ view: EmptyStateOverlayView, context: Context) {

  }
}
