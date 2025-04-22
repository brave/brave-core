// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import Data
import DesignSystem
import Favicon
import Preferences
import Shared
import SwiftUI

struct HistoryItemView: View {
  var title: String?
  var url: URL
  var dateAdded: Date?

  var body: some View {
    Label {
      HStack(alignment: .top) {
        VStack {
          if let title = title, !title.isEmpty {
            Text(title)
              .lineLimit(1)
              .truncationMode(.tail)
              .frame(maxWidth: .infinity, alignment: .leading)
              .fixedSize(horizontal: false, vertical: true)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
          }

          URLElidedText(
            text: URLFormatter.formatURLOrigin(
              forDisplayOmitSchemePathAndTrivialSubdomains: url.strippingBlobURLAuth.absoluteString
            )
          )
          .font(.footnote)
          .frame(maxWidth: .infinity, alignment: .leading)
          .fixedSize(horizontal: false, vertical: true)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
        }

        if let dateAdded {
          Text(dateAdded, format: .dateTime.hour().minute())
            .font(.caption2)
            .foregroundStyle(Color(braveSystemName: .textTertiary))
        }
      }
    } icon: {
      FaviconImage(url: url, isPrivateBrowsing: false)
        .frame(width: 20, height: 20)
        .padding(6)
        .overlay(
          ContainerRelativeShape()
            .strokeBorder(Color(braveSystemName: .dividerSubtle), lineWidth: 1.0)
        )
        .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
    }
  }
}

private struct RecentlyClosedTabsSection: View {
  @FetchRequest(
    entity: RecentlyClosed.entity(),
    sortDescriptors: [.init(keyPath: \RecentlyClosed.dateAdded, ascending: false)]
  ) private var recentlyClosedTabs: FetchedResults<RecentlyClosed>

  @State private var isExpanded: Bool = false
  var restoreTab: (RecentlyClosed) -> Void

  var body: some View {
    if !recentlyClosedTabs.isEmpty {
      Section {
        Button {
          withAnimation {
            isExpanded.toggle()
          }
        } label: {
          Label {
            HStack {
              Text("Recently Closed Tabs")
                .foregroundStyle(Color(braveSystemName: .textPrimary))
              Spacer()
              Image(braveSystemName: "leo.carat.down")
                .foregroundStyle(Color(braveSystemName: .iconDefault))
                .rotationEffect(.degrees(isExpanded ? 180 : 0))
            }
          } icon: {
            Image(braveSystemName: "leo.browser.mobile-recent-tabs")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
          }
          .padding(.vertical, 4)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        if isExpanded {
          ForEach(recentlyClosedTabs, id: \RecentlyClosed.objectID) { tab in
            if let url = URL(string: tab.url) {
              Button {
                restoreTab(tab)
              } label: {
                HistoryItemView(title: tab.title, url: url, dateAdded: tab.dateAdded)
              }
              .listRowBackground(Color(.secondaryBraveGroupedBackground))
            }
          }
          .onDelete { indexSet in
            // Delete an individual info from recently closed
            let tabsToRemove = indexSet.map { recentlyClosedTabs[$0] }
            withAnimation {
              for tab in tabsToRemove {
                RecentlyClosed.remove(with: tab.url)
              }
            }
          }
        }
      } header: {
        // This is unfortunately needed to ensure there's proper padding at the top of the list
        // most likely due to using UIAppearance defaults across the app to manage the navigation
        // bar appearance
        Color.clear
      }
    }
  }
}

struct HistoryView: View {
  @Environment(\.dismiss) private var dismiss

  @ObservedObject var model: HistoryModel

  @State private var searchText = ""
  @State private var deleteAllAlertPresented = false
  @State private var timer: Timer?

  var body: some View {
    NavigationStack {
      List {
        if searchText.isEmpty {
          RecentlyClosedTabsSection(restoreTab: { tab in
            model.handleRecentlyClosedSelection(tab)
          })
          .environment(\.managedObjectContext, DataController.swiftUIContext)
        }
        ForEach(model.items.elements, id: \.key) { section, nodes in
          Section {
            ForEach(nodes, id: \.self) { node in
              Button {
                model.handleHistoryItemSelection(.selectTab, node: node)
              } label: {
                HistoryItemView(title: node.title, url: node.url, dateAdded: node.dateAdded)
              }
              .listRowBackground(Color(.secondaryBraveGroupedBackground))
              .contextMenu {
                Section {
                  Button {
                    model.handleHistoryItemSelection(.openInNewTab, node: node)
                  } label: {
                    Label(Strings.openNewTabButtonTitle, systemImage: "plus.square.on.square")
                  }

                  if !model.isPrivateBrowsing {
                    Button {
                      model.handleHistoryItemSelection(.openInNewPrivateTab, node: node)
                    } label: {
                      Label(
                        Strings.openNewPrivateTabButtonTitle,
                        systemImage: "plus.square.fill.on.square.fill"
                      )
                    }
                  }
                }
                Section {
                  Button {
                    model.handleHistoryItemSelection(.copyLink, node: node)
                  } label: {
                    Label(Strings.copyLinkActionTitle, systemImage: "doc.on.doc")
                  }
                  Button {
                    model.handleHistoryItemSelection(.shareLink, node: node)
                  } label: {
                    Label(Strings.shareLinkActionTitle, systemImage: "square.and.arrow.up")
                  }
                }
                Section {
                  Button(role: .destructive) {
                    withAnimation {
                      model.delete(nodes: [node])
                    }
                  } label: {
                    Label(Strings.History.deleteFromHistory, braveSystemImage: "leo.trash")
                  }
                }
              }
            }
            .onDelete { indexSet in
              withAnimation {
                self.delete(indexSet, in: section)
              }
            }
          } header: {
            Text(section.title)
              .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        }
      }
      .animation(.default, value: model.items)
      .scrollContentBackground(.hidden)
      .background(Color(.braveGroupedBackground))
      .listStyle(.insetGrouped)
      .overlay {
        if model.items.isEmpty {
          HistoryEmptyStateView(isSearching: !searchText.isEmpty)
            .transition(.opacity.animation(.default))
        }
      }
      .toolbar {
        ToolbarItemGroup(placement: .topBarLeading) {
          Button {
            dismiss()
          } label: {
            Label {
              Text(Strings.close)
            } icon: {
              Image(braveSystemName: "leo.close")
            }
            .labelStyle(.iconOnly)
          }
        }
        ToolbarItemGroup(placement: .topBarTrailing) {
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
      .navigationTitle(Strings.historyScreenTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbarBackground(.visible, for: .navigationBar)
      .searchable(
        text: $searchText,
        placement: .navigationBarDrawer,
        prompt: Strings.History.historySearchBarTitle
      )
    }
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
