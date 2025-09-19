/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/brave_protocol_handler.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/update_client/brave_protocol_serializer_json.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/update_client/persisted_data.h"
#include "components/update_client/protocol_definition.h"
#include "components/update_client/protocol_serializer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

namespace update_client {

TEST(BraveProtocolSerializerJSON, StripsPrivacySensitiveData) {
  // Much of this code was copied from protocol_serializer_json_unittest.cc.
  auto pref = std::make_unique<TestingPrefServiceSimple>();
  RegisterPersistedDataPrefs(pref->registry());
  auto metadata = CreatePersistedData(
      base::BindRepeating([](PrefService* pref) { return pref; }, pref.get()),
      nullptr);

  std::vector<base::Value::Dict> events(2);
  events[0].Set("download_time_ms", 9965);
  events[0].Set("eventresult", 1);
  events[1].Set("eventtype", 63);

  std::vector<protocol_request::App> apps;
  apps.push_back(MakeProtocolApp(
      "id1", base::Version("1.0"), "ap1", "BRND", "ins_id", "lang", -1,
      "source1", "location1", {{"attr", "1"}}, "c1", "ch1", "cn1", "test",
      {9384}, /*cached_hashes=*/{},
      MakeProtocolUpdateCheck(true, "33.12", true, false),
      {{"install", "foobar_install_data_index", ""}},
      MakeProtocolPing("id1", metadata.get(), {}), std::move(events)));

  const auto request =
      std::make_unique<BraveProtocolSerializerJSON>()->Serialize(
          MakeProtocolRequest(false, "{15160585-8ADE-4D3C-839B-1281A6035D1F}",
                              "prod_id", "1.0", "channel", "OS", "cacheable",
                              std::nullopt, {{"extra", "params"}}, {},
                              std::move(apps)));
  // The regex below checks two things. First, that the data from above is
  // faithfully reproduced in the serialized request. Second, that the following
  // fields, which could be used for fingerprinting users, do not appear:
  //  - hw
  //  - apps[*].lang
  //  - apps[*].events[*].download_time_ms
  static constexpr char regex[] =
      R"({"request":{"@os":"\w+","@updater":"prod_id",)"
      R"("acceptformat":"[^"]+",)"
      R"("apps":\[{"ap":"ap1","appid":"id1","attr":"1",)"
      R"("brand":"BRND","cohort":"c1","cohorthint":"ch1","cohortname":"cn1",)"
      R"("data":\[{"index":"foobar_install_data_index","name":"install"}],)"
      R"("disabled":\[{"reason":9384}],"enabled":false,)"
      R"("events":\[{"eventresult":1},{"eventtype":63}],)"
      R"("iid":"ins_id",)"
      R"("installdate":-1,)"
      R"("installedby":"location1","installsource":"source1",)"
      R"("ping":{[^}]*},)"
      R"("release_channel":"test",)"
      R"("updatecheck":{"rollback_allowed":true,)"
      R"("targetversionprefix":"33.12",)"
      R"("updatedisabled":true},"version":"1.0"}],"arch":"\w+","dedup":"cr",)"
      R"("dlpref":"cacheable","extra":"params",)"
      R"("ismachine":false,)"
      R"("os":{"arch":"[_,-.\w]+","platform":"OS",)"
      R"(("sp":"[\s\w]+",)?"version":"[+-.\w]+"},"prodchannel":"channel",)"
      R"("prodversion":"1.0","protocol":"4.0","requestid":"{[-\w]{36}}",)"
      R"("sessionid":"{[-\w]{36}}","updaterchannel":"channel",)"
      R"("updaterversion":"1.0"(,"wow64":true)?}})";
  EXPECT_TRUE(RE2::FullMatch(request, regex)) << request << "\n VS \n" << regex;
}

}  // namespace update_client
