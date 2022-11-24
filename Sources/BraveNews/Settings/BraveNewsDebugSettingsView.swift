// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SwiftUI
import BraveShared
import BraveUI

extension FeedDataSource.Environment {
  fileprivate var name: String {
    switch self {
    case .dev: return "Dev"
    case .staging: return "Staging"
    case .production: return "Production"
    }
  }
}

extension FeedDataSource {
  fileprivate func description(of state: State) -> String {
    switch state {
    case .initial:
      return "—"
    case .loading:
      return "Loading"
    case .success:
      return "Success"
    case .failure:
      return "Error"
    }
  }
}

public struct BraveNewsDebugSettingsView: View {
  private let feedDataSource: FeedDataSource
  private let dismissAction: (() -> Void)?
  
  public init(dataSource: FeedDataSource, dismissAction: (() -> Void)? = nil) {
    self.feedDataSource = dataSource
    self.dismissAction = dismissAction
    // These values should always be overridden
    self._environment = .init(wrappedValue: dataSource.environment)
  }
  
  @State private var environment: FeedDataSource.Environment = .production
  
  public var body: some View {
    List {
      Section {
        Picker("Environment", selection: $environment) {
          ForEach(FeedDataSource.Environment.allCases, id: \.self) { env in
            Text(env.name)
          }
        }
      } footer: {
        Text("Changing the environment or language will purge all cached resources immediately.")
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        HStack {
          Text("State")
          Spacer()
          Text(feedDataSource.description(of: feedDataSource.state))
            .foregroundColor(.secondary)
        }
        switch feedDataSource.state {
        case .initial:
          EmptyView()
        case .loading(let previousState):
          HStack {
            Text("Previous State")
            Spacer()
            Text(feedDataSource.description(of: previousState))
              .foregroundColor(.secondary)
          }
        case .success(let cards):
          HStack {
            Text("Sources")
            Spacer()
            Text("\(feedDataSource.sources.count)")
              .foregroundColor(.secondary)
          }
          HStack {
            Text("Cards Generated")
            Spacer()
            Text("\(cards.count)")
              .foregroundColor(.secondary)
          }
        case .failure(let error):
          Text(error.localizedDescription)
            .foregroundColor(.secondary)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle("Brave News QA Settings")
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        if let dismissAction = dismissAction {
          Button(action: dismissAction) {
            Text("Done")
          }
        }
      }
    }
    .onChange(of: environment) { _ in
      feedDataSource.environment = environment
    }
  }
}

#if DEBUG
struct BraveNewsDebugSettingsView_PreviewProvided: PreviewProvider {
  static var previews: some View {
    NavigationView {
      BraveNewsDebugSettingsView(dataSource: .init())
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
