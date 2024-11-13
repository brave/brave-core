// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/core/browser/import/safari_password_importer.h"

#include <map>
#include <optional>
#include <string>
#include <utility>

#include "base/barrier_closure.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "base/types/expected_macros.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password_sequence.h"
#include "brave/components/password_manager/services/csv_password/csv_safari_password_parser_impl.h"
#include "build/blink_buildflags.h"
#include "build/build_config.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/ui/credential_ui_entry.h"
#include "components/password_manager/core/browser/ui/credential_utils.h"
#include "components/password_manager/core/browser/ui/saved_passwords_presenter.h"
#include "components/password_manager/core/common/password_manager_constants.h"

#if BUILDFLAG(USE_BLINK)
#include "brave/components/password_manager/services/csv_password/csv_safari_password_parser_service.h"
#endif

namespace password_manager {

SafariIncomingPasswords::SafariIncomingPasswords() = default;
SafariIncomingPasswords::~SafariIncomingPasswords() = default;
SafariIncomingPasswords::SafariIncomingPasswords(
    SafariIncomingPasswords&& other) = default;
SafariIncomingPasswords& SafariIncomingPasswords::operator=(
    SafariIncomingPasswords&& other) = default;

struct ConflictsResolutionCache {
  ConflictsResolutionCache(
      SafariIncomingPasswords incoming_passwords,
      std::vector<std::vector<password_manager::PasswordForm>> conflicts,
      SafariImportResults results,
      base::Time start_time)
      : incoming_passwords(std::move(incoming_passwords)),
        conflicts(std::move(conflicts)),
        results(std::move(results)),
        start_time(start_time) {}
  ~ConflictsResolutionCache() = default;

  SafariIncomingPasswords incoming_passwords;
  std::vector<std::vector<password_manager::PasswordForm>> conflicts;
  SafariImportResults results;
  base::Time start_time;
};

namespace {

const base::FilePath::CharType kFileExtension[] = FILE_PATH_LITERAL("csv");
const int32_t kMaxFileSizeBytes = 150 * 1024;

base::expected<std::string, SafariImportResults::Status> ReadFileToString(
    const base::FilePath& path) {
  std::optional<int64_t> file_size = base::GetFileSize(path);

  if (file_size.has_value()) {
    if (file_size.value() > kMaxFileSizeBytes) {
      return base::unexpected(SafariImportResults::Status::MAX_FILE_SIZE);
    }
  }

  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return base::unexpected(SafariImportResults::Status::IO_ERROR);
  }

