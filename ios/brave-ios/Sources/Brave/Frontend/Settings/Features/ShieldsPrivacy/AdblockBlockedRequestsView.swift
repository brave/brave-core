// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import Strings
import SwiftUI

struct AdblockBlockedRequestsView: View {

  let url: String
  @ObservedObject var contentBlockerHelper: ContentBlockerHelper

  @State private var filterText: String = ""

  private var blockedRequests: [BlockedRequestInfo] {
    let blockedRequests = Array(contentBlockerHelper.blockedRequests)
    guard !filterText.isEmpty else {
      return blockedRequests
    }
    return blockedRequests.filter {
      $0.requestURL.absoluteString.localizedCaseInsensitiveContains(filterText)
        || $0.sourceURL.absoluteString.localizedCaseInsensitiveContains(filterText)
        || $0.resourceType.rawValue.localizedCaseInsensitiveContains(filterText)
        || $0.location.display.localizedCaseInsensitiveContains(filterText)
    }
  }

  public var body: some View {
    List {
      Section(header: Text(url)) {
        ForEach(blockedRequests) { request in
          VStack {
            row(
              title: String.localizedStringWithFormat("%@:", Strings.Shields.requestURLLabel),
              detail: request.requestURL.absoluteString
            )
            row(
              title: String.localizedStringWithFormat("%@:", Strings.Shields.sourceURLLabel),
              detail: request.sourceURL.absoluteString
            )
            row(
              title: String.localizedStringWithFormat("%@:", Strings.Shields.resourceTypeLabel),
              detail: request.resourceType.rawValue
            )
            row(
              title: String.localizedStringWithFormat("%@:", Strings.Shields.aggressiveLabel),
              detail: "\(request.isAggressive)"
            )
            row(
              title: String.localizedStringWithFormat("%@:", Strings.Shields.blockedByLabel),
              detail: request.location.display
            )
          }
        }
      }
    }
    .navigationTitle(Strings.Shields.blockedRequestsTitle)
    .toolbar(.visible)
    .searchable(text: $filterText)
  }

  @ViewBuilder private func row(title: String, detail: String) -> some View {
    Group {
      Text(title)
      Text(detail)
        .font(.system(.caption, design: .monospaced))
        .textSelection(.enabled)
    }
    .font(.body)
    .frame(maxWidth: .infinity, alignment: .leading)
  }
}
