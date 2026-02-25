/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_NOTES_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_NOTES_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/email_aliases/email_aliases_api.h"

class PrefRegistrySimple;
class PrefService;

namespace email_aliases {

class EmailAliasesNotes {
 public:
  EmailAliasesNotes(PrefService& pref_service,
                    const std::string& primary_email);
  ~EmailAliasesNotes();

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  std::optional<std::string> GetNote(const std::string& alias);
  void UpdateNote(const std::string& alias, const std::string& notes);
  void RemoveNote(const std::string& alias);
  void RemoveInactiveNotes(const std::vector<AliasListEntry>& active_aliases);

 private:
  const raw_ref<PrefService> pref_service_;
  const std::string primary_email_;
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_EMAIL_ALIASES_NOTES_H_