  return std::move(contents);
}

SafariImportEntry::Status GetConflictType(
    password_manager::PasswordForm::Store target_store) {
  switch (target_store) {
    case PasswordForm::Store::kProfileStore:
      return SafariImportEntry::Status::CONFLICT_PROFILE;
    case PasswordForm::Store::kAccountStore:
      return SafariImportEntry::Status::CONFLICT_ACCOUNT;
    case PasswordForm::Store::kNotSet:
      return SafariImportEntry::Status::UNKNOWN_ERROR;
    default:
      NOTREACHED();
  }
}

SafariImportEntry CreateFailedSafariImportEntry(
    const CredentialUIEntry& credential,
    const SafariImportEntry::Status status) {
  SafariImportEntry result;
  result.url = credential.GetAffiliatedDomains()[0].name;
  result.username = base::UTF16ToUTF8(credential.username);
  result.status = status;
  return result;
}

SafariImportEntry CreateValidSafariImportEntry(
    const CredentialUIEntry& credential,
    int id) {
  SafariImportEntry result;
  result.id = id;
  result.url = credential.GetAffiliatedDomains()[0].name;
  result.username = base::UTF16ToUTF8(credential.username);
  result.password = base::UTF16ToUTF8(credential.password);
  result.status = SafariImportEntry::VALID;
  return result;
}

base::expected<CredentialUIEntry, SafariImportEntry>
CSVSafariPasswordToCredentialUIEntry(
    const CSVSafariPassword& csv_safari_password,
    PasswordForm::Store store) {
  auto with_status = [&](SafariImportEntry::Status status) {
    SafariImportEntry entry;
    entry.status = status;
    // The raw URL is shown in the errors list in the UI to make it easier to
    // match the listed entry with the one in the CSV file.
    auto url = csv_safari_password.GetURL();
    entry.url = url.has_value() ? url.value().spec() : url.error();
    entry.username = csv_safari_password.GetUsername();
    return entry;
  };

  if (csv_safari_password.GetParseStatus() != CSVSafariPassword::Status::kOK) {
    return base::unexpected(
        with_status(SafariImportEntry::Status::UNKNOWN_ERROR));
  }

  const std::string& password = csv_safari_password.GetPassword();
  if (password.empty()) {
    return base::unexpected(
        with_status(SafariImportEntry::Status::MISSING_PASSWORD));
  }
  if (password.length() > 1000) {
    return base::unexpected(
        with_status(SafariImportEntry::Status::LONG_PASSWORD));
  }

  if (csv_safari_password.GetUsername().length() > 1000) {
    return base::unexpected(
        with_status(SafariImportEntry::Status::LONG_USERNAME));
  }

  if (csv_safari_password.GetNotes().length() > 1000) {
    return base::unexpected(with_status(SafariImportEntry::Status::LONG_NOTE));
  }

  ASSIGN_OR_RETURN(
      GURL url, csv_safari_password.GetURL(), [&](const std::string& error) {
        return with_status(error.empty()
                               ? SafariImportEntry::Status::MISSING_URL
                               : SafariImportEntry::Status::INVALID_URL);
      });
  if (url.spec().length() > 2048) {
    return base::unexpected(with_status(SafariImportEntry::Status::LONG_URL));
  }
  if (!IsValidPasswordURL(url)) {
    return base::unexpected(
        with_status(SafariImportEntry::Status::INVALID_URL));
  }

  auto credential = CSVPassword(
      url, csv_safari_password.GetUsername(), password,
      csv_safari_password.GetNotes(),
      static_cast<CSVPassword::Status>(csv_safari_password.GetParseStatus()));
  return CredentialUIEntry(credential, store);
}

std::optional<CredentialUIEntry> GetConflictingCredential(
    const std::map<std::u16string, std::vector<CredentialUIEntry>>&
        credentials_by_username,
    const CredentialUIEntry& imported_credential) {
  auto it = credentials_by_username.find(imported_credential.username);
  if (it != credentials_by_username.end()) {
    // Iterate over all local credentials with matching username.
    for (const CredentialUIEntry& local_credential : it->second) {
      // Check if `local_credential` has matching `signon_realm`, but different
      // `password`.
      if (local_credential.password != imported_credential.password &&
          base::ranges::any_of(
              local_credential.facets,
              [&imported_credential](const CredentialFacet& facet) {
                return facet.signon_realm ==
                       imported_credential.facets[0].signon_realm;
              })) {
        return local_credential;
      }
    }
  }
  return std::nullopt;
}

std::vector<PasswordForm> GetMatchingPasswordForms(
    SavedPasswordsPresenter* presenter,
    const CredentialUIEntry& credential,
    PasswordForm::Store store) {
  // Returns matching local forms for a given `credential`, excluding grouped
  // forms with different `signon_realm`.
  CHECK(presenter);
  std::vector<PasswordForm> results;
  base::ranges::copy_if(
      presenter->GetCorrespondingPasswordForms(credential),
      std::back_inserter(results), [&](const PasswordForm& form) {
        return form.signon_realm == credential.GetFirstSignonRealm() &&
               store == form.in_store;
      });
  return results;
}

std::u16string ComputeNotesConcatenation(const std::u16string& local_note,
                                         const std::u16string& imported_note,
                                         SafariNotesImportMetrics& metrics) {
  CHECK_LE(imported_note.size(),
           static_cast<unsigned>(constants::kMaxPasswordNoteLength));

  if (imported_note.empty()) {
    return local_note;
  }

  if (local_note.empty()) {
    return imported_note;
  }

  if (local_note == imported_note) {
    metrics.notes_duplicates_per_file_count++;
    return local_note;
  }

  if (local_note.find(imported_note) != std::u16string::npos) {
    metrics.notes_substrings_per_file_count++;
    return local_note;
  }

  return base::JoinString(/*parts=*/{local_note, imported_note}, u"\n");
}

void MergeNotesOrReportError(const std::vector<PasswordForm>& local_forms,
                             const CredentialUIEntry& imported_credential,
                             SafariImportResults& results,
                             std::vector<PasswordForm>& edit_forms,
                             SafariNotesImportMetrics& metrics) {
  const std::u16string local_note = CredentialUIEntry(local_forms).note;
  const std::u16string& imported_note = imported_credential.note;
  const std::u16string concatenation =
      ComputeNotesConcatenation(local_note, imported_note, metrics);

  if (concatenation.size() > constants::kMaxPasswordNoteLength) {
    // Notes concatenation size should not exceed 1000 characters.
    results.displayed_entries.push_back(CreateFailedSafariImportEntry(
        imported_credential,
        SafariImportEntry::Status::LONG_CONCATENATED_NOTE));
    return;
  }

  if (concatenation != local_note) {
    // Local credential needs to be updated with concatenation.
    for (PasswordForm form : local_forms) {
      form.SetNoteWithEmptyUniqueDisplayName(concatenation);
      edit_forms.emplace_back(std::move(form));
    }
    metrics.notes_concatenations_per_file_count++;
  }

  results.number_imported++;
}

bool DefaultDeleteFunction(const base::FilePath& file) {
  return base::DeleteFile(file);
}

void ProcessParsedCredential(
    const CredentialUIEntry& imported_credential,
    SavedPasswordsPresenter* presenter,
    const std::map<std::u16string, std::vector<CredentialUIEntry>>&
        credentials_by_username,
    PasswordForm::Store to_store,
    SafariIncomingPasswords& incoming_passwords,
    std::vector<std::vector<PasswordForm>>& conflicts,
    SafariImportResults& results,
    SafariNotesImportMetrics& notes_metrics,
    size_t& duplicates_count) {
  if (!imported_credential.note.empty()) {
    notes_metrics.notes_per_file_count++;
  }

  // Check if there are local credentials with the same signon_realm and
  // username, but different password. Such credentials are considered
  // conflicts.
  std::optional<CredentialUIEntry> conflicting_credential =
      GetConflictingCredential(credentials_by_username, imported_credential);
  if (conflicting_credential.has_value()) {
    std::vector<PasswordForm> forms = GetMatchingPasswordForms(
        presenter, conflicting_credential.value(), to_store);
    // Password notes are not taken into account when conflicting passwords
    // are overwritten. Only the local note is persisted.
    for (PasswordForm& form : forms) {
      form.password_value = imported_credential.password;
    }
    conflicts.push_back(std::move(forms));
    return;
  }

  // Check for duplicates.
  std::vector<PasswordForm> forms =
      GetMatchingPasswordForms(presenter, imported_credential, to_store);
  if (!forms.empty()) {
    duplicates_count++;

    if (imported_credential.note.empty()) {
      // Duplicates are reported as successfully imported credentials.
      results.number_imported++;
      return;
    }

    MergeNotesOrReportError(
        /*local_forms=*/forms, /*imported_credential=*/imported_credential,
        /*results=*/results, /*edit_forms=*/incoming_passwords.edit_forms,
        /*metrics=*/notes_metrics);
    return;
  }

  // Valid credential with no conflicts and no duplicates.
  incoming_passwords.add_credentials.push_back(imported_credential);
}

}  // namespace

