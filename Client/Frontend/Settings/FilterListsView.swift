// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import Data
import DesignSystem

/// A view showing enabled and disabled community filter lists
struct FilterListsView: View {
  @ObservedObject private var downloader = FilterListResourceDownloader.shared
  
  var body: some View {
    List {
      Section {
        ForEach($downloader.filterLists) { $filterList in
          Toggle(isOn: $filterList.isEnabled) {
            VStack(alignment: .leading) {
              Text(filterList.title)
                .foregroundColor(Color(.bravePrimary))
              Text(filterList.description)
                .font(.caption)
                .foregroundColor(Color(.secondaryBraveLabel))
            }
          }.toggleStyle(SwitchToggleStyle(tint: .accentColor))
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        Text(Strings.filterListsDescription)
          .textCase(.none)
      }
      
    }.navigationTitle(Strings.filterLists)
  }
  
  /// Get an index for the given filter list.
  func getIndex(for filterList: FilterList) -> Int? {
    return downloader.filterLists.firstIndex(where: { $0.id == filterList.id })
  }
}

#if DEBUG
struct FilterListsView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      FilterListsView()
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
