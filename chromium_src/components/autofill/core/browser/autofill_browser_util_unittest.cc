/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/autofill_browser_util.h"

#include "base/test/task_environment.h"
#include "components/autofill/core/browser/foundations/test_autofill_client.h"
#include "components/autofill/core/common/form_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace autofill {
namespace {

FormData CreateFormWithAction(const GURL& action) {
  FormData form;
  form.set_action(action);
  return form;
}

class AutofillBrowserUtilTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  TestAutofillClient client_;
};

TEST_F(AutofillBrowserUtilTest, HttpsFormWithInsecureAction) {
  client_.set_last_committed_primary_main_frame_url(GURL("https://myform.com"));
  EXPECT_TRUE(IsFormMixedContent(
      client_, CreateFormWithAction(GURL("http://myform.com/submit"))));
}

TEST_F(AutofillBrowserUtilTest, HttpsFormWithSecureAction) {
  client_.set_last_committed_primary_main_frame_url(GURL("https://myform.com"));
  EXPECT_FALSE(IsFormMixedContent(
      client_, CreateFormWithAction(GURL("https://myform.com/submit"))));
}

TEST_F(AutofillBrowserUtilTest, HttpFormWithInsecureAction) {
  // A plain http page isn't a secure context, so it can't have "mixed"
  // content regardless of the action's scheme.
  client_.set_last_committed_primary_main_frame_url(GURL("http://myform.com"));
  EXPECT_FALSE(IsFormMixedContent(
      client_, CreateFormWithAction(GURL("http://myform.com/submit"))));
}

// .onion pages are treated as a secure context for mixed-content purposes,
// even when served over plain http, since Tor already provides the
// transport security https would otherwise provide.
TEST_F(AutofillBrowserUtilTest, OnionHttpsFormWithInsecureAction) {
  client_.set_last_committed_primary_main_frame_url(
      GURL("https://myform.onion"));
  EXPECT_TRUE(IsFormMixedContent(
      client_, CreateFormWithAction(GURL("http://myform.com/submit"))));
}

TEST_F(AutofillBrowserUtilTest, OnionHttpFormWithInsecureAction) {
  client_.set_last_committed_primary_main_frame_url(
      GURL("http://myform.onion"));
  EXPECT_TRUE(IsFormMixedContent(
      client_, CreateFormWithAction(GURL("http://myform.com/submit"))));
}

TEST_F(AutofillBrowserUtilTest, OnionHttpSubdomainFormWithInsecureAction) {
  client_.set_last_committed_primary_main_frame_url(
      GURL("http://a.myform.onion"));
  EXPECT_TRUE(IsFormMixedContent(
      client_, CreateFormWithAction(GURL("http://myform.com/submit"))));
}

TEST_F(AutofillBrowserUtilTest, OnionFormWithSecureAction) {
  client_.set_last_committed_primary_main_frame_url(
      GURL("http://myform.onion"));
  EXPECT_FALSE(IsFormMixedContent(
      client_, CreateFormWithAction(GURL("https://myform.com/submit"))));
}

}  // namespace
}  // namespace autofill
