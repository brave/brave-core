/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_file_select_utils.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/javascript_dialogs/app_modal_dialog_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

TEST(BraveFileSelectUtilsUnitTest, GetSiteFrameTitle_InSyncWithUpstream) {
  constexpr struct Case {
    // The name of the test case.
    const char* case_name;

    // The URL of the main frame of the page.
    const char* main_frame_url;

    // Whether the main frame is alerting.
    bool is_main_frame;

    // If `is_main_frame` is false, the URL of the alerting frame of the page.
    const char* alerting_frame_url;
  } kCases[] = {
      // Standard main frame alert.
      {"standard", "http://foo.com/", true, ""},

      // Subframe alert from the same origin.
      {"subframe same origin", "http://foo.com/1", false, "http://foo.com/2"},
      // Subframe alert from a different origin.
      {"subframe different origin", "http://foo.com/", false,
       "http://bar.com/"},

      // file:
      // - main frame:
      {"file main frame", "file:///path/to/page.html", true, ""},
      // - subframe:
      {"file subframe", "http://foo.com/", false, "file:///path/to/page.html"},

      // data:
      // /!\ NOTE that this is for data URLs entered directly in the omnibox.
      // For pages that generate frames with data URLs, see the browsertest.
      // - main frame:
      {"data main frame", "data:blahblah", true, ""},
      // - subframe:
      {"data subframe", "http://foo.com/", false, "data:blahblah"},

      // javascript:
      // /!\ NOTE that this is for javascript URLs entered directly in the
      // omnibox. For pages that generate frames with javascript URLs, see the
      // browsertest.
      // - main frame:
      {"javascript main frame", "javascript:abc", true, ""},
      // - subframe:
      {"javascript subframe", "http://foo.com/", false, "javascript:abc"},

      // about:
      // /!\ NOTE that this is for about:blank URLs entered directly in the
      // omnibox. For pages that generate frames with about:blank URLs, see the
      // browsertest.
      // - main frame:
      {"about main frame", "about:blank", true, ""},
      // - subframe:
      {"about subframe", "http://foo.com/", false, "about:blank"},

      // blob:
      // - main frame:
      {"blob main frame",
       "blob:http://foo.com/66666666-6666-6666-6666-666666666666", true, ""},
      // - subframe:
      {"blob subframe", "http://bar.com/", false,
       "blob:http://foo.com/66666666-6666-6666-6666-666666666666"},

      // filesystem:
      // - main frame:
      {"filesystem main frame", "filesystem:http://foo.com/bar.html", true, ""},
      // - subframe:
      {"filesystem subframe", "http://bar.com/", false,
       "filesystem:http://foo.com/bar.html"},
  };
  // Checks if our implementation is in sync with upstream.

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.case_name);

    url::Origin main_frame_origin =
        url::Origin::Create(GURL(test_case.main_frame_url));
    url::Origin alerting_frame_origin =
        test_case.is_main_frame
            ? main_frame_origin
            : url::Origin::Create(GURL(test_case.alerting_frame_url));
    EXPECT_EQ(javascript_dialogs::AppModalDialogManager::GetSiteFrameTitle(
                  main_frame_origin, alerting_frame_origin),
              brave::GetSiteFrameTitleForFileSelect(
                  brave::GetSiteFrameTitleType(main_frame_origin,
                                               alerting_frame_origin),
                  alerting_frame_origin,
                  brave::FileSelectTitleType::kChromiumDefault));
  }
}

TEST(BraveFileSelectUtilsUnitTest, GetSiteFrameTitleForFileSelect_Open) {
  constexpr struct Case {
    // The name of the test case.
    const char* case_name;

    // The URL of the main frame of the page.
    const char* main_frame_url;

    // Whether the main frame is alerting.
    bool is_main_frame;

    // If `is_main_frame` is false, the URL of the alerting frame of the page.
    const char* alerting_frame_url;

    // The expected title for the alert.
    const char* expected;
  } kCases[] = {
      // Standard main frame alert.
      {"standard", "http://foo.com/", true, "", "foo.com wants to open"},

      // Subframe alert from the same origin.
      {"subframe same origin", "http://foo.com/1", false, "http://foo.com/2",
       "foo.com wants to open"},
      // Subframe alert from a different origin.
      {"subframe different origin", "http://foo.com/", false, "http://bar.com/",
       "An embedded page at bar.com wants to open"},

      // file:
      // - main frame:
      {"file main frame", "file:///path/to/page.html", true, "",
       "This page wants to open"},
      // - subframe:
      {"file subframe", "http://foo.com/", false, "file:///path/to/page.html",
       "An embedded page on this page wants to open"},

      // data:
      // /!\ NOTE that this is for data URLs entered directly in the omnibox.
      // For pages that generate frames with data URLs, see the browsertest.
      // - main frame:
      {"data main frame", "data:blahblah", true, "", "This page wants to open"},
      // - subframe:
      {"data subframe", "http://foo.com/", false, "data:blahblah",
       "An embedded page on this page wants to open"},

      // javascript:
      // /!\ NOTE that this is for javascript URLs entered directly in the
      // omnibox. For pages that generate frames with javascript URLs, see the
      // browsertest.
      // - main frame:
      {"javascript main frame", "javascript:abc", true, "",
       "This page wants to open"},
      // - subframe:
      {"javascript subframe", "http://foo.com/", false, "javascript:abc",
       "An embedded page on this page wants to open"},

      // about:
      // /!\ NOTE that this is for about:blank URLs entered directly in the
      // omnibox. For pages that generate frames with about:blank URLs, see the
      // browsertest.
      // - main frame:
      {"about main frame", "about:blank", true, "", "This page wants to open"},
      // - subframe:
      {"about subframe", "http://foo.com/", false, "about:blank",
       "An embedded page on this page wants to open"},

      // blob:
      // - main frame:
      {"blob main frame",
       "blob:http://foo.com/66666666-6666-6666-6666-666666666666", true, "",
       "foo.com wants to open"},
      // - subframe:
      {"blob subframe", "http://bar.com/", false,
       "blob:http://foo.com/66666666-6666-6666-6666-666666666666",
       "An embedded page at foo.com wants to open"},

      // filesystem:
      // - main frame:
      {"filesystem main frame", "filesystem:http://foo.com/bar.html", true, "",
       "foo.com wants to open"},
      // - subframe:
      {"filesystem subframe", "http://bar.com/", false,
       "filesystem:http://foo.com/bar.html",
       "An embedded page at foo.com wants to open"},
  };

  brave_l10n::test::ScopedDefaultLocale scoped_locale("en-US");

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.case_name);
    url::Origin main_frame_origin =
        url::Origin::Create(GURL(test_case.main_frame_url));
    url::Origin alerting_frame_origin =
        test_case.is_main_frame
            ? main_frame_origin
            : url::Origin::Create(GURL(test_case.alerting_frame_url));
    EXPECT_EQ(base::UTF8ToUTF16(test_case.expected),
              brave::GetSiteFrameTitleForFileSelect(
                  brave::GetSiteFrameTitleType(main_frame_origin,
                                               alerting_frame_origin),
                  alerting_frame_origin, brave::FileSelectTitleType::kOpen));
  }
}

