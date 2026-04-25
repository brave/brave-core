# Testing

Helpful stuff for testing ads.

Ads tests **MUST** inherit from `test::TestBase` when using `TEST_F` or `TEST_P`, i.e.,

    class FooTest : public test::TestBase {
     protected:
      void SetUp() override { ... }
      void SetUpMocks() override { ... }
      void TearDown() override { ... }
    };

    TEST_F(FooTest, Bar) { ... }

Mocks should be set up in `SetUpMocks`, `TEST_F`, or `TEST_P`. Avoid setting up mocks in `SetUp` as they might be overridden by `test::TestBase`.

`test::TestBase`, inherited from `::testing::Test`, is not copyable.

Refer to `*_test_util.h`, `*_test_helper.h`, and `*_test_constants.h` for support in efficiently creating and structuring unit tests.

## Timezone-Sensitive Tests

Tests that depend on local-time behavior (month boundaries, midnight calculations, etc.) must use `test::ScopedTimezoneForTesting` to pin the timezone; otherwise tests may fail or produce incorrect results in other timezones. Use `::testing::WithParamInterface<std::string>` to parameterize over a representative set of IANA time zone IDs, e.g.,

    class BraveAdsFooTimezoneTest
        : public test::TestBase,
          public ::testing::WithParamInterface<std::string_view> {
     protected:
      const test::ScopedTimezoneForTesting scoped_timezone_{GetParam()};
    };

    INSTANTIATE_TEST_SUITE_P(BraveAdsTimezones,
                             BraveAdsFooTimezoneTest,
                             test::kTimezones,
                             test::TimezoneTestParamName);

`ScopedTimezoneForTesting` sets both ICU and libc timezones to ensure consistent local time conversion across platforms. See [scoped_timezone_for_testing.h](scoped_timezone_for_testing.h). `test::kTimezones` and `test::TimezoneTestParamName` provide the shared timezone set and name sanitizer. See [timezone_test_util.h](timezone_test_util.h).

## Mocking Task Environment

You can fast-forward virtual time using the `FastForwardClockBy`, `SuspendedFastForwardClockBy`, `FastForwardClockTo`, `FastForwardClockToNextPendingTask` helpers. These cause all tasks on the main thread and thread pool with a remaining delay to be executed.

Alternatively, you can advance the clock using the `AdvanceClockBy`, `AdvanceClockTo`, `AdvanceClockToLocalMidnight`, and `AdvanceClockToUTCMidnight` helpers, which do not run tasks.

See [task_environment_](../../../../../../../base/test/task_environment.h).

## Mocking Command-Line Switches

To mock command-line switches, invoke `test::AppendCommandLineSwitches` within `SetUpMocks`, i.e.,

    void SetUpMocks() override {
        test::AppendCommandLineSwitches({{"foo", "bar"}, {"baz", "qux"}});
    }

See [command_line_switch_test_util.h](command_line_switch_test_util.h).

## Mocking Prefs

You can register preferences in [pref_registry_test_util.cc](./pref_registry_test_util.cc) or by using `RegisterProfile*Pref` or `RegisterLocalState*Pref`. To set a preference and notify listeners, use `SetProfile*Pref` or `SetLocalState*Pref`. To set a preference without notifying listeners, use `SetProfile*PrefValue` or `SetLocalState*PrefValue`. See [profile_pref_value_test_util.h](./profile_pref_value_test_util.h) and [local_state_pref_value_test_util.h](./local_state_pref_value_test_util.h).

## Mocking Files

To mock files loaded with `Load`, place your mocked files in the following directory:

    .
    └── brave/components/brave_ads/core/
        └── test/
            └── data

To simulate a profile, use `CopyFileFromTestDataPathToProfilePath` or
`CopyDirectoryFromTestDataPathToProfilePath` in `SetUpMocks` for copying files or directories, respectively.

See [file_path_test_util.h](file_path_test_util.h) and [file_test_util.h](file_test_util.h).

## Mocking Resource Components

To mock resource components loaded with `LoadResourceComponent`, place your mocked files in the following directory:

    .
    └── brave/components/brave_ads/core/
        └── test/
            └── data/
                └── resources/
                   └── components

See [file_path_test_util.h](file_path_test_util.h).

## Mocking URL Responses

Mocked responses for URL requests can be defined inline or read from a text file.

To mock resource components, place your mocked files in the following directory:

    .
    └── brave/components/brave_ads/core/
        └── test/
            └── data/
                └── url_responses

