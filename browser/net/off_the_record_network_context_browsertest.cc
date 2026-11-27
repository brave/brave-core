/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <atomic>
#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/image_fetcher/image_fetcher_service_factory.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/profile_network_context_service_factory.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_key.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/profile_destruction_waiter.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/image_fetcher/core/image_fetcher_service.h"
#include "components/image_fetcher/core/request_metadata.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/test/browser_test.h"
#include "net/base/net_errors.h"
#include "net/test/embedded_test_server/controllable_http_response.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_profile_service_impl.h"
#endif

// On Windows the browser test headers pull in winuser.h, which defines
// LoadImage as a macro to LoadImageW. Undefine it before including
// image_cache.h, so ImageCache::LoadImage keeps its name in the declaration
// and in this file's calls, matching the definition in image_cache.cc.
#if BUILDFLAG(IS_WIN)
#undef LoadImage
#endif
#include "components/image_fetcher/core/cache/image_cache.h"

namespace {

// A host that only resolves through a proxy, so a request can reach a server
// only through the network context whose proxy points at it.
constexpr char kImageUrl[] =
    "http://unresolvable.test/image_decoding/droids.png";

// Counts the requests a server receives for `kImageUrl`'s path.
class RequestCounter {
 public:
  void Monitor(net::EmbeddedTestServer& server) {
    server.RegisterRequestMonitor(base::BindRepeating(
        &RequestCounter::OnRequest, base::Unretained(this)));
  }

  int count() const { return count_; }

 private:
  void OnRequest(const net::test_server::HttpRequest& request) {
    if (request.GetURL().path() == GURL(kImageUrl).path()) {
      ++count_;
    }
  }

  std::atomic<int> count_ = 0;
};

}  // namespace

// Requests made for an off-the-record profile must go through that profile's
// own network context, never the browser-wide system network context. Each
// context gets a fixed-server proxy pointing at its own test server, so the
// server that receives a request shows which context sent it.
class OffTheRecordNetworkContextBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Must exist before the server starts. Only the test that waits on it
    // keeps a request pending; the others' requests get no response.
    held_profile_response_ =
        std::make_unique<net::test_server::ControllableHttpResponse>(
            &profile_proxy_, std::string(GURL(kImageUrl).path()));
    system_counter_.Monitor(system_proxy_);
    profile_counter_.Monitor(profile_proxy_);
    // Serves a real image, so a regular profile's fetch is stored in its
    // image cache.
    system_proxy_.ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(system_proxy_.Start());
    ASSERT_TRUE(profile_proxy_.Start());

    g_browser_process->local_state()->SetDict(
        proxy_config::prefs::kProxy,
        ProxyConfigDictionary::CreateFixedServers(
            system_proxy_.host_port_pair().ToString(), std::string()));
    g_browser_process->system_network_context_manager()
        ->FlushProxyConfigMonitorForTesting();
  }

  // Creates a Private window's profile and points its network context at
  // `profile_proxy_`.
  Profile* CreateOffTheRecordProfile() {
    Profile* profile = browser()->GetProfile()->GetPrimaryOTRProfile(
        /*create_if_needed=*/true);
    profile->GetPrefs()->SetDict(
        proxy_config::prefs::kProxy,
        ProxyConfigDictionary::CreateFixedServers(
            profile_proxy_.host_port_pair().ToString(), std::string()));
    ProfileNetworkContextServiceFactory::GetForContext(profile)
        ->FlushProxyConfigMonitorForTesting();
    return profile;
  }

  // Fetches `kImageUrl` through `profile`'s ImageFetcherService, and calls
  // `on_done` with the request's net error.
  void StartImageFetch(Profile* profile,
                       base::OnceCallback<void(int)> on_done,
                       image_fetcher::ImageFetcherConfig config =
                           image_fetcher::ImageFetcherConfig::kNetworkOnly) {
    image_fetcher::ImageFetcherService* service =
        ImageFetcherServiceFactory::GetForKey(profile->GetProfileKey());
    ASSERT_TRUE(service);
    service->GetImageFetcher(config)->FetchImage(
        GURL(kImageUrl),
        base::BindOnce(
            [](base::OnceCallback<void(int)> on_done, const gfx::Image&,
               const image_fetcher::RequestMetadata& metadata) {
              std::move(on_done).Run(metadata.net_error);
            },
            std::move(on_done)),
        image_fetcher::ImageFetcherParams(TRAFFIC_ANNOTATION_FOR_TESTS,
                                          "BraveTest"));
  }

  // Fetches `kImageUrl` through `profile`'s ImageFetcherService, waits for the
  // result and returns its net error.
  int FetchImage(Profile* profile,
                 image_fetcher::ImageFetcherConfig config =
                     image_fetcher::ImageFetcherConfig::kNetworkOnly,
                 base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    base::test::TestFuture<int> net_error;
    StartImageFetch(profile, net_error.GetCallback(), config);
    return net_error.Get();
  }

  // Returns whether `profile`'s image cache holds `kImageUrl` now.
  bool IsImageCached(Profile* profile) {
    base::test::TestFuture<bool, std::string> cached;
    ImageFetcherServiceFactory::GetForKey(profile->GetProfileKey())
        ->ImageCacheForTesting()
        ->LoadImage(/*read_only=*/true, kImageUrl, cached.GetCallback());
    return !cached.Get<std::string>().empty();
  }

  // Waits until `profile`'s image cache holds `kImageUrl`. The cache stores an
  // image on a background sequence and reports no completion, so this polls
  // with a non-blocking cache read.
  bool WaitForImageCached(Profile* profile) {
    scoped_refptr<image_fetcher::ImageCache> cache =
        ImageFetcherServiceFactory::GetForKey(profile->GetProfileKey())
            ->ImageCacheForTesting();
    bool cached = false;
    bool read_pending = false;
    return base::test::RunUntil([&]() {
      if (!cached && !read_pending) {
        read_pending = true;
        cache->LoadImage(
            /*read_only=*/true, kImageUrl,
            base::BindLambdaForTesting([&](bool, std::string image_data) {
              cached = !image_data.empty();
              read_pending = false;
            }));
      }
      return cached;
    });
  }

  net::test_server::ControllableHttpResponse& held_profile_response() {
    return *held_profile_response_;
  }

  int system_requests() const { return system_counter_.count(); }
  int profile_requests() const { return profile_counter_.count(); }

 private:
  RequestCounter system_counter_;
  RequestCounter profile_counter_;
  // Declared after the counters their monitors use, so the servers shut down
  // before the counters are destroyed.
  net::EmbeddedTestServer system_proxy_;
  net::EmbeddedTestServer profile_proxy_;
  std::unique_ptr<net::test_server::ControllableHttpResponse>
      held_profile_response_;
};

