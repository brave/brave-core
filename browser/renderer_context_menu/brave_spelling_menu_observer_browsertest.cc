/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/renderer_context_menu/spelling_menu_observer.h"

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/renderer_context_menu/brave_mock_render_view_context_menu.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/spellcheck/browser/pref_names.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A test class used in this file. This test should be a browser test because it
// accesses resources.
class BraveSpellingMenuObserverTest : public InProcessBrowserTest {
 public:
  BraveSpellingMenuObserverTest();
  BraveSpellingMenuObserverTest(const BraveSpellingMenuObserverTest&) = delete;
  BraveSpellingMenuObserverTest& operator=(
      const BraveSpellingMenuObserverTest&) = delete;

  void SetUpOnMainThread() override {}

  void TearDownOnMainThread() override {
    observer_.reset();
    menu_.reset();
  }

  void Reset(bool incognito = false) {
    observer_.reset();
    menu_.reset(new BraveMockRenderViewContextMenu(
        incognito ? browser()->profile()->GetPrimaryOTRProfile(
                        /*create_if_needed=*/true)
                  : browser()->profile()));
    observer_.reset(new SpellingMenuObserver(menu_.get()));
    menu_->SetObserver(observer_.get());
    // Uncomment to print the menu to standard output for each test.
    // menu_->EnablePrintMenu();
  }

  void InitMenu(const char* word, const char* suggestion) {
    content::ContextMenuParams params;
    params.is_editable = true;
    params.misspelled_word = base::ASCIIToUTF16(word);
    params.dictionary_suggestions.clear();
    if (suggestion)
      params.dictionary_suggestions.push_back(base::ASCIIToUTF16(suggestion));
    observer_->InitMenu(params);
  }

  void CheckExpected() {
    for (size_t i = 0; i < menu()->GetMenuSize(); ++i) {
      BraveMockRenderViewContextMenu::MockMenuItem item;
      menu()->GetMenuItem(i, &item);
      EXPECT_NE(IDC_CONTENT_CONTEXT_SPELLING_TOGGLE, item.command_id);
    }
  }

  ~BraveSpellingMenuObserverTest() override;
  BraveMockRenderViewContextMenu* menu() { return menu_.get(); }
  SpellingMenuObserver* observer() { return observer_.get(); }

 private:
  std::unique_ptr<SpellingMenuObserver> observer_;
  std::unique_ptr<BraveMockRenderViewContextMenu> menu_;
};

BraveSpellingMenuObserverTest::BraveSpellingMenuObserverTest() {}

BraveSpellingMenuObserverTest::~BraveSpellingMenuObserverTest() {}

}  // namespace

// Tests that right-clicking not add "Ask Brave for suggestions".
IN_PROC_BROWSER_TEST_F(BraveSpellingMenuObserverTest,
                       CheckAskBraveNotShown) {
  // Test menu with a misspelled word.
  Reset();
  InitMenu("wiimode", nullptr);
  menu()->PrintMenu("Test menu with a misspelled word.");
  CheckExpected();

  // Test menu with a correct word and spelling service enabled.
  Reset();
  menu()->GetPrefs()->SetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService, true);
  InitMenu("", nullptr);
  menu()->PrintMenu("Test menu with spelling service enabled.");
  CheckExpected();

  // Test menu with a misspelled word and spelling service enabled.
  Reset();
  menu()->GetPrefs()->SetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService, true);
  InitMenu("wiimode", nullptr);
  menu()->PrintMenu(
      "Test menu with a misspelled word spelling service enabled.");
  CheckExpected();

  // Test menu with a misspelled word, a suggestion, and spelling service
  // enabled.
  Reset();
  menu()->GetPrefs()->SetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService, true);
  InitMenu("wiimode", "wii mode");
  menu()->PrintMenu(
      "Test menu with a misspelled word, a suggestion, spelling service "
      "enabled.");
  CheckExpected();

  // Test menu with a misspelled word spelling service enabled in incognito
  // profile (which doesn't allow spelling service).
  Reset(true);
  menu()->GetPrefs()->SetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService, true);
  InitMenu("sjxdjiiiiii", nullptr);
  menu()->PrintMenu("Test menu with spelling service enabled in incognito.");
  CheckExpected();
}
