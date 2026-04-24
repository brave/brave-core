// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/os_crypt/sync/key_storage_libsecret.h"

#include "base/base64.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/os_crypt/sync/libsecret_util_linux.h"
#include "build/branding_buildflags.h"

namespace {
const char* GetApplicationName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-chrome")) {
    return "chrome";
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    return "chromium";
  } else {
    return "brave";
  }
}
}  // namespace

namespace {

const SecretSchema kKeystoreSchemaV2 = {
    "chrome_libsecret_os_crypt_password_v2",
    SECRET_SCHEMA_DONT_MATCH_NAME,
    {
        {"application", SECRET_SCHEMA_ATTRIBUTE_STRING},
        {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
    }};

// From a search result, extracts a SecretValue, asserting that there was at
// most one result. Returns nullptr if there are no results.
SecretValue* ToSingleSecret(GList* secret_items) {
  GList* first = g_list_first(secret_items);
  if (first == nullptr) {
    return nullptr;
  }
  if (g_list_next(first) != nullptr) {
    VLOG(1) << "OSCrypt found more than one encryption keys.";
  }
  SecretItem* secret_item = static_cast<SecretItem*>(first->data);
  SecretValue* secret_value =
      LibsecretLoader::secret_item_get_secret(secret_item);
  return secret_value;
}

// Checks the timestamps of the secret item and prints findings to logs. We
// presume that at most one secret item can be present.
void AnalyseKeyHistory(GList* secret_items) {
  GList* first = g_list_first(secret_items);
  if (first == nullptr) {
    return;
  }

  SecretItem* secret_item = static_cast<SecretItem*>(first->data);
  auto created = base::Time::FromTimeT(
      LibsecretLoader::secret_item_get_created(secret_item));
  auto last_modified = base::Time::FromTimeT(
      LibsecretLoader::secret_item_get_modified(secret_item));

  VLOG(1) << "Libsecret key created: " << created;
  VLOG(1) << "Libsecret key last modified: " << last_modified;
  LOG_IF(WARNING, created != last_modified)
      << "the encryption key has been modified since it was created.";
}

}  // namespace

KeyStorageLibsecret::KeyStorageLibsecret(std::string application_name)
    : application_name_(std::move(application_name)) {}

std::optional<std::string> KeyStorageLibsecret::AddRandomPasswordInLibsecret() {
  std::string password = base::Base64Encode(base::RandBytesAsVector(16));
  GError* error = nullptr;
  bool success = LibsecretLoader::secret_password_store_sync(
      &kKeystoreSchemaV2, nullptr, KeyStorageLinux::kKey, password.c_str(),
      nullptr, &error, "application", application_name_.c_str(), nullptr);
  if (error) {
    VLOG(1) << "Libsecret lookup failed: " << error->message;
    g_error_free(error);
    return std::nullopt;
  }
  if (!success) {
    VLOG(1) << "Libsecret lookup failed.";
    return std::nullopt;
  }

  VLOG(1) << "OSCrypt generated a new password.";
  return password;
}

std::optional<std::string> KeyStorageLibsecret::GetKeyImpl() {
  LibsecretAttributesBuilder attrs;
  attrs.Append("application", GetApplicationName());

  LibsecretLoader::SearchHelper helper;
  helper.Search(&kKeystoreSchemaV2, attrs.Get(),
                SECRET_SEARCH_UNLOCK | SECRET_SEARCH_LOAD_SECRETS);
  if (!helper.success()) {
    VLOG(1) << "Libsecret lookup failed: " << helper.error()->message;
    return std::nullopt;
  }

  SecretValue* password_libsecret = ToSingleSecret(helper.results());
  if (!password_libsecret) {
    return AddRandomPasswordInLibsecret();
  }
  AnalyseKeyHistory(helper.results());
  std::optional<std::string> password(
      LibsecretLoader::secret_value_get_text(password_libsecret));
  LibsecretLoader::secret_value_unref(password_libsecret);
  return password;
}

bool KeyStorageLibsecret::Init() {
  bool loaded = LibsecretLoader::EnsureLibsecretLoaded();
  if (loaded) {
    LibsecretLoader::EnsureKeyringUnlocked();
  }
  return loaded;
}
