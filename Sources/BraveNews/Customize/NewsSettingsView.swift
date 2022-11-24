// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import SwiftUI
import Combine
import BraveUI

struct Divided<Content: View, Divider: View>: View {
  var content: Content
  var divider: Divider
  
  init(
    by divider: Divider,
    @ViewBuilder content: () -> Content
  ) {
    self.divider = divider
    self.content = content()
  }
  
  private struct _Layout<Divider: View>: _VariadicView_MultiViewRoot {
    var divider: Divider
    func body(children: _VariadicView.Children) -> some View {
      let last = children.last?.id
      ForEach(children) { child in
        child
        if child.id != last {
          divider
        }
      }
    }
  }
  
  var body: some View {
    _VariadicView.Tree(_Layout(divider: divider)) {
      content
    }
  }
}

struct DestinationLabel<Title: View>: View {
  var image: Image
  var title: Title
  var subtitle: String?
  
  @ScaledMetric private var imageSize: CGFloat = 38.0
  
  init(
    image: Image,
    @ViewBuilder title: () -> Title,
    subtitle: String? = nil
  ) {
    self.image = image
    self.title = title()
    self.subtitle = subtitle
  }
  
  var body: some View {
    HStack(spacing: 12) {
      image
        .foregroundColor(Color(.braveLabel))
        .frame(width: imageSize, height: imageSize)
        .background(Color(.secondaryBraveBackground).clipShape(Circle()))
      VStack(alignment: .leading, spacing: 2) {
        title
          .font(.headline)
          .foregroundColor(Color(.braveLabel))
        if let subtitle {
          Text(subtitle)
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
      }
    }
    .padding(.vertical, 6)
  }
}

extension DestinationLabel where Title == Text {
  init(
    image: Image,
    title: String,
    subtitle: String? = nil
  ) {
    self.init(image: image, title: { Text(title) }, subtitle: subtitle)
  }
}

private class SearchDelegate: NSObject, UISearchBarDelegate, ObservableObject {
  @Published var query: String = ""
  @Published var isEditing: Bool = false
  
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    isEditing = true
  }
  func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
    isEditing = false
  }
  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    query = searchText
  }
  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    query = ""
  }
}

public class NewsSettingsViewController: UIViewController {
  private let hostingController: UIHostingController<NewsSettingsView>
  private let searchDelegate = SearchDelegate()
  private var cancellable: AnyCancellable?
  public var viewDidDisappear: (() -> Void)?
  
  public init(dataSource: FeedDataSource) {
    hostingController = .init(
      rootView: NewsSettingsView(dataSource: dataSource, searchDelegate: searchDelegate)
    )
    super.init(nibName: nil, bundle: nil)
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    if navigationController?.viewControllers.first === self {
      navigationItem.rightBarButtonItem = .init(
        barButtonSystemItem: .done,
        target: self,
        action: #selector(tappedDone)
      )
    }
    navigationItem.largeTitleDisplayMode = .never
    navigationItem.title = "Brave News" // DNT?
    navigationItem.hidesSearchBarWhenScrolling = false
    if Preferences.BraveNews.isEnabled.value {
      navigationItem.searchController = searchController
    }
    if #available(iOS 16.0, *) {
      navigationItem.preferredSearchBarPlacement = .stacked
    }
    
    addChild(hostingController)
    view.addSubview(hostingController.view)
    hostingController.didMove(toParent: self)
    hostingController.view.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    cancellable = Preferences.BraveNews.isEnabled.$value
      .sink { [weak self] isEnabled in
        guard let self else { return }
        self.navigationItem.searchController = isEnabled ? self.searchController : nil
      }
  }
  
  private lazy var searchController = UISearchController(searchResultsController: nil).then {
    $0.automaticallyShowsCancelButton = true
    $0.hidesNavigationBarDuringPresentation = true
    $0.obscuresBackgroundDuringPresentation = false
    $0.automaticallyShowsSearchResultsController = false
    $0.searchBar.delegate = searchDelegate
    $0.searchBar.placeholder = "Search for news, site, topic or RSS feed" // TODO: Localize
  }
  
  @objc private func tappedDone() {
    dismiss(animated: true)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  public override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    viewDidDisappear?()
  }
}

private struct NewsSettingsView: View {
  @ObservedObject var dataSource: FeedDataSource
  @ObservedObject var searchDelegate: SearchDelegate
  @ObservedObject private var isNewsEnabled = Preferences.BraveNews.isEnabled
  @State private var searchResults: SearchResults?
  
  var showBraveNewsToggle: some View {
    Toggle(isOn: $isNewsEnabled.value.animation(.default)) {
      VStack(alignment: .leading, spacing: 4) {
        Text("Show Brave News") // TODO: Localize
          .font(.headline)
          .foregroundColor(Color(.bravePrimary))
        Text("Ad-supported, private and anonymised ads matched on your device.") // TODO: Localize
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .frame(maxWidth: .infinity, alignment: .leading)
    }
    .toggleStyle(SwitchToggleStyle(tint: Color(.braveOrange)))
  }
  
  public var body: some View {
    let totalFollowCount = dataSource.followedSources.count + dataSource.followedChannels.count
    List {
      Section {
        if isNewsEnabled.value {
          NavigationLink {
            SourceListContainerView(dataSource: dataSource)
          } label: {
            DestinationLabel(
              image: Image(braveSystemName: "brave.crown"),
              title: "Popular Sources", // TODO: Localize
              subtitle: "Currently trending sources" // TODO: Localize
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          NavigationLink {
            EmptyView()
          } label: {
            DestinationLabel(
              image: Image(braveSystemName: "brave.star"),
              title: "Suggested", // TODO: Localize
              subtitle: "Our curated list of sources"
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          if !dataSource.channels.isEmpty {
            NavigationLink {
              ChannelListContainerView(dataSource: dataSource)
            } label: {
              DestinationLabel(
                image: Image(braveSystemName: "brave.newspaper"),
                title: "Channels", // TODO: Localize
                subtitle: "Follow topics relevant to you" // TODO: Localize
              )
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
          if totalFollowCount > 0 {
            NavigationLink {
              FollowingListContainerView(dataSource: dataSource)
            } label: {
              DestinationLabel(
                image: Image(braveSystemName: "brave.heart"),
                title: {
                  HStack {
                    Text("Following") // TODO: Localize
                    Text(NSNumber(value: totalFollowCount), formatter: NumberFormatter())
                      .font(.caption)
                      .padding(.vertical, 1)
                      .padding(.horizontal, 6)
                      .background(Capsule().stroke(Color(.secondaryButtonTint), lineWidth: 1))
                      .foregroundColor(Color(.secondaryBraveLabel))
                  }
                },
                subtitle: "Manage your following list" // TODO: Localize
              )
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }
      } header: {
        showBraveNewsToggle
          .padding(.bottom)
          .padding(.vertical)
          .resetListHeaderStyle()
      }
    }
    .animation(.default, value: searchDelegate.isEditing)
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .onChange(of: searchDelegate.query) { query in
      searchResults = dataSource.search(query: query)
    }
    .overlay(Group {
      if !searchDelegate.query.isEmpty {
        SearchResultsView(
          results: searchResults ?? .empty,
          isFollowingSource: { dataSource.isFollowingSourceBinding(source: $0) },
          isFollowingChannel: { dataSource.isFollowingChannelBinding(channel: $0) }
        )
      }
    })
  }
}

#if DEBUG
struct NewsSettingsView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    UIKitController(
      UINavigationController(
        rootViewController: NewsSettingsViewController(dataSource: .init())
      )
    )
    .accentColor(Color(.braveOrange))
  }
}
#endif
