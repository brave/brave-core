// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import DesignSystem
import Favicon
import Preferences
import Shared
import SwiftUI

struct HistoryItemView: View {
  var title: String?
  var url: URL

  var body: some View {
    HStack {
      FaviconImage(
        url: url.absoluteString,
        isPrivateBrowsing: false
      )
      .overlay(
        ContainerRelativeShape()
          .strokeBorder(Color(braveSystemName: .dividerSubtle), lineWidth: 1.0)
      )
      .containerShape(RoundedRectangle(cornerRadius: 6.0, style: .continuous))

      VStack {
        if let title = title, !title.isEmpty {
          Text(title)
            .font(.subheadline)
            .lineLimit(2)
            .truncationMode(.tail)
            .frame(maxWidth: .infinity, alignment: .leading)
            .fixedSize(horizontal: false, vertical: true)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        }

        URLElidedText(
          text: URLFormatter.formatURLOrigin(
            forDisplayOmitSchemePathAndTrivialSubdomains: url.absoluteString
          )
        )
        .truncationMode(.tail)
        .font(.footnote)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
        .environment(\.layoutDirection, .leftToRight)
        .flipsForRightToLeftLayoutDirection(false)
      }
    }
  }
}

struct HistoryView: View {
  @ObservedObject
  var model: HistoryModel

  @State
  private var searchText = ""

  @State
  private var deleteAllAlertPresented = false

  @State
  private var timer: Timer?

  var body: some View {
    VStack(spacing: 0.0) {
      if model.items.isEmpty {
        HistoryEmptyStateView(
          details: .init(
            title: Preferences.Privacy.privateBrowsingOnly.value
              ? Strings.History.historyPrivateModeOnlyStateTitle
              : Strings.History.historyEmptyStateTitle,
            icon: UIImage(named: "emptyHistory", in: .module, compatibleWith: nil)
          )
        )
      } else {
        List {
          ForEach(model.items.elements, id: \.key) { section, nodes in
            Section {
              ForEach(nodes, id: \.self) { node in
                Button {
                  model.handleHistoryItemSelection(.selectTab, node: node)
                } label: {
                  HistoryItemView(title: node.title, url: node.url)
                }
                .contextMenu(menuItems: {
                  Button {
                    model.handleHistoryItemSelection(.openInNewTab, node: node)
                  } label: {
                    Text(Strings.openNewTabButtonTitle)
                      .frame(maxWidth: .infinity, alignment: .leading)
                      .fixedSize(horizontal: false, vertical: true)
                    Image(systemName: "plus.square.on.square")
                  }

                  if !model.isPrivateBrowsing {
                    Button {
                      model.handleHistoryItemSelection(.openInNewPrivateTab, node: node)
                    } label: {
                      Text(Strings.openNewPrivateTabButtonTitle)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .fixedSize(horizontal: false, vertical: true)
                      Image(systemName: "plus.square.fill.on.square.fill")
                    }
                  }

                  Divider()

                  Button {
                    model.handleHistoryItemSelection(.copyLink, node: node)
                  } label: {
                    Text(Strings.copyLinkActionTitle)
                      .frame(maxWidth: .infinity, alignment: .leading)
                      .fixedSize(horizontal: false, vertical: true)
                    Image(systemName: "doc.on.doc")
                  }

                  Button {
                    model.handleHistoryItemSelection(.shareLink, node: node)
                  } label: {
                    Text(Strings.shareLinkActionTitle)
                      .frame(maxWidth: .infinity, alignment: .leading)
                      .fixedSize(horizontal: false, vertical: true)
                    Image(systemName: "square.and.arrow.up")
                  }
                })
              }
              .onDelete {
                self.delete($0, in: section)
              }
            } header: {
              Text(section.title)
                .font(.headline)
                .frame(maxWidth: .infinity, alignment: .leading)
                .fixedSize(horizontal: false, vertical: true)
                .foregroundStyle(Color(braveSystemName: .textPrimary))
            }
          }
        }
        .listStyle(.plain)
        .toolbar {
          Button {
            deleteAllAlertPresented = true
          } label: {
            Label {
              Text(Strings.History.historyClearActionTitle)
            } icon: {
              Image(braveSystemName: "leo.trash")
            }
            .labelStyle(.iconOnly)
          }
        }
        .alert(isPresented: $deleteAllAlertPresented) {
          Alert(
            title: Text(Strings.History.historyClearAlertTitle),
            message: Text(Strings.History.historyClearAlertDescription),
            primaryButton: .destructive(
              Text(Strings.History.historyClearActionTitle),
              action: {
                model.deleteAll()
              }
            ),
            secondaryButton: .cancel()
          )
        }
      }
    }
    .navigationTitle(Strings.historyScreenTitle)
    .searchable(
      text: $searchText,
      placement: .navigationBarDrawer(displayMode: .always),
      prompt: Strings.History.historySearchBarTitle
    )
    .onChange(of: searchText) { searchText in
      self.timer?.invalidate()
      self.timer = Timer.scheduledTimer(
        withTimeInterval: 0.1,
        repeats: false,
        block: { [weak model] timer in
          timer.invalidate()
          model?.refreshHistory(query: searchText)
        }
      )
    }
  }

  func delete(_ offsets: IndexSet, in section: HistorySection) {
    guard let nodes = model.items[section] else { return }
    let nodesToDelete = offsets.map({ nodes[$0] })
    model.delete(nodes: nodesToDelete)
  }
}

#if DEBUG
#Preview {
  HistoryView(
    model: HistoryModel(
      api: nil,
      tabManager: nil,
      toolbarUrlActionsDelegate: nil,
      dismiss: {},
      askForAuthentication: { _, _ in

      }
    )
  )
}
#endif
