// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

@Observable
class ManagedPasswordListViewModel {
  var credentialList: [PasswordForm] = []
  var blockedList: [PasswordForm] = []
  var isRefreshing: Bool = false

  //TODO: Use CWVAutoFillDataManager
  private let passwordAPI: BravePasswordAPI
  private var passwordStoreListener: PasswordStoreListener?
  private var searchTimer: Timer?
  private var isCredentialsBeingSearched = false
  private var isCredentialsRefreshing = false
  private var pendingSearchQuery: String?

  init(passwordAPI: BravePasswordAPI) {
    self.passwordAPI = passwordAPI

    // Adding the Password store observer to watch credentials changes
    passwordStoreListener = passwordAPI.add(
      PasswordStoreStateObserver { [weak self] _ in
        guard let self = self, !self.isCredentialsBeingSearched else {
          return
        }
        DispatchQueue.main.async {
          self.fetchCredentials()
        }
      }
    )
  }

  deinit {
    if let observer = passwordStoreListener {
      passwordAPI.removeObserver(observer)
    }
  }

  func fetchCredentials(_ searchQuery: String? = nil) {
    guard !isRefreshing else { return }
    isRefreshing = true
    fetchCredentials(searchQuery) { [weak self] _ in
      DispatchQueue.main.async {
        self?.isRefreshing = false
      }
    }
  }

  func fetchCredentials(_ searchQuery: String? = nil, completion: @escaping (Bool) -> Void) {
    if !isCredentialsRefreshing {
      isCredentialsRefreshing = true

      passwordAPI.getSavedLogins { [weak self] credentials in
        guard let self = self else { return }
        let queryToUse = self.pendingSearchQuery ?? searchQuery
        self.pendingSearchQuery = nil
        self.reloadEntries(with: queryToUse, passwordForms: credentials) { editEnabled in
          completion(editEnabled)
        }
      }
    }
  }
  private func reloadEntries(
    with query: String? = nil,
    passwordForms: [PasswordForm],
    completion: @escaping (Bool) -> Void
  ) {
    DispatchQueue.main.async { [self] in
      // Clear the blocklist before new items append
      blockedList.removeAll()

      if let query = query, !query.isEmpty {
        credentialList = passwordForms.filter { form in
          if let origin = form.url.origin.url?.absoluteString.lowercased(), origin.contains(query) {
            if form.isBlockedByUser {
              blockedList.append(form)
            }
            return !form.isBlockedByUser
          }

          if form.signOnRealm.lowercased().contains(query) {
            if form.isBlockedByUser {
              blockedList.append(form)
            }
            return !form.isBlockedByUser
          }

          if let username = form.usernameValue?.lowercased(), username.contains(query) {
            if form.isBlockedByUser {
              blockedList.append(form)
            }
            return !form.isBlockedByUser
          }

          return false
        }
      } else {
        credentialList = passwordForms.filter { form in
          // Check If the website is blocked by user with Never Save functionality
          if form.isBlockedByUser {
            blockedList.append(form)
          }

          return !form.isBlockedByUser
        }
      }
      self.isCredentialsRefreshing = false
      completion(true)
    }
  }

  func fetchPasswordFormFor(indexPath: IndexPath) -> PasswordForm? {
    if isCredentialsBeingSearched {
      switch indexPath.section {
      case 0:
        return credentialList.isEmpty
          ? blockedList[safe: indexPath.item] : credentialList[safe: indexPath.item]
      case 1:
        return blockedList[safe: indexPath.item]
      default:
        return nil
      }
    } else {
      switch indexPath.section {
      case 1:
        return credentialList.isEmpty
          ? blockedList[safe: indexPath.item] : credentialList[safe: indexPath.item]
      case 2:
        return blockedList[safe: indexPath.item]
      default:
        return nil
      }
    }
  }

  func performSearch(query: String) {
    searchTimer?.invalidate()
    pendingSearchQuery = query.isEmpty ? nil : query

    if query.isEmpty {
      isCredentialsBeingSearched = false
      fetchCredentials(nil)
      return
    }

    isCredentialsBeingSearched = true
    searchTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: false) { [weak self] _ in
      self?.fetchCredentials(query)
    }
  }

  func removeLogin(_ credential: PasswordForm) {
    passwordAPI.removeLogin(credential)
    fetchCredentials()
  }

  /// Removes all given credentials then refreshes the list once.
  func removeCredentials(_ credentials: [PasswordForm]) {
    guard !credentials.isEmpty else { return }
    for credential in credentials {
      passwordAPI.removeLogin(credential)
    }
    fetchCredentials()
  }

  /// Credentials grouped by base domain for the saved logins section (one row per site).
  var groupedCredentialList: [(domain: String, credentials: [PasswordForm])] {
    Self.groupCredentialsByDomain(credentialList)
  }

  /// Blocked credentials grouped by base domain for the never-saved section.
  var groupedBlockedList: [(domain: String, credentials: [PasswordForm])] {
    Self.groupCredentialsByDomain(blockedList)
  }

  static func groupCredentialsByDomain(
    _ credentials: [PasswordForm]
  ) -> [(domain: String, credentials: [PasswordForm])] {
    let grouped = Dictionary(
      grouping: credentials,
      by: { URL(string: $0.signOnRealm)?.baseDomain ?? "" }
    )
    return
      grouped
      .filter { !$0.key.isEmpty }
      .map { (domain: $0.key, credentials: $0.value) }
      .sorted { $0.domain.localizedCaseInsensitiveCompare($1.domain) == .orderedAscending }
  }
}
