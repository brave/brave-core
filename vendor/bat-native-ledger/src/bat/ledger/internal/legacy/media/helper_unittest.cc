/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/legacy/media/helper.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=MediaHelperTest.*

namespace braveledger_media {

class MediaHelperTest : public testing::Test {
};

TEST(MediaHelperTest, GetMediaKey) {
  // provider is missing
  std::string result = braveledger_media::GetMediaKey("key", "");
  ASSERT_EQ(result, "");

  // key is missing
  result = braveledger_media::GetMediaKey("", "youtube");
  ASSERT_EQ(result, "");

  // all ok
  result = braveledger_media::GetMediaKey("key", "youtube");
  ASSERT_EQ(result, "youtube_key");
}

TEST(MediaHelperTest, GetTwitchParts) {
  std::vector<base::flat_map<std::string, std::string>> twitch_parts;

  // string is empty
  braveledger_media::GetTwitchParts("", &twitch_parts);
  ASSERT_EQ(twitch_parts.size(), 0u);

  // random string
  braveledger_media::GetTwitchParts("this is random string", &twitch_parts);
  ASSERT_EQ(twitch_parts.size(), 0u);

  const std::string post_data =
      "data=W3siZXZlbnQiOiJtaW51dGUtd2F0Y2hlZCIsInByb3BlcnRpZXMiOnsiYXBwX3ZlcnN"
      "pb24iOiI5LjI0LjAiLCJmbGFzaF92ZXJzaW9uIjoiMCwwLDAiLCJyZWZlcnJlcl91cmwiOiI"
      "iLCJyZWZlcnJlcl9ob3N0IjoiIiwicmVmZXJyZXJfZG9tYWluIjoiIiwiYnJvd3NlciI6IjU"
      "uMCAoTWFjaW50b3NoOyBJbnRlbCBNYWMgT1MgWCAxMF8xNF8zKSBBcHBsZVdlYktpdC81Mzc"
      "uMzYgKEtIVE1MLCBsaWtlIEdlY2tvKSBDaHJvbWUvNzQuMC4zNzI5LjYxIFNhZmFyaS81Mzc"
      "uMzYiLCJicm93c2VyX2ZhbWlseSI6ImNocm9tZSIsImJyb3dzZXJfdmVyc2lvbiI6Ijc0LjA"
      "iLCJvc19uYW1lIjoibWFjT1MiLCJvc192ZXJzaW9uIjoiMTAuMTQuMyIsInVzZXJfYWdlbnQ"
      "iOiJNb3ppbGxhLzUuMCAoTWFjaW50b3NoOyBJbnRlbCBNYWMgT1MgWCAxMF8xNF8zKSBBcHB"
      "sZVdlYktpdC81MzcuMzYgKEtIVE1MLCBsaWtlIEdlY2tvKSBDaHJvbWUvNzQuMC4zNzI5LjY"
      "xIFNhZmFyaS81MzcuMzYiLCJkZXZpY2VfaWQiOiIwOTM4MjFlYTZhYmQyN2MyIiwiZGlzdGl"
      "uY3RfaWQiOiIwOTM4MjFlYTZhYmQyN2MyIiwic2Vzc2lvbl9kZXZpY2VfaWQiOiJITnpZRHp"
      "3VzczWFlGUnJkRWttMkg1R1lQdktRWlFQbSIsImNsaWVudF9hcHAiOiJ0d2lsaWdodCIsImN"
      "saWVudF9idWlsZF9pZCI6IjczZmQ4NTdjLWJmNWMtNGUyYS05OWQwLWFlYzBkZjNmODVlNSI"
      "sInBsYXRmb3JtIjoid2ViIiwiY2hhbm5lbCI6ImRha290YXoiLCJiZW5jaG1hcmtfc2Vzc2l"
      "vbl9pZCI6IlZhNjBDdjlqbFA1UmxoalY1QXpjNHFYNGhVcVk1Smt5IiwibXNlX3N1cHBvcnQ"
      "iOnRydWUsImNvbnRlbnRfaWQiOiIiLCJjdXN0b21lcl9pZCI6IiIsInR1cmJvIjpmYWxzZSw"
      "iYmFja2VuZCI6Im1lZGlhcGxheWVyIiwiYmFja2VuZF92ZXJzaW9uIjoiMi45LjMtNWM0NjU"
      "0ODgiLCJnYW1lIjoiRm9ydG5pdGUiLCJicm9hZGNhc3Rlcl9zb2Z0d2FyZSI6InVua25vd25"
      "fcnRtcCIsImxpdmUiOnRydWUsImNoYW5uZWxfaWQiOjM5Mjk4MjE4LCJjb250ZW50X21vZGU"
      "iOiJsaXZlIiwicGFydG5lciI6dHJ1ZSwicXVhbGl0eSI6IkF1dG8iLCJzdHJlYW1fZm9ybWF"
      "0IjoiY2h1bmtlZCIsImJhbmR3aWR0aCI6NzA3OS45ODgsImNsdXN0ZXIiOiJ2aWUwMSIsImN"
      "1cnJlbnRfYml0cmF0ZSI6NzA3OS45ODgsImN1cnJlbnRfZnBzIjo2MCwiZGVjb2RlZF9mcmF"
      "tZXMiOjE0NTMsImRyb3BwZWRfZnJhbWVzIjowLCJobHNfbGF0ZW5jeV9icm9hZGNhc3RlciI"
      "6ODA1MiwiaGxzX2xhdGVuY3lfZW5jb2RlciI6ODAwMCwiaGxzX3RhcmdldF9kdXJhdGlvbiI"
      "6NSwibWFuaWZlc3RfY2x1c3RlciI6InZpZTAxIiwibWFuaWZlc3Rfbm9kZSI6InZpZGVvLXd"
      "lYXZlci52aWUwMSIsIm1hbmlmZXN0X25vZGVfdHlwZSI6IndlYXZlcl9jbHVzdGVyIiwic2V"
      "ydmluZ19pZCI6ImIzYmNkN2VmNjc0ZjQxYTE5MTM0NmY5NWNiMWU0NzRiIiwibm9kZSI6InZ"
      "pZGVvLWVkZ2UtNjljMTUwLnZpZTAxIiwidXNlcl9pcCI6Ijc3LjM4LjUxLjE5MSIsInZpZF9"
      "kaXNwbGF5X2hlaWdodCI6MzI2LCJ2aWRfZGlzcGxheV93aWR0aCI6NTgwLCJ2aWRfaGVpZ2h"
      "0Ijo5MDAsInZpZF93aWR0aCI6MTYwMCwidmlkZW9fYnVmZmVyX3NpemUiOjYuODE4NDE3OTk"
      "5OTk5OTk4LCJ2b2RfY2RuX29yaWdpbiI6IiIsInZvZF9jZG5fcmVnaW9uIjoiIiwidm9sdW1"
      "lIjowLjUsIm11dGVkIjpmYWxzZSwiaXNfaHR0cHMiOnRydWUsImF1dG9wbGF5ZWQiOnRydWU"
      "sImF2ZXJhZ2VfYml0cmF0ZSI6MCwiYnJvYWRjYXN0X2lkIjozMzY5OTgzMjczNiwiY2FwdGl"
      "vbnNfZW5hYmxlZCI6ZmFsc2UsImxhbmd1YWdlIjoiZW4tVVMiLCJtYW5pZmVzdF9icm9hZGN"
      "hc3RfaWQiOjMzNjk5ODMyNzM2LCJtaW51dGVzX2xvZ2dlZCI6MSwiY2xvY2tfZHJpZnQiOi0"
      "xMSwicGxheWVyX3NpemVfbW9kZSI6IiIsInF1YWxpdHlfY2hhbmdlX2NvdW50IjoyLCJzZWN"
      "vbmRzX29mZnNldCI6MjUuMzU0LCJzdHJlYW1UeXBlIjoibGl2ZSIsInRhYl9zZXNzaW9uX2l"
      "kIjoiZTM4MDBiZTIwOGU1OTcyZCIsInRpbWVfc3BlbnRfaGlkZGVuIjowLCJ0cmFuc2NvZGV"
      "yX3R5cGUiOiIyMDE3VHJhbnNjb2RlWDI2NF9WMiIsImF1dG9fbXV0ZWQiOmZhbHNlLCJkZXZ"
      "pY2VfcGl4ZWxfcmF0aW8iOjIsImVzdGltYXRlZF9iYW5kd2lkdGgiOjEyNjQ5NjQ5NywicGx"
      "heWJhY2tfcmF0ZSI6MSwidHJhbnNwb3J0X3NlZ21lbnRzIjoxNSwidHJhbnNwb3J0X2ZpcnN"
      "0X2J5dGVfbGF0ZW5jeSI6ODAxLCJ0cmFuc3BvcnRfc2VnbWVudF9kdXJhdGlvbiI6Mjk5OTE"
      "sInRyYW5zcG9ydF9kb3dubG9hZF9kdXJhdGlvbiI6MTgwMCwidHJhbnNwb3J0X2Rvd25sb2F"
      "kX2J5dGVzIjoyNzYxMDA1NiwiY29udGVudCI6ImdhbWVfYm94YXJ0IiwibWVkaXVtIjoidHd"
      "pdGNoX2hvbWUiLCJjb2xsYXBzZV9sZWZ0IjpmYWxzZSwiY29sbGFwc2VfcmlnaHQiOmZhbHN"
      "lLCJpdGVtX3RyYWNraW5nX2lkIjoiMDk5YmMyMGFmOGVmYTI4ZiIsIml0ZW1fcG9zaXRpb24"
      "iOjAsInJvd19uYW1lIjoiVG9wR2FtZXNGb3JZb3UiLCJyb3dfcG9zaXRpb24iOjEsInRhZ19"
      "maWx0ZXJfc2V0IjoiW10iLCJ0YWdfc2V0IjoiW1wiNmVhNmJjYTQtNDcxMi00YWI5LWE5MDY"
      "tZTMzMzZhOWQ4MDM5XCJdIiwidGFnX3N0cmVhbWVyX3NldCI6IltdIiwiYXBwX3Nlc3Npb25"
      "faWQiOiJlMzgwMGJlMjA4ZTU5NzJkIiwicGFnZV9zZXNzaW9uX2lkIjoiODkwNjc1OWZhOWE"
      "wNzg4NSIsInJlZmVycmVyIjoiIiwicGxheV9zZXNzaW9uX2lkIjoiZGJXUVRwRkhSZU9jUEF"
      "YV2M3bThoV3VRdmJ3OVZQR08iLCJ1cmwiOiJodHRwczovL3d3dy50d2l0Y2gudHYvZGFrb3R"
      "heiIsImhvc3QiOiJ3d3cudHdpdGNoLnR2IiwiZG9tYWluIjoidHdpdGNoLnR2IiwibGF0ZW5"
      "jeV9tb2RlX3RvZ2dsZSI6dHJ1ZSwibG93X2xhdGVuY3kiOmZhbHNlLCJoaWRkZW4iOmZhbHN"
      "lLCJwbGF5ZXIiOiJzaXRlIiwiZW5jcnlwdGVkIjpmYWxzZSwidGltZSI6MTU1NTMxNjQ0NS4"
      "0Mzd9fV0=";

  const std::vector<base::flat_map<std::string, std::string>> result =
      {{
        {"channel", "dakotaz"},
        {"event", "minute-watched"},
        {"properties", ""},
        {"time", "1555316445.437000"}
      }};

  // all ok
  braveledger_media::GetTwitchParts(post_data, &twitch_parts);
  ASSERT_EQ(twitch_parts.size(), 1u);
  ASSERT_EQ(twitch_parts, result);
}

TEST(MediaHelperTest, ExtractData) {
//  // string empty
  std::string result = braveledger_media::ExtractData("", "/", "!");
  ASSERT_EQ(result, "");

  // missing start
  result = braveledger_media::ExtractData("st/find/me!", "", "!");
  ASSERT_EQ(result, "st/find/me");

  // missing end
  result = braveledger_media::ExtractData("st/find/me!", "/", "");
  ASSERT_EQ(result, "find/me!");

  // all ok
  result = braveledger_media::ExtractData("st/find/me!", "/", "!");
  ASSERT_EQ(result, "find/me");
}

}  // namespace braveledger_media
