/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_notes.h"

#include "base/check.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace email_aliases {

EmailAliasesNotes::EmailAliasesNotes(PrefService* pref_service,
                                     const std::string& primary_email)
    : pref_service_(pref_service), primary_email_(primary_email) {
  CHECK(pref_service_);
}

EmailAliasesNotes::~EmailAliasesNotes() = default;

// static
void EmailAliasesNotes::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(
      prefs::kEmailAliasesNotes,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

std::optional<std::string> EmailAliasesNotes::GetNote(
    const std::string& alias) {
  const auto& pref = pref_service_->GetDict(prefs::kEmailAliasesNotes);
  const auto* aliases = pref.FindDict(primary_email_);
  if (!aliases) {
    return std::nullopt;
  }

  if (const auto* note = aliases->FindString(alias)) {
    return *note;
  }
  return std::nullopt;
}

void EmailAliasesNotes::UpdateNote(const std::string& alias,
                                   const std::string& note) {
  ScopedDictPrefUpdate update(pref_service_.get(), prefs::kEmailAliasesNotes);
  auto& aliases = update.Get();
  if (note.empty()) {
    if (auto* notes = aliases.FindDict(primary_email_)) {
      notes->Remove(alias);
    }
  } else {
    auto* notes = aliases.EnsureDict(primary_email_);
    notes->Set(alias, note);
  }
}

void EmailAliasesNotes::RemoveNote(const std::string& alias) {
  UpdateNote(alias, {});
}

void EmailAliasesNotes::RemoveInactiveNotes(
    const std::vector<AliasListEntry>& active_aliases) {
  base::Value::Dict notes;
  for (const auto& alias : active_aliases) {
    if (alias.status != "active") {
      continue;
    }
    if (auto note = GetNote(alias.alias)) {
      notes.Set(alias.alias, std::move(note).value());
    }
  }
  // Set only notes for active aliases.
  ScopedDictPrefUpdate update(pref_service_.get(), prefs::kEmailAliasesNotes);
  if (notes.empty()) {
    update->Remove(primary_email_);
  } else {
    update->Set(primary_email_, std::move(notes));
  }
}

}  // namespace email_aliases
