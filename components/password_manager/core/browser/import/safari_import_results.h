// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_SAFARI_IMPORT_RESULTS_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_SAFARI_IMPORT_RESULTS_H_

#include <string>
#include <vector>

namespace password_manager {

struct SafariImportEntry {
  // Matches api::passwords_private::SafariImportEntryStatus.
  // Needs to be kept in sync with PasswordManagerSafariImportEntryStatus in
  // tools/metrics/histograms/enums.xml
  enum Status {
    // Should not be used.
    NONE = 0,
    // Any other error state.
    UNKNOWN_ERROR = 1,
    // Missing password field.
    MISSING_PASSWORD = 2,
    // Missing url field.
    MISSING_URL = 3,
    // Bad url formatting.
    INVALID_URL = 4,
    // Deprecated in crrev.com/c/4478954.
    // NON_ASCII_URL = 5,
    // URL is too long.
    LONG_URL = 6,
    // Password is too long.
    LONG_PASSWORD = 7,
    // Username is too long.
    LONG_USERNAME = 8,
    // Credential is already stored in profile store.
    CONFLICT_PROFILE = 9,
    // Credential is already stored in account store.
    CONFLICT_ACCOUNT = 10,
    // Note is too long.
    LONG_NOTE = 11,
    // Concatenation of imported and local notes is too long.
    LONG_CONCATENATED_NOTE = 12,
    // Valid credential.
    VALID = 13,
    kMaxValue = VALID
  };

  // The status of parsing for individual row that represents a credential
  // during import process.
  Status status = UNKNOWN_ERROR;

  // The url of the credential.
  std::string url;

  // The username of the credential.
  std::string username;

  // The password of the credential.
  std::string password;

  // Unique identifier of the credential.
  int id = 0;

  SafariImportEntry();
  SafariImportEntry(const SafariImportEntry& other);
  SafariImportEntry(SafariImportEntry&& other);
  ~SafariImportEntry();

  SafariImportEntry& operator=(const SafariImportEntry& entry);
  SafariImportEntry& operator=(SafariImportEntry&& entry);
};

struct SafariImportResults {
  // Matches api::passwords_private::SafariImportResultsStatus.
  enum Status {
    // Should not be used.
    NONE = 0,
    // Any other error state.
    UNKNOWN_ERROR = 1,
    // Data was fully or partially imported.
    SUCCESS = 2,
    // Failed to read provided file.
    IO_ERROR = 3,
    // Header is missing, invalid or could not be read.
    BAD_FORMAT = 4,
    // File selection dismissed.
    DISMISSED = 5,
    // Size of the chosen file exceeds the limit.
    MAX_FILE_SIZE = 6,
    // User has already started the import flow in a different window.
    IMPORT_ALREADY_ACTIVE = 7,
    // User tried to import too many passwords from one file.
    NUM_PASSWORDS_EXCEEDED = 8,
    // Conflicts found and they need to be resolved by the user.
    CONFLICTS = 9,
    kMaxValue = CONFLICTS
  };

  // General status of the triggered password import process.
  Status status = NONE;

  // Number of successfully imported passwords.
  size_t number_imported = 0;

  // Possibly empty, list of credentials that should be shown to the user.
  std::vector<SafariImportEntry> displayed_entries;

  // Possibly not set, name of file that user has chosen for the import.
  std::string file_name;

  SafariImportResults();
  SafariImportResults(const SafariImportResults& other);
  SafariImportResults(SafariImportResults&& other);
  ~SafariImportResults();

  SafariImportResults& operator=(const SafariImportResults& entry);
  SafariImportResults& operator=(SafariImportResults&& entry);
};

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_SAFARI_IMPORT_RESULTS_H_
