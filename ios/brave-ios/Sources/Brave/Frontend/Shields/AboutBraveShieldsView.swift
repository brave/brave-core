// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Strings
import SwiftUI

struct AboutBraveShieldsView: View {
  var body: some View {
    ScrollView {
      Text(Strings.Shields.aboutBraveShieldsBody)
    }
    .padding()
    .navigationTitle(Strings.Shields.aboutBraveShieldsTitle)
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .toolbar(.visible)
  }
}

#Preview {
  AboutBraveShieldsView()
}
