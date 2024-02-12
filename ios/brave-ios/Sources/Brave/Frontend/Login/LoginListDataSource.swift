// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class LoginListDataSource {
    
  private let passwordAPI: BravePasswordAPI
  
  private(set) var credentialList = [PasswordForm]()
  private(set) var blockedList = [PasswordForm]()
  private var isCredentialsRefreshing = false
  
  var isCredentialsBeingSearched = false
  
  var isDataSourceEmpty: Bool {
    get {
      return credentialList.isEmpty && blockedList.isEmpty
    }
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
        return credentialList.isEmpty ? blockedList[safe: indexPath.item] : credentialList[safe: indexPath.item]
      case 1:
        return blockedList[safe: indexPath.item]
      default:
        return nil
      }
    } else {
      switch indexPath.section {
      case 1:
        return credentialList.isEmpty ? blockedList[safe: indexPath.item] : credentialList[safe: indexPath.item]
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
  
  private func reloadEntries(with query: String? = nil, passwordForms: [PasswordForm], completion: @escaping (Bool) -> Void) {
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