SafariPasswordImporter::SafariPasswordImporter(
    SavedPasswordsPresenter* presenter)
    : delete_function_(base::BindRepeating(&DefaultDeleteFunction)),
      presenter_(presenter) {}

SafariPasswordImporter::~SafariPasswordImporter() = default;

const mojo::Remote<mojom::CSVSafariPasswordParser>&
SafariPasswordImporter::GetParser() {
  if (!parser_) {
#if BUILDFLAG(USE_BLINK)
    parser_ = LaunchCSVSafariPasswordParser();
#else
    mojo::PendingReceiver<mojom::CSVSafariPasswordParser> receiver =
        parser_.BindNewPipeAndPassReceiver();

    // Instantiate the implementation and bind it to the receiver
    new CSVSafariPasswordParserImpl(std::move(receiver));
#endif

    // Ensure the remote resets itself on disconnect
    parser_.reset_on_disconnect();
  }
  return parser_;
}

void SafariPasswordImporter::ParseCSVSafariPasswordsInSandbox(
    PasswordForm::Store to_store,
    SafariImportResultsCallback results_callback,
    base::expected<std::string, SafariImportResults::Status> result) {
  // Currently, CSV is the only supported format.
  if (result.has_value()) {
    GetParser()->ParseCSV(
        std::move(result.value()),
        base::BindOnce(&SafariPasswordImporter::ConsumePasswords,
                       weak_ptr_factory_.GetWeakPtr(), to_store,
                       std::move(results_callback)));
  } else {
    SafariImportResults results;
    results.status = result.error();
    // Importer is reset to the initial state, due to the error.
    state_ = State::kNotStarted;
    std::move(results_callback).Run(std::move(results));
  }
}

