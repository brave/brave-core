// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import BraveUI
import Combine
import Foundation
import Preferences
import SwiftUI

public class NewsSettingsViewController: UIHostingController<NewsSettingsView> {
  private let dataSource: FeedDataSource
  private let searchDelegate = SearchDelegate()
  private var cancellables: Set<AnyCancellable> = []
  public var viewDidDisappear: (() -> Void)?

  public init(
    dataSource: FeedDataSource,
    openURL: @escaping (URL) -> Void
  ) {
    self.dataSource = dataSource
    super.init(rootView: NewsSettingsView(dataSource: dataSource, searchDelegate: searchDelegate))
    rootView.tappedOptInLearnMore = {
      openURL(.brave.braveNewsPrivacy)
    }
  }

  private var isControllerSetUp: Bool = false
  private func setUpController() {
    if isControllerSetUp { return }
    defer { isControllerSetUp = true }

    if navigationController?.viewControllers.first === self {
      navigationItem.rightBarButtonItem = .init(
        barButtonSystemItem: .done,
        target: self,
        action: #selector(tappedDone)
      )
    }
    navigationItem.largeTitleDisplayMode = .never
    navigationItem.title = Strings.BraveNews.braveNews
    if Preferences.BraveNews.isEnabled.value {
      navigationItem.searchController = searchController
    }
    navigationItem.hidesSearchBarWhenScrolling = false
    navigationItem.preferredSearchBarPlacement = .stacked

    // Hide the search bar when Brave News is off
    Preferences.BraveNews.isEnabled.$value
      .sink { [weak self] isEnabled in
        guard let self else { return }
        self.navigationItem.searchController = isEnabled ? self.searchController : nil
        self.navigationItem.hidesSearchBarWhenScrolling = false
        // Setting `searchController` to nil seems to invalidate this setting and needs to be set again
        self.navigationItem.preferredSearchBarPlacement = .stacked
        self.navigationController?.setToolbarHidden(!isEnabled, animated: true)
      }
      .store(in: &cancellables)

    Preferences.BraveNews.userOptedIn.$value
      .sink { [weak self] isOptedIn in
        self?.navigationController?.setToolbarHidden(!isOptedIn, animated: true)
      }
      .store(in: &cancellables)

    // Hide the toolbar when search overlay is visible
    searchDelegate.$query
      .sink { [weak self] query in
        guard let self else { return }
        self.navigationController?.setToolbarHidden(!query.isEmpty, animated: false)
      }
      .store(in: &cancellables)
  }

  public override func viewDidLoad() {
    super.viewDidLoad()
    setUpController()
  }

  private lazy var searchController = UISearchController(searchResultsController: nil).then {
    $0.automaticallyShowsCancelButton = true
    $0.hidesNavigationBarDuringPresentation = true
    $0.obscuresBackgroundDuringPresentation = false
    $0.automaticallyShowsSearchResultsController = false
    $0.searchBar.delegate = searchDelegate
    $0.searchBar.placeholder = Strings.BraveNews.searchPlaceholder
  }

  @objc private func tappedDone() {
    dismiss(animated: true)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    navigationController?.setToolbarHidden(
      !Preferences.BraveNews.isEnabled.value,
      animated: animated
    )
  }

  public override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    if navigationController?.isBeingDismissed == false {  // Gesture dismiss
      navigationController?.setToolbarHidden(true, animated: animated)
    }
  }

  public override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    if navigationController?.isBeingDismissed == true {
      viewDidDisappear?()
    }
  }
}

public struct NewsSettingsView: View {
  @ObservedObject var dataSource: FeedDataSource
  @ObservedObject fileprivate var searchDelegate: SearchDelegate
  @ObservedObject private var isNewsEnabled = Preferences.BraveNews.isEnabled
  @ObservedObject private var userOptedIn = Preferences.BraveNews.userOptedIn

  fileprivate var tappedOptInLearnMore: (() -> Void)?

  @State private var searchResults: SearchResults?
  @State private var isShowingImportOPML: Bool = false

  private var showBraveNewsToggle: some View {
    Toggle(isOn: $isNewsEnabled.value.animation(.default)) {
      Text(Strings.BraveNews.isEnabledToggleLabel)
        .font(.headline)
        .foregroundColor(Color(.bravePrimary))
        .frame(maxWidth: .infinity, alignment: .leading)
    }
    .toggleStyle(SwitchToggleStyle(tint: Color(.braveBlurpleTint)))
  }

