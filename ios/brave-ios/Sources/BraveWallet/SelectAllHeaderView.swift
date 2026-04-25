// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// Header view for Wallet v2 with an optional `Select All` / `Deselect All` button.
struct SelectAllHeaderView<T: Identifiable>: View {

  let title: String
  let showsSelectAllButton: Bool
  let allModels: [T]
  let selectedModels: [T]
  let select: (T) -> Void

  var body: some View {
    HStack {
      Text(title)
        .font(.body.weight(.semibold))
        .foregroundColor(Color(uiColor: WalletV2Design.textPrimary))
      Spacer()
      if showsSelectAllButton {
        Button(action: selectAll) {
          Text(selectAllButtonTitle(allSelected))
            .font(.callout.weight(.semibold))
            .foregroundColor(Color(uiColor: WalletV2Design.textInteractive))
        }
      }
    }
    .padding(.horizontal)
    .padding(.vertical, 12)
  }

  private var allSelected: Bool {
    allModels.allSatisfy({ model in
      selectedModels.contains(where: { $0.id == model.id })
    })
  }

  private func selectAllButtonTitle(_ allSelected: Bool) -> String {
    if allSelected {
      return Strings.Wallet.deselectAllButtonTitle
    }
    return Strings.Wallet.selectAllButtonTitle
  }

  private func selectAll() {
    if allSelected {  // deselect all
      allModels.forEach(select)
    } else {  // select all
      for model in allModels where !selectedModels.contains(where: { $0.id == model.id }) {
        select(model)
      }
    }
  }
}