void SafariPasswordImporter::Import(
    const base::FilePath& path,
    PasswordForm::Store to_store,
    SafariImportResultsCallback results_callback) {
  // Blocks concurrent import requests.
  state_ = State::kInProgress;
  file_path_ = path;

  // Posting with USER_VISIBLE priority, because the result of the import is
  // visible to the user in the password settings page.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE, base::MayBlock()},
      base::BindOnce(&ReadFileToString, path),
      base::BindOnce(&SafariPasswordImporter::ParseCSVSafariPasswordsInSandbox,
                     weak_ptr_factory_.GetWeakPtr(), to_store,
                     std::move(results_callback)));
}

void SafariPasswordImporter::ContinueImport(
    const std::vector<int>& selected_ids,
    SafariImportResultsCallback results_callback) {
  CHECK(IsState(State::kConflicts));
  CHECK(conflicts_cache_);
  // Blocks concurrent import requests, when switching from `kConflicts` state.
  state_ = State::kInProgress;

  for (int id : selected_ids) {
    conflicts_cache_->results.number_imported++;
    CHECK_LT(static_cast<size_t>(id), conflicts_cache_->conflicts.size());
    for (const PasswordForm& form : conflicts_cache_->conflicts[id]) {
      conflicts_cache_->incoming_passwords.edit_forms.push_back(form);
    }
  }

  ExecuteImport(
      std::move(results_callback), std::move(conflicts_cache_->results),
      std::move(conflicts_cache_->incoming_passwords),
      conflicts_cache_->start_time, conflicts_cache_->conflicts.size());

  conflicts_cache_.reset();
}