Inline filenames should begin with a forward slash, i.e.,

    const test::URLResponseMap url_responses = {
      "/foo/bar", {
        {
          net::HTTP_NOT_FOUND, "What we've got here is... failure to communicate"
        },
        {
          net::HTTP_OK, "/response.json"
        }
      }
    };

    test::MockUrlResponses(ads_client_mock_, url_responses);

Inline or text file responses can contain `<time:period>` tags for mocking timestamps, where `period` can be `now`, `distant_past`, `distant_future`, `+/-# seconds`, `+/-# minutes`, `+/-# hours`, or `+/-# days`, i.e.,

    const test::URLResponseMap url_responses = {
      "/foo/bar", {
        {
          net::HTTP_OK, "An example response with a timestamp <time:+7 days> in the not too distant future"
        }
      }
    };

    test::MockUrlResponses(ads_client_mock_, url_responses);

`distant_past` is equivalent to `base::Time() + base::Milliseconds(1)` and `distant_future` is defined as `Tuesday, 19 January 2038 03:14:07` in UTC.

You can add one or more responses per request. These will be returned in the given order and will wrap back to the first response after reaching the last, i.e.,

    const test::URLResponseMap url_responses = {
      {
        "/foo/bar", {
          {
            net::HTTP_INTERNAL_SERVER_ERROR, "Program say to kill, to disassemble, to make dead"
          },
          {
            net::HTTP_CREATED, "To me there's no creativity without boundaries"
          }
        }
      },
      {
        "/baz", {
          {
            net::HTTP_IM_A_TEAPOT, "Keep Calm & Drink Tea! L. Masinter, 1 April 1998"
          }
        }
      }
    };

    test::MockUrlResponses(ads_client_mock_, url_responses);

## Test Environment Defaults

| setting  | type  | default  | example  |
|---|---|---|---|
| Browser version number  | string  | `1.2.3.4`  | `fake_browser_version_.SetNumber("2.0.0.0");` — see [`fake_browser_version.h`](../application_state/test/fake_browser_version.h)  |
| Device identifier  | string  | `21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e`  | `test::SetUpDeviceId();`  |
| Language code  | string (ISO 639-1)  | `en`  | `fake_locale_.SetLanguageCode("fr");`  |
| Country code  | string (ISO 3166-1)  | `US`  | `fake_locale_.SetCountryCode("FR");`  |
| Operating system  | `kWindows`, `kMacOS`, `kLinux`, `kAndroid` or `kIOS`  | `kWindows`  | `fake_operating_system_.SetType(OperatingSystemType::kMacOS);`  |
| Build channel  | `kRelease`, `kBeta` or `kNightly`  | `kRelease`  | `test::SetUpBuildChannel(test::BuildChannelType::kNightly);`  |
| JavaScript content setting  | boolean  | `true`  | `test::SetUpAllowJavaScript(false);`  |
| Is network connection available  | boolean  | `true`  | `test::MockIsNetworkConnectionAvailable(ads_client_mock_, false);`  |
| Is browser active  | boolean  | `true`  | `test::MockIsBrowserActive(ads_client_mock_, false);`  |
| Is browser in full-screen mode  | boolean  | `false`  | `test::MockIsBrowserInFullScreenMode(ads_client_mock_, true);`  |
| Can show notification ads  | boolean  | `true`  | `test::MockCanShowNotificationAds(ads_client_mock_, false);`  |
| Can show notification ads while browser is backgrounded  | boolean  | `false`  | `test::MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, true);`  |
| Site history  | vector\<GURL>  |  | `test::MockGetSiteHistory(ads_client_mock_, {GURL("https://foo.com"), GURL("https://bar.com")});`  |
| URL response  | URLResponseMap  |  | See [mocking server responses](#mocking-url-responses).  |

See [test_environment_util.h](./test_environment_util.h), [mock_test_util.h](./mock_test_util.h), and [test_constants.h](./test_constants.h).

# Integration Testing

Override `SetUp` and call it with `is_integration_test` set to `true` to test functionality and performance under product-like circumstances. This approach uses data to replicate live settings, simulating a real user scenario from start to finish.

Use the `GetAds` convenience function to access `Ads`. i.e.

    GetAds().TriggerNotificationAdEvent(
        /*placement_id=*/"7ee858e8-6306-4317-88c3-9e7d58afad26",
        mojom::ConfirmationType::kClicked);

Please add to it!
