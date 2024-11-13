// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_SAFARI_PASSWORD_IMPORTER_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_SAFARI_PASSWORD_IMPORTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "brave/components/password_manager/core/browser/import/safari_import_results.h"
#include "brave/components/password_manager/services/csv_password/public/mojom/csv_safari_password_parser.mojom.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/ui/credential_ui_entry.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace password_manager {

struct SafariNotesImportMetrics {
  size_t notes_per_file_count = 0;
  size_t notes_duplicates_per_file_count = 0;
  size_t notes_substrings_per_file_count = 0;
  size_t notes_concatenations_per_file_count = 0;
};

struct SafariIncomingPasswords {
  SafariIncomingPasswords();
  SafariIncomingPasswords(SafariIncomingPasswords&& other);
  ~SafariIncomingPasswords();

  SafariIncomingPasswords& operator=(SafariIncomingPasswords&& other);

  std::vector<password_manager::CredentialUIEntry> add_credentials;
  std::vector<password_manager::PasswordForm> edit_forms;
};

struct ConflictsResolutionCache;
class SavedPasswordsPresenter;

class SafariPasswordImporter {
 public:
  enum State {
    kNotStarted = 0,
    kInProgress = 1,
    kConflicts = 2,
    kFinished = 3,
  };

  using ConsumePasswordsCallback =
      mojom::CSVSafariPasswordParser::ParseCSVCallback;

  using SafariImportResultsCallback =
      base::OnceCallback<void(const SafariImportResults&)>;

  using DeleteFileCallback =
      base::RepeatingCallback<bool(const base::FilePath&)>;

  explicit SafariPasswordImporter(SavedPasswordsPresenter* presenter);
  SafariPasswordImporter(const SafariPasswordImporter&) = delete;
  SafariPasswordImporter& operator=(const SafariPasswordImporter&) = delete;
  ~SafariPasswordImporter();

  void Import(const base::FilePath& path,
              PasswordForm::Store to_store,
              SafariImportResultsCallback results_callback);

  void ContinueImport(const std::vector<int>& selected_ids,
                      SafariImportResultsCallback results_callback);

  void DeleteFile();

  bool IsState(State state) const { return state_ == state; }

  static std::vector<std::vector<base::FilePath::StringType>>
  GetSupportedFileExtensions();

 private:
  void ParseCSVSafariPasswordsInSandbox(
      PasswordForm::Store to_store,
      SafariImportResultsCallback results_callback,
      base::expected<std::string, SafariImportResults::Status> result);

  void ConsumePasswords(PasswordForm::Store to_store,
                        SafariImportResultsCallback results_callback,
                        mojom::CSVSafariPasswordSequencePtr seq);

  void ExecuteImport(SafariImportResultsCallback results_callback,
                     SafariImportResults results,
                     SafariIncomingPasswords incoming_passwords,
                     base::Time start_time,
                     size_t conflicts_count);

  void ImportFinished(SafariImportResultsCallback results_callback,
                      SafariImportResults results,
                      base::Time start_time,
                      size_t conflicts_count);

  const mojo::Remote<mojom::CSVSafariPasswordParser>& GetParser();

  mojo::Remote<mojom::CSVSafariPasswordParser> parser_;
  State state_ = State::kNotStarted;
  base::FilePath file_path_;
  std::unique_ptr<ConflictsResolutionCache> conflicts_cache_;

  DeleteFileCallback delete_function_;
  const raw_ptr<SavedPasswordsPresenter> presenter_;
  base::WeakPtrFactory<SafariPasswordImporter> weak_ptr_factory_{this};
};

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_SAFARI_PASSWORD_IMPORTER_H_