  // Xcode typechecker struggled when these were inside of `body`
  @ViewBuilder private var destinations: some View {
    let followedSourcesCount = dataSource.followedSources.count
    let totalFollowCount =
      followedSourcesCount + dataSource.followedChannels.count + dataSource.rssFeedLocations.count
    NavigationLink {
      SourceListContainerView(dataSource: dataSource)
    } label: {
      DestinationLabel(
        image: Image(braveSystemName: "leo.crown"),
        title: Strings.BraveNews.popularSourcesButtonTitle,
        subtitle: Strings.BraveNews.popularSourcesButtonSubtitle
      )
    }
    if followedSourcesCount > 0 {
      // Only have suggestions if you have followed individual sources
      NavigationLink {
        SourceSuggestionsContainerView(dataSource: dataSource)
      } label: {
        DestinationLabel(
          image: Image(braveSystemName: "leo.star.outline"),
          title: Strings.BraveNews.suggestedSourcesButtonTitle,
          subtitle: Strings.BraveNews.suggestedSourcesButtonSubtitle
        )
      }
    }
    if !dataSource.channels.isEmpty {
      NavigationLink {
        ChannelListContainerView(dataSource: dataSource)
      } label: {
        DestinationLabel(
          image: Image(braveSystemName: "leo.product.brave-news"),
          title: Strings.BraveNews.channelsButtonTitle,
          subtitle: Strings.BraveNews.channelsButtonSubtitle
        )
      }
    }
    if totalFollowCount > 0 {
      NavigationLink {
        FollowingListContainerView(dataSource: dataSource)
      } label: {
        DestinationLabel(
          image: Image(braveSystemName: "leo.heart.outline"),
          title: {
            HStack {
              Text(Strings.BraveNews.followingButtonTitle)
              Text(NSNumber(value: totalFollowCount), formatter: NumberFormatter())
                .font(.caption)
                .padding(.vertical, 1)
                .padding(.horizontal, 6)
                .background(Capsule().stroke(Color(.secondaryButtonTint), lineWidth: 1))
                .foregroundColor(Color(.secondaryBraveLabel))
            }
          },
          subtitle: Strings.BraveNews.followingButtonSubtitle
        )
      }
    }
  }

  public var body: some View {
    Form {
      Section {
        if isNewsEnabled.value {
          destinations
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        showBraveNewsToggle
          .padding(.vertical)
          .resetListHeaderStyle()
      }
    }
    .listStyle(.insetGrouped)
    .animation(.default, value: searchDelegate.isEditing)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .navigationTitle(Strings.BraveNews.braveNews)
    .navigationBarTitleDisplayMode(.inline)
    .onChange(of: searchDelegate.query) { query in
      searchResults = dataSource.search(query: query)
    }
    .onChange(of: dataSource.rssFeedLocations) { _ in
      withAnimation {
        searchResults = dataSource.search(query: searchDelegate.query)
      }
    }
    .overlay(
      Group {
        if !searchDelegate.query.isEmpty {
          SearchResultsView(
            dataSource: dataSource,
            query: searchDelegate.query,
            results: searchResults ?? .empty
          )
        }
      }
    )
    .background(
      Group {
        if searchDelegate.query.isEmpty {
          Color.clear
            .toolbar {
              ToolbarItemGroup(placement: .bottomBar) {
                if isNewsEnabled.value {
                  Spacer()
                  Menu {
                    Button {
                      isShowingImportOPML = true
                    } label: {
                      Label(Strings.BraveNews.importOPML, systemImage: "square.and.arrow.down")
                    }
                  } label: {
                    Image(braveSystemName: "leo.more.horizontal")
                      .frame(height: 44)  // Menu label's don't have proper tap areas in toolbar
                  }
                }
              }
            }
        }
      }
    )
    .opmlImporter(isPresented: $isShowingImportOPML, dataSource: dataSource)
    .overlay(optInView)
  }

  @ViewBuilder private var optInView: some View {
    if !userOptedIn.value || !isNewsEnabled.value {
      OptInView { @MainActor in
        Preferences.BraveNews.isShowingOptIn.value = false
        // Initialize ads if it hasn't already been done
        await dataSource.getAdsAPI?().initialize()
        if dataSource.isSourcesExpired {
          await withCheckedContinuation { c in
            dataSource.load {
              c.resume()
            }
          }
        }
        withAnimation {
          userOptedIn.value = true
          isNewsEnabled.value = true
        }
      } tappedLearnMore: {
        tappedOptInLearnMore?()
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(Color(.braveBackground).ignoresSafeArea())
      .transition(.opacity.animation(.default))
    }
  }
}

private struct DestinationLabel<Title: View>: View {
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
    // Not sure why this is required on iOS 14 tbh
    objectWillChange.send()
  }
  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    query = ""
    // Not sure why this is required on iOS 14 tbh
    objectWillChange.send()
  }
}

#if DEBUG
struct NewsSettingsView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    UIKitController(
      UINavigationController(
        rootViewController: NewsSettingsViewController(dataSource: .init(), openURL: { _ in })
      )
    )
    .accentColor(Color(.braveBlurpleTint))
  }
}
#endif
