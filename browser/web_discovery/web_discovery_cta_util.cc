/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_cta_util.h"

#include "base/check_is_test.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/time/clock.h"
#include "base/values.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace {

constexpr int kMaxDisplayCount = 5;
constexpr char kWebDiscoveryCTAStateIdKey[] = "id";
constexpr char kWebDiscoveryCTAStateCountKey[] = "count";
constexpr char kWebDiscoveryCTAStateDismissedKey[] = "dismissed";
constexpr char kWebDiscoveryCTAStateLastDisplayedKey[] = "last_displayed";

bool IsBraveSearchDefault(TemplateURLService* template_service) {
  DCHECK(template_service);

  auto* template_url = template_service->GetDefaultSearchProvider();
  if (!template_url)
    return false;
  return template_url->prepopulate_id() ==
         TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE;
}

}  // namespace

WebDiscoveryCTAState GetWebDiscoveryCTAState(PrefService* prefs,
                                             const std::string& cta_id) {
  WebDiscoveryCTAState state;
  const auto& state_value = prefs->GetDict(kWebDiscoveryCTAState);
  const auto* id = state_value.FindString(kWebDiscoveryCTAStateIdKey);
  if (!id || *id != cta_id) {
    // Give fresh state with new id.
    state.id = cta_id;
    return state;
  }

  state.id = *id;

  const auto count = state_value.FindInt(kWebDiscoveryCTAStateCountKey);
  const auto dismissed =
      state_value.FindBool(kWebDiscoveryCTAStateDismissedKey);
  const auto* last_displayed =
      state_value.Find(kWebDiscoveryCTAStateLastDisplayedKey);
  // If state is invalid, give fresh one.
  if (!count || !dismissed || !last_displayed)
    return state;

  state.count = *count;
  state.dismissed = *dismissed;
  state.last_displayed = *base::ValueToTime(last_displayed);

  return state;
}

std::string GetWebDiscoveryCurrentCTAId() {
  if (!GetWebDiscoveryCTAIDForTesting().empty()) {
    CHECK_IS_TEST();
    return GetWebDiscoveryCTAIDForTesting();
  }

  // Update this when we want to start cta again.
  // TODO(simonhong): Need to improve for updating current cta id.
  // Maybe fetching new cta id remotely?
  constexpr char kCurrentCTAId[] = "v1";
  return kCurrentCTAId;
}

bool ShouldShowWebDiscoveryInfoBar(TemplateURLService* service,
                                   PrefService* prefs,
                                   const WebDiscoveryCTAState& state,
                                   base::Clock* test_clock) {
  if (prefs->GetBoolean(kWebDiscoveryEnabled))
    return false;

  if (!service || !IsBraveSearchDefault(service))
    return false;

  // Show when |state| is newly created one.
  if (state.last_displayed.is_null() && state.count == 0)
    return true;

  // Don't show if user dismissed explicitely.
  if (state.dismissed)
    return false;

  // Don't show same cta more than 5 times.
  if (state.count >= kMaxDisplayCount)
    return false;

  const auto now = test_clock ? test_clock->Now() : base::Time::Now();

  // Don't show if |last_displayed| is not valid.
  // ex) last_displayed is newer than now.
  if (now < state.last_displayed)
    return false;

  // Don't show same cta twice in one day.
  if (now - state.last_displayed < base::Days(1))
    return false;

  return true;
}

void SetWebDiscoveryCTAStateToPrefs(PrefService* prefs,
                                    const WebDiscoveryCTAState& state) {
  ScopedDictPrefUpdate update(prefs, kWebDiscoveryCTAState);
  base::Value::Dict& dict = update.Get();
  dict.Set(kWebDiscoveryCTAStateIdKey, state.id);
  dict.Set(kWebDiscoveryCTAStateCountKey, state.count);
  dict.Set(kWebDiscoveryCTAStateDismissedKey, state.dismissed);
  dict.Set(kWebDiscoveryCTAStateLastDisplayedKey,
           base::TimeToValue(state.last_displayed));
}

std::string& GetWebDiscoveryCTAIDForTesting() {
  static base::NoDestructor<std::string> cta_id;
  return *cta_id;
}