IN_PROC_BROWSER_TEST_F(OffTheRecordNetworkContextBrowserTest,
                       ImageFetcherServiceUsesProfileNetworkContext) {
  Profile* profile = CreateOffTheRecordProfile();
  StartImageFetch(profile, base::DoNothing());
  held_profile_response().WaitForRequest();

  EXPECT_EQ(1, profile_requests())
      << "The fetch did not go through the off-the-record profile's network "
         "context.";
  EXPECT_EQ(0, system_requests())
      << "The fetch went through the system network context.";
}

IN_PROC_BROWSER_TEST_F(OffTheRecordNetworkContextBrowserTest,
                       RegularProfileImageFetcherServiceIsUnchanged) {
  FetchImage(browser()->GetProfile());

  EXPECT_EQ(1, system_requests())
      << "A regular profile's fetch no longer uses the system network context.";
  EXPECT_EQ(0, profile_requests());
}

// An off-the-record profile's image cache does not return images the regular
// profile cached. Its key keeps the regular profile's path, so without its own
// data store it would read the regular profile's image files.
IN_PROC_BROWSER_TEST_F(OffTheRecordNetworkContextBrowserTest,
                       DiskCacheFetchDoesNotReadRegularProfileCache) {
  // The regular profile fetches the image once, which stores it in its cache.
  // The system proxy's server answers with an image the cache will keep.
  ASSERT_NE(net::ERR_UNEXPECTED,
            FetchImage(browser()->GetProfile(),
                       image_fetcher::ImageFetcherConfig::kDiskCacheOnly));
  ASSERT_EQ(1, system_requests());
  ASSERT_TRUE(WaitForImageCached(browser()->GetProfile()))
      << "The regular profile did not cache the image.";

  Profile* profile = CreateOffTheRecordProfile();
  EXPECT_FALSE(IsImageCached(profile))
      << "The off-the-record profile's cache holds the regular profile's "
         "image.";

  // The fetch misses the cache and goes through the off-the-record profile's
  // network context.
  StartImageFetch(profile, base::DoNothing(),
                  image_fetcher::ImageFetcherConfig::kDiskCacheOnly);
  held_profile_response().WaitForRequest();
  EXPECT_EQ(1, profile_requests());
  EXPECT_EQ(1, system_requests());
}

// Destroying an off-the-record profile while its fetch is in flight drops the
// fetch with its service, as upstream does for every profile, without
// crashing or sending the request through another network context.
IN_PROC_BROWSER_TEST_F(OffTheRecordNetworkContextBrowserTest,
                       FetchInFlightWhenProfileIsDestroyed) {
  Profile* profile = CreateOffTheRecordProfile();
  StartImageFetch(profile, base::DoNothing());
  held_profile_response().WaitForRequest();

  ProfileDestructionWaiter destruction_waiter(profile);
  browser()->GetProfile()->DestroyOffTheRecordProfile(profile);
  destruction_waiter.Wait();

  EXPECT_EQ(1, profile_requests());
  EXPECT_EQ(0, system_requests());
}

#if BUILDFLAG(ENABLE_TOR)
// A Tor window's fetch goes through Tor. The Tor profile's proxy refuses
// connections, so a fetch sent through Tor fails with
// ERR_PROXY_CONNECTION_FAILED and reaches neither test server.
IN_PROC_BROWSER_TEST_F(OffTheRecordNetworkContextBrowserTest,
                       TorImageFetcherServiceUsesTorProxy) {
  TorProfileManager::SwitchToTorProfile(browser()->GetProfile());
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(browser()->GetProfile());
  ASSERT_TRUE(tor_profile);

  net::EmbeddedTestServer closed_server;
  ASSERT_TRUE(closed_server.Start());
  const std::string closed_host_port =
      closed_server.host_port_pair().ToString();
  ASSERT_TRUE(closed_server.ShutdownAndWaitUntilComplete());
  static_cast<tor::TorProfileServiceImpl*>(
      TorProfileServiceFactory::GetForContext(tor_profile))
      ->OnTorNewProxyURI(base::StrCat({"socks5://", closed_host_port}));
  ProfileNetworkContextServiceFactory::GetForContext(tor_profile)
      ->FlushProxyConfigMonitorForTesting();

  EXPECT_EQ(net::ERR_PROXY_CONNECTION_FAILED, FetchImage(tor_profile));
  EXPECT_EQ(0, system_requests())
      << "The Tor window's fetch went through the system network context.";
  EXPECT_EQ(0, profile_requests());
}
#endif  // BUILDFLAG(ENABLE_TOR)
