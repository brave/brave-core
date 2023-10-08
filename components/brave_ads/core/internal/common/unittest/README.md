# Testing

Helpful stuff for testing ads.

Ads tests **MUST** inherit from `UnitTestBase` when using `TEST_F` or `TEST_P`, i.e.

    class FooTest : public UnitTestBase {
     protected:
      void SetUp() override { ... }
      void SetUpMocks() override { ... }
      void TearDown() override { ... }
    };

    TEST_F(FooTest, Bar) { ... }

You should set up mocks in `SetUpMocks`, `TEST_F`, or `TEST_P`. You should **NOT** add mocks to `SetUp`.

`UnitTestBase` is inherited from `::testing::Test` and is not copyable.

## Integration Testing

Override `SetUp` and call `SetUpForTesting` with `is_integration_test` set to `true` to test functionality and performance under product-like circumstances with data to replicate live settings to simulate a real user scenario from start to finish.

Use the `GetAds` convenience function to access `AdsImpl`. i.e.

    GetAds().TriggerNotificationAdEvent(/*placement_id*
    "7ee858e8-6306-4317-88c3-9e7d58afad26", ConfirmationType::kClicked);

## Mocking Command-Line Switches

See [unittest_command_line_switch_util.h](unittest_command_line_switch_util.h).

## Mocking Task Environment

See [unittest_time_util.h](unittest_time_util.h).

## Mocking File Resources

You can mock file resources loaded with `LoadFileResource` by placing your mocked files in the following directory:

    .
    └── brave/components/brave_ads/core/
        └── test/
            └── data/
                └── resources

## Mocking Files

You can mock files loaded with `LoadFile` by placing your mocked files in the following directory:

    .
    └── brave/components/brave_ads/core/
        └── test/
            └── data

You can copy files to the simulated user profile (temp path) using `CopyFileFromTestPathToTempPath` or `CopyDirectoryFromTestPathToTempPath` in `SetUpMocks`.

## Mocking Prefs

Prefs must be registered in [unittest_pref_registry_util.cc](./unittest_pref_registry_util.cc).

You can call `ads_client_mock_->Set*Pref` to set a pref and notify listeners. If you do not want to notify listeners, you can call `Set*Pref`, which will only set the pref. See [unittest_pref_util.h](./unittest_pref_util.h).

## Mocking Server Responses

Mocked responses for URL requests can be defined inline or read from a text file. You should store test data files at `brave/components/brave_ads/core/test/data/`. Inline filenames should begin with a forward slash, i.e.

    const URLResponseMap url_responses = {
      "/foo/bar", {
        {
          net::HTTP_NOT_FOUND, "What we've got here is... failure to communicate"
        },
        {
          net::HTTP_OK, "/response.json"
        }
      }
    }

    MockUrlResponses(ads_client_mock_, url_responses);

Inline or text file responses can contain `<time:period>` tags for mocking timestamps, where `period` can be `now`, `distant_past`, `distant_future`, `+/-# seconds`, `+/-# minutes`, `+/-# hours` or `+/-# days`, i.e.

    const URLResponseMap url_responses = {
      "/foo/bar", {
        {
          net::HTTP_OK, "An example response with a timestamp <time:+7 days> in the not too distant future"
        }
      }
    }

    MockUrlResponses(ads_client_mock_, url_responses);

`distant_past` is equivalent to `base::Time() + base::Microseconds(1)` and `distant_future` is defined as `Tuesday, 19 January 2038 03:14:07` in UTC.

You can add one or more responses per request, which will be returned in the given order, then will wrap back to the first response after mocking the last, i.e.

    const URLResponseMap url_responses = {
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

    MockUrlResponses(ads_client_mock_, url_responses);

## Mocking Settings

See [settings_unittest_util.h](../../settings/settings_unittest_util.h).

## Mocking Client

| mock  | type  | default  | example  |
|---|---|---|---|
| Device identifier  |  |  | `MockDeviceId();`  |
| Build channel  | `kRelease`, `kBeta` or `kNightly`  | `kRelease`  | `MockBuildChannel(BuildChannelType::kNightly);`  |
| Platform  | `kWindows`, `kMacOS`, `kLinux`, `kAndroid` or `kIOS`  | `kWindows`  | `MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);`  |
| Is network connection available  | boolean  | `true`  | `MockIsNetworkConnectionAvailable(ads_client_mock_, false);`  |
| Is browser active  | boolean  | `true`  | `MockIsBrowserActive(ads_client_mock_, false);`  |
| Is browser in full-screen mode  | boolean  | `false`  | `MockIsBrowserInFullScreenMode(ads_client_mock_, true);`  |
| Can show notification ads  | boolean  | `true`  | `MockCanShowNotificationAds(ads_client_mock_, false);`  |
| Can show notification ads while browser is backgrounded  | boolean  | `true`  | `MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, false);`  |
| Browsing history  | vector<GURL>  |  | `MockGetBrowsingHistory(ads_client_mock_, {GURL("https://foo.com"), GURL("https://bar.com")});`  |
| Mock URL responses  | URLResponseMap  |  | See [mocking server responses](#mocking-server-responses).  |

See [unittest_mock_util.h](./unittest_mock_util.h) and [unittest_constants.h](./unittest_constants.h).

Please add to it!
