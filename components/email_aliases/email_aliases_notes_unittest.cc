/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_notes.h"

#include "base/values.h"
#include "brave/components/email_aliases/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace email_aliases {

namespace {
struct AliasNote {
  std::string alias;
  std::string note;
};

}  // namespace

class EmailAliasesNotesTest : public ::testing::Test {
 public:
  EmailAliasesNotesTest() {
    EmailAliasesNotes::RegisterProfilePrefs(prefs_.registry());
  }
  ~EmailAliasesNotesTest() override = default;

  PrefService& GetPrefs() { return prefs_; }

  base::DictValue CreateNotes(const std::vector<AliasNote>& notes) {
    base::DictValue v;
    for (const auto& note : notes) {
      v.Set(note.alias, note.note);
    }
    return v;
  }

  std::vector<AliasListEntry> CreateAliases(
      const std::vector<std::string>& aliases) {
    std::vector<AliasListEntry> r;
    for (const auto& a : aliases) {
      AliasListEntry e;
      e.alias = a;
      e.status = "active";
      r.push_back(std::move(e));
    }
    return r;
  }

 private:
  TestingPrefServiceSimple prefs_;
};

TEST_F(EmailAliasesNotesTest, InitAndRemoveInactive) {
  base::DictValue notes;
  notes.Set("a@prima.ry", CreateNotes({{"alias1", "note1"},
                                       {"alias2", "note2"},
                                       {"alias3", "note3"}}));
  notes.Set("b@prima.ry", CreateNotes({{"alias1", "note1"},
                                       {"alias2", "note2"},
                                       {"alias3", "note3"}}));
  notes.Set("c@prima.ry", CreateNotes({{"alias1", "note1"},
                                       {"alias2", "note2"},
                                       {"alias3", "note3"}}));
  GetPrefs().SetDict(prefs::kEmailAliasesNotes, std::move(notes));

  {
    EmailAliasesNotes email_aliases_notes(GetPrefs(), "a@prima.ry");
    // No active aliases for `a@primay.ry`
    email_aliases_notes.RemoveInactiveNotes({});
    EXPECT_EQ(2u, GetPrefs().GetDict(prefs::kEmailAliasesNotes).size());
    EXPECT_EQ(std::nullopt, email_aliases_notes.GetNote("alias1"));
  }

  {
    EmailAliasesNotes email_aliases_notes(GetPrefs(), "b@prima.ry");
    email_aliases_notes.RemoveInactiveNotes(
        CreateAliases({"alias1", "alias3"}));
    EXPECT_EQ(2u, GetPrefs().GetDict(prefs::kEmailAliasesNotes).size());
    EXPECT_EQ(2u, GetPrefs()
                      .GetDict(prefs::kEmailAliasesNotes)
                      .FindDict("b@prima.ry")
                      ->size());
    EXPECT_EQ("note1", email_aliases_notes.GetNote("alias1"));
    EXPECT_EQ(std::nullopt, email_aliases_notes.GetNote("alias2"));
    EXPECT_EQ("note3", email_aliases_notes.GetNote("alias3"));
  }
  {
    EmailAliasesNotes email_aliases_notes(GetPrefs(), "c@prima.ry");
    email_aliases_notes.RemoveInactiveNotes(
        CreateAliases({"alias1", "alias2", "alias3"}));
    EXPECT_EQ(2u, GetPrefs().GetDict(prefs::kEmailAliasesNotes).size());
    EXPECT_EQ(3u, GetPrefs()
                      .GetDict(prefs::kEmailAliasesNotes)
                      .FindDict("c@prima.ry")
                      ->size());
    EXPECT_EQ("note1", email_aliases_notes.GetNote("alias1"));
    EXPECT_EQ("note2", email_aliases_notes.GetNote("alias2"));
    EXPECT_EQ("note3", email_aliases_notes.GetNote("alias3"));
  }
}

TEST_F(EmailAliasesNotesTest, Update) {
  base::DictValue notes;
  notes.Set("a@prima.ry", CreateNotes({{"alias1", "note1"},
                                       {"alias2", "note2"},
                                       {"alias3", "note3"}}));
  GetPrefs().SetDict(prefs::kEmailAliasesNotes, std::move(notes));

  EmailAliasesNotes email_aliases_notes(GetPrefs(), "a@prima.ry");

  email_aliases_notes.UpdateNote("alias1", "updated1");
  email_aliases_notes.UpdateNote("alias2", "updated2");
  email_aliases_notes.UpdateNote("alias3", "updated3");
  email_aliases_notes.UpdateNote("alias4", "updated4");

  EXPECT_EQ("updated1", email_aliases_notes.GetNote("alias1"));
  EXPECT_EQ("updated2", email_aliases_notes.GetNote("alias2"));
  EXPECT_EQ("updated3", email_aliases_notes.GetNote("alias3"));
  EXPECT_EQ("updated4", email_aliases_notes.GetNote("alias4"));
}

TEST_F(EmailAliasesNotesTest, Remove) {
  base::DictValue notes;
  notes.Set("a@prima.ry", CreateNotes({{"alias1", "note1"},
                                       {"alias2", "note2"},
                                       {"alias3", "note3"}}));
  GetPrefs().SetDict(prefs::kEmailAliasesNotes, std::move(notes));

  EmailAliasesNotes email_aliases_notes(GetPrefs(), "a@prima.ry");

  email_aliases_notes.RemoveNote("alias1");
  EXPECT_EQ(std::nullopt, email_aliases_notes.GetNote("alias1"));
  email_aliases_notes.RemoveNote("alias2");
  EXPECT_EQ(std::nullopt, email_aliases_notes.GetNote("alias2"));
  email_aliases_notes.RemoveNote("alias3");
  EXPECT_EQ(std::nullopt, email_aliases_notes.GetNote("alias3"));
}

}  // namespace email_aliases