TEST(BraveFileSelectUtilsUnitTest, GetSiteFrameTitleForFileSelect_Save) {
  constexpr struct Case {
    // The name of the test case.
    const char* case_name;

    // The URL of the main frame of the page.
    const char* main_frame_url;

    // Whether the main frame is alerting.
    bool is_main_frame;

    // If `is_main_frame` is false, the URL of the alerting frame of the page.
    const char* alerting_frame_url;

    // The expected title for the alert.
    const char* expected;
  } kCases[] = {
      // Standard main frame alert.
      {"standard", "http://foo.com/", true, "", "foo.com wants to save"},

      // Subframe alert from the same origin.
      {"subframe same origin", "http://foo.com/1", false, "http://foo.com/2",
       "foo.com wants to save"},
      // Subframe alert from a different origin.
      {"subframe different origin", "http://foo.com/", false, "http://bar.com/",
       "An embedded page at bar.com wants to save"},

      // file:
      // - main frame:
      {"file main frame", "file:///path/to/page.html", true, "",
       "This page wants to save"},
      // - subframe:
      {"file subframe", "http://foo.com/", false, "file:///path/to/page.html",
       "An embedded page on this page wants to save"},

      // data:
      // /!\ NOTE that this is for data URLs entered directly in the omnibox.
      // For pages that generate frames with data URLs, see the browsertest.
      // - main frame:
      {"data main frame", "data:blahblah", true, "", "This page wants to save"},
      // - subframe:
      {"data subframe", "http://foo.com/", false, "data:blahblah",
       "An embedded page on this page wants to save"},

      // javascript:
      // /!\ NOTE that this is for javascript URLs entered directly in the
      // omnibox. For pages that generate frames with javascript URLs, see the
      // browsertest.
      // - main frame:
      {"javascript main frame", "javascript:abc", true, "",
       "This page wants to save"},
      // - subframe:
      {"javascript subframe", "http://foo.com/", false, "javascript:abc",
       "An embedded page on this page wants to save"},

      // about:
      // /!\ NOTE that this is for about:blank URLs entered directly in the
      // omnibox. For pages that generate frames with about:blank URLs, see the
      // browsertest.
      // - main frame:
      {"about main frame", "about:blank", true, "", "This page wants to save"},
      // - subframe:
      {"about subframe", "http://foo.com/", false, "about:blank",
       "An embedded page on this page wants to save"},

      // blob:
      // - main frame:
      {"blob main frame",
       "blob:http://foo.com/66666666-6666-6666-6666-666666666666", true, "",
       "foo.com wants to save"},
      // - subframe:
      {"blob subframe", "http://bar.com/", false,
       "blob:http://foo.com/66666666-6666-6666-6666-666666666666",
       "An embedded page at foo.com wants to save"},

      // filesystem:
      // - main frame:
      {"filesystem main frame", "filesystem:http://foo.com/bar.html", true, "",
       "foo.com wants to save"},
      // - subframe:
      {"filesystem subframe", "http://bar.com/", false,
       "filesystem:http://foo.com/bar.html",
       "An embedded page at foo.com wants to save"},
  };

  brave_l10n::test::ScopedDefaultLocale scoped_locale("en-US");

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.case_name);
    url::Origin main_frame_origin =
        url::Origin::Create(GURL(test_case.main_frame_url));
    url::Origin alerting_frame_origin =
        test_case.is_main_frame
            ? main_frame_origin
            : url::Origin::Create(GURL(test_case.alerting_frame_url));
    EXPECT_EQ(base::UTF8ToUTF16(test_case.expected),
              brave::GetSiteFrameTitleForFileSelect(
                  brave::GetSiteFrameTitleType(main_frame_origin,
                                               alerting_frame_origin),
                  alerting_frame_origin, brave::FileSelectTitleType::kSave));
  }
}
