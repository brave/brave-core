// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Strings
import SwiftUI

public struct AdblockBlockedRequestsView: View {

  private let url: String
  private let blockedRequestsPublisher: AnyPublisher<[BlockedRequestInfo], Never>

  @State private var allBlockedRequests: [BlockedRequestInfo] = []
  @State private var filterText: String = ""

  private var blockedRequests: [BlockedRequestInfo] {
    if filterText.isEmpty {
      return allBlockedRequests
    }
    return allBlockedRequests.filter {
      $0.requestURL.absoluteString.localizedCaseInsensitiveContains(filterText)
        || $0.sourceURL.absoluteString.localizedCaseInsensitiveContains(filterText)
        || $0.resourceType.rawValue.localizedCaseInsensitiveContains(filterText)
        || $0.location.display.localizedCaseInsensitiveContains(filterText)
    }
  }

  public init(url: String, blockedRequests: AnyPublisher<[BlockedRequestInfo], Never>) {
    self.url = url
    self.blockedRequestsPublisher = blockedRequests
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
    .searchable(text: $filterText)
    .toolbarVisibility(.visible, for: .navigationBar)
    .onReceive(blockedRequestsPublisher.receive(on: DispatchQueue.main)) { blockedRequests in
      allBlockedRequests = blockedRequests
    }
  }

  private func row(title: String, detail: String) -> some View {
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