void SafariPasswordImporter::ConsumePasswords(
    PasswordForm::Store to_store,
    SafariImportResultsCallback results_callback,
    mojom::CSVSafariPasswordSequencePtr seq) {
  // Used to aggregate final results of the current import.
  SafariImportResults results;
  results.file_name = file_path_.BaseName().AsUTF8Unsafe();
  CHECK_EQ(results.number_imported, 0u);

  if (!seq) {
    // A nullptr returned by the parser means a bad format.
    results.status = SafariImportResults::Status::BAD_FORMAT;
    // Importer is reset to the initial state, due to the error.
    state_ = State::kNotStarted;
    std::move(results_callback).Run(std::move(results));
    return;
  }
  if (seq->csv_passwords.size() > constants::kMaxPasswordsPerCSVFile) {
    results.status = SafariImportResults::Status::NUM_PASSWORDS_EXCEEDED;

    // Importer is reset to the initial state, due to the error.
    state_ = State::kNotStarted;
    std::move(results_callback).Run(results);
    return;
  }

  // TODO(crbug.com/40225420): Either move to earlier point or update histogram.
  base::Time start_time = base::Time::Now();
  // Used to compute conflicts and duplicates.
  std::map<std::u16string, std::vector<CredentialUIEntry>>
      credentials_by_username;
  for (const CredentialUIEntry& credential : presenter_->GetSavedPasswords()) {
    // Don't consider credentials from a store other than the target store.
    if (credential.stored_in.contains(to_store)) {
      credentials_by_username[credential.username].push_back(credential);
    }
  }

  SafariNotesImportMetrics notes_metrics;
  size_t duplicates_count = 0;  // Number of duplicates per imported file.

  // Aggregate all passwords that might need to be added or updated.
  SafariIncomingPasswords incoming_passwords;

  // Conflicting credential that could be updated. Each nested vector
  // represents one credential, i.e. all PasswordForm's in such a vector have
  // the same signon_ream, username, password.
  std::vector<std::vector<PasswordForm>> conflicts;

  // Go over all canonically parsed passwords:
  // 1) aggregate all valid ones in `incoming_passwords` to be passed over to
  // the presenter. 2) aggregate all parsing errors in the `results`.
  for (const CSVSafariPassword& csv_safari_password : seq->csv_passwords) {
    base::expected<CredentialUIEntry, SafariImportEntry> credential =
        CSVSafariPasswordToCredentialUIEntry(csv_safari_password, to_store);

    if (!credential.has_value()) {
      results.displayed_entries.emplace_back(std::move(credential.error()));
      continue;
    }

    ProcessParsedCredential(credential.value(), presenter_,
                            credentials_by_username, to_store,
                            incoming_passwords, conflicts, results,
                            notes_metrics, duplicates_count);
  }

  results.number_imported += incoming_passwords.add_credentials.size();

  if (conflicts.empty()) {
    for (const std::vector<PasswordForm>& forms : conflicts) {
      results.displayed_entries.push_back(CreateFailedSafariImportEntry(
          CredentialUIEntry(forms), GetConflictType(to_store)));
    }

    ExecuteImport(std::move(results_callback), std::move(results),
                  std::move(incoming_passwords), start_time, conflicts.size());
    return;
  }

  state_ = State::kConflicts;
  SafariImportResults conflicts_results;
  conflicts_results.status = SafariImportResults::CONFLICTS;
  for (size_t idx = 0; idx < conflicts.size(); idx++) {
    conflicts_results.displayed_entries.push_back(
        CreateValidSafariImportEntry(CredentialUIEntry(conflicts[idx]), idx));
  }

  conflicts_cache_ = std::make_unique<ConflictsResolutionCache>(
      std::move(incoming_passwords), std::move(conflicts), std::move(results),
      start_time);

  std::move(results_callback).Run(std::move(conflicts_results));
}

void SafariPasswordImporter::ExecuteImport(
    SafariImportResultsCallback results_callback,
    SafariImportResults results,
    SafariIncomingPasswords incoming_passwords,
    base::Time start_time,
    size_t conflicts_count) {
  // Run `results_callback` when both `AddCredentials` and
  // `UpdatePasswordForms` have finished running.
  auto barrier_done_callback = base::BarrierClosure(
      2, base::BindOnce(base::BindOnce(
             &SafariPasswordImporter::ImportFinished,
             weak_ptr_factory_.GetWeakPtr(), std::move(results_callback),
             std::move(results), start_time, conflicts_count)));

  presenter_->AddCredentials(incoming_passwords.add_credentials,
                             PasswordForm::Type::kImported,
                             barrier_done_callback);
  presenter_->UpdatePasswordForms(incoming_passwords.edit_forms,
                                  barrier_done_callback);
}

void SafariPasswordImporter::ImportFinished(
    SafariImportResultsCallback results_callback,
    SafariImportResults results,
    base::Time start_time,
    size_t conflicts_count) {
  if (results.displayed_entries.empty()) {
    // After successful import with no errors, the user has an option to delete
    // the imported file.
    state_ = State::kFinished;
  } else {
    // After successful import with some errors, the importer is reset to the
    // initial state.
    state_ = State::kNotStarted;
  }

  results.status = SafariImportResults::Status::SUCCESS;
  std::move(results_callback).Run(std::move(results));
}

void SafariPasswordImporter::DeleteFile() {
  CHECK(IsState(State::kFinished));
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(base::IgnoreResult(delete_function_), file_path_));
}

// static
std::vector<std::vector<base::FilePath::StringType>>
SafariPasswordImporter::GetSupportedFileExtensions() {
  return std::vector<std::vector<base::FilePath::StringType>>(
      1, std::vector<base::FilePath::StringType>(1, kFileExtension));
}

}  // namespace password_manager
