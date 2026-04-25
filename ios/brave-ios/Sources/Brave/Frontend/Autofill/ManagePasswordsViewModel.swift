// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

@MainActor
@Observable
class ManagePasswordsViewModel {
  var isFetching: Bool = false
  var searchText: String = ""
  private(set) var allowedGroups: [(domain: String, credentials: [CWVPassword])] = []
  private(set) var blockedGroups: [(domain: String, credentials: [CWVPassword])] = []

  /// Filtered view of `allowedGroups` for display. Never mutates the source of truth.
  var filteredAllowedGroups: [(domain: String, credentials: [CWVPassword])] {
    guard !searchText.isEmpty else { return allowedGroups }
    let query = searchText
    return allowedGroups.filter { group in
      group.domain.localizedCaseInsensitiveContains(query)
        || group.credentials.contains {
          ($0.username ?? "").localizedCaseInsensitiveContains(query)
        }
    }
  }

  /// Filtered view of `blockedGroups` for display. Never mutates the source of truth.
  var filteredBlockedGroups: [(domain: String, credentials: [CWVPassword])] {
    guard !searchText.isEmpty else { return blockedGroups }
    let query = searchText
    return blockedGroups.filter { group in
      group.domain.localizedCaseInsensitiveContains(query)
        || group.credentials.contains {
          ($0.username ?? "").localizedCaseInsensitiveContains(query)
        }
    }
  }

  private let autofillDataManager: CWVAutofillDataManager
  private let observer: AutofillDataManagerObserver

  init(autofillDataManager: CWVAutofillDataManager) {
    self.autofillDataManager = autofillDataManager

    observer = AutofillDataManagerObserver()
    autofillDataManager.add(observer)
    observer.notify = { [weak self] in
      self?.fetchPasswords()
    }

    fetchPasswords()
  }

  deinit {
    autofillDataManager.remove(observer)
  }

  private func updateGroups(allowed: [CWVPassword], blocked: [CWVPassword]) {
    allowedGroups = allowed.groupedByDomain()
    blockedGroups = blocked.groupedByDomain()
  }

  func fetchPasswords() {
    // We simply need to track that a fetch is in flight for the purpose of updating the UI or caller
    // and then make a fetch request. Chromium’s CWVAutofillDataManager implementation handles re-entrancy/debouncing.
    // So multiple rapid calls to fetchPasswords only trigger one underlying fetch, and all callers receive the same result.
    isFetching = true
    autofillDataManager.fetchPasswords { [weak self] passwords in
      guard let self else { return }
      Task { @MainActor in
        self.updateGroups(
          allowed: passwords.filter { !$0.isBlocked },
          blocked: passwords.filter { $0.isBlocked }
        )
        self.isFetching = false
      }
    }
  }

  func deletePasswords(_ credentials: [CWVPassword]) {
    for credential in credentials {
      autofillDataManager.delete(credential)
    }
  }

  func deletePasswords(forGroupIds groupIds: Set<GroupID>) {
    // Snapshot the current groups before deletion to avoid operating on
    // potentially stale data if a fetch completes mid-deletion.
    let allowedSnapshot = allowedGroups
    let blockedSnapshot = blockedGroups
    let toDelete = groupIds.flatMap { groupId -> [CWVPassword] in
      let (groups, domain): ([(domain: String, credentials: [CWVPassword])], String) =
        switch groupId {
        case .saved(let d): (allowedSnapshot, d)
        case .blocked(let d): (blockedSnapshot, d)
        }
      return groups.first { $0.domain == domain }?.credentials ?? []
    }
    deletePasswords(toDelete)
  }
}

extension ManagePasswordsViewModel {
  /// A typed identifier for a single domain group as it appears in the UI, encoding both the
  /// domain name and which list the group belongs to. This distinction matters because the same
  /// domain can appear independently in both the saved and blocked lists, so a plain domain string
  /// would be ambiguous as an identifier.
  enum GroupID: Hashable {
    case saved(domain: String)
    case blocked(domain: String)

    /// The domain string regardless of which list this group belongs to. Useful at the call site
    /// when only the display label is needed and list membership is irrelevant.
    var domain: String {
      switch self {
      case .saved(let domain), .blocked(let domain): return domain
      }
    }
  }
}

extension Array where Element == CWVPassword {
  /// Returns an alphabetically sorted list of (domain, credentials) tuples, where each tuple
  /// represents a base domain and all passwords associated with it. For example, credentials for
  /// `accounts.google.com` and `mail.google.com` will both appear under the single key `"google.com"`.
  ///
  /// Passwords whose `site` cannot be parsed into a valid URL, or whose URL yields no base domain,
  /// are silently excluded rather than grouped under a catch-all key. This prevents malformed or
  /// internal entries from surfacing in the UI.
  ///
  /// The sort uses `localizedCaseInsensitiveCompare` so that ordering respects the user's locale
  /// (e.g. accented characters sort naturally) and is case-insensitive. Ordering of credentials
  /// within each group is not guaranteed — callers should apply their own sort if display order
  /// within a domain matters.
  func groupedByDomain() -> [(domain: String, credentials: [CWVPassword])] {
    let grouped = Dictionary(
      grouping: self,
      by: { URL(string: $0.site)?.baseDomain ?? "" }
    )
    return
      grouped
      .filter { !$0.key.isEmpty }
      .map { (domain: $0.key, credentials: $0.value) }
      .sorted { $0.domain.localizedCaseInsensitiveCompare($1.domain) == .orderedAscending }
  }
}

/// A thin bridge between `CWVAutofillDataManagerObserver` and the view model. Both delegate
/// methods funnel into a single `notify` closure, decoupling the observer from the view model
/// and avoiding the need for a direct reference back to it.
private class AutofillDataManagerObserver: NSObject, CWVAutofillDataManagerObserver {
  /// Called whenever passwords change. Expected to trigger a fresh password fetch, with results delivered back on the main thread.
  var notify: () -> Void = {}

  /// Protocol conformance: Triggered when any autofill data changes, not limited to passwords (e.g. credit cards, addresses).
  func autofillDataManagerDataDidChange(_ autofillDataManager: CWVAutofillDataManager) {
    notify()
  }

  /// Protocol conformance: Triggered with a breakdown of which passwords were added, updated, or removed.
  /// Both this and `autofillDataManagerDataDidChange` may fire for the same change event,
  /// so `notify` is intentionally idempotent — re-fetching all passwords each time is safe.
  func autofillDataManager(
    _ autofillDataManager: CWVAutofillDataManager,
    didChangePasswordsByAdding added: [CWVPassword],
    updating updated: [CWVPassword],
    removing removed: [CWVPassword]
  ) {
    notify()
  }
}
