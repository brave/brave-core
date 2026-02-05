// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

class LoginListDataSource {

  private let passwordAPI: BravePasswordAPI

  private(set) var credentialList = [PasswordForm]()
  private(set) var blockedList = [PasswordForm]()
  private var isCredentialsRefreshing = false

  var isCredentialsBeingSearched = false

  var isDataSourceEmpty: Bool {
    return credentialList.isEmpty && blockedList.isEmpty
  }

  // MARK: Internal

  init(with passwordAPI: BravePasswordAPI) {
    self.passwordAPI = passwordAPI
  }

  func fetchLoginInfo(_ searchQuery: String? = nil, completion: @escaping (Bool) -> Void) {
    if !isCredentialsRefreshing {
      isCredentialsRefreshing = true

      passwordAPI.getSavedLogins { credentials in
        self.reloadEntries(with: searchQuery, passwordForms: credentials) { editEnabled in
          completion(editEnabled)
        }
      }
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

  func fetchNumberOfSections() -> Int {
    // Option - Saved Logins - Never Saved
    var sectionCount = 3

    if blockedList.isEmpty {
      sectionCount -= 1
    }

    if credentialList.isEmpty {
      sectionCount -= 1
    }

    return isCredentialsBeingSearched ? sectionCount - 1 : sectionCount
  }

  func fetchNumberOfRowsInSection(section: Int) -> Int {
    switch section {
    case 0:
      if !isCredentialsBeingSearched {
        return 1
      }

      return credentialList.isEmpty ? blockedList.count : credentialList.count
    case 1:
      if !isCredentialsBeingSearched {
        return credentialList.isEmpty ? blockedList.count : credentialList.count
      }

      return blockedList.count
    case 2:
      return isCredentialsBeingSearched ? 0 : blockedList.count
    default:
      return 0
    }
  }

  // MARK: Private

  private func reloadEntries(
    with query: String? = nil,
    passwordForms: [PasswordForm],
    completion: @escaping (Bool) -> Void
  ) {
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

    DispatchQueue.main.async {
      self.isCredentialsRefreshing = false
      completion(!self.credentialList.isEmpty)
    }
  }
}

@Observable
class LoginListViewModel {
  var credentialList: [PasswordForm] = []
  var blockedList: [PasswordForm] = []
  var isRefreshing: Bool = false

  private let passwordAPI: BravePasswordAPI
  private let dataSource: LoginListDataSource
  private var passwordStoreListener: PasswordStoreListener?
  private var searchTimer: Timer?

  init(passwordAPI: BravePasswordAPI) {
    self.passwordAPI = passwordAPI
    self.dataSource = LoginListDataSource(with: passwordAPI)

    // Adding the Password store observer to watch credentials changes
    passwordStoreListener = passwordAPI.add(
      PasswordStoreStateObserver { [weak self] _ in
        guard let self = self, !self.dataSource.isCredentialsBeingSearched else {
          return
        }
        DispatchQueue.main.async {
          self.fetchLoginInfo()
        }
      }
    )
  }

  deinit {
    if let observer = passwordStoreListener {
      passwordAPI.removeObserver(observer)
    }
  }

  func fetchLoginInfo(_ searchQuery: String? = nil) {
    guard !isRefreshing else { return }
    isRefreshing = true
    dataSource.fetchLoginInfo(searchQuery) { [weak self] _ in
      DispatchQueue.main.async {
        self?.credentialList = self?.dataSource.credentialList ?? []
        self?.blockedList = self?.dataSource.blockedList ?? []
        self?.isRefreshing = false
      }
    }
  }

  func performSearch(query: String) {
    searchTimer?.invalidate()

    if query.isEmpty {
      dataSource.isCredentialsBeingSearched = false
      fetchLoginInfo(nil)
      return
    }

    dataSource.isCredentialsBeingSearched = true
    searchTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: false) { [weak self] _ in
      self?.fetchLoginInfo(query)
    }
  }

  func removeLogin(_ credential: PasswordForm) {
    passwordAPI.removeLogin(credential)
    fetchLoginInfo()
  }
}
