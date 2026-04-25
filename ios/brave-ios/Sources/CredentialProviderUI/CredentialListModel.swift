// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AuthenticationServices
import BraveCore
import Foundation

public class CredentialListModel: ObservableObject {
  @Published private(set) var suggestedCredentials: [any Credential] = []
  @Published private(set) var allCredentials: [any Credential] = []
  @Published private(set) var originHost: String?
  @Published private(set) var faviconAttributes: [String: FaviconAttributes] = [:]
  @Published var isAuthenticated: Bool = false
  public var actionHandler: ((Action) -> Void)?

  public init() {}

  // For mock use only
  internal init(
    suggestedCredentials: [any Credential] = [],
    allCredentials: [any Credential] = [],
    originHost: String? = nil,
    faviconAttributes: [String: FaviconAttributes] = [:],
    isAuthenticated: Bool = false,
    actionHandler: ((CredentialListModel.Action) -> Void)? = nil
  ) {
    self.suggestedCredentials = suggestedCredentials
    self.allCredentials = allCredentials
    self.originHost = originHost
    self.faviconAttributes = faviconAttributes
    self.isAuthenticated = isAuthenticated
    self.actionHandler = actionHandler
  }

  public enum Action {
    case selectedCredential(any Credential)
    case cancelled
  }

  public func populateFromStore(
    _ store: CredentialStore,
    identifiers: [ASCredentialServiceIdentifier]
  ) {
    self.allCredentials = store.credentials
    if let origin = identifiers.first?.identifier,
      let originURL = URL(string: origin)
    {
      self.originHost = URLOrigin(url: originURL).host
      // From credential_list_mediator.mm
      self.suggestedCredentials =
        allCredentials
        .filter { credential in
          if credential.serviceName != nil,
            origin.localizedStandardContains(credential.serviceName)
          {
            return true
          }
          if credential.serviceIdentifier != nil,
            origin.localizedStandardContains(credential.serviceIdentifier)
          {
            return true
          }
          return false
        }
        .sorted(using: KeyPathComparator(\.rank))
    }
  }

  public func loadFavicon(for credential: any Credential) {
    if faviconAttributes[credential.serviceIdentifier] != nil {
      return
    }
    CredentialProviderAPI.loadAttributes(for: credential) { [weak self] attributes in
      self?.faviconAttributes[credential.serviceIdentifier] = attributes
    }
  }
}
