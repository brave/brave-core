/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/omnibox/browser/brave_search_provider.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/buildflags/buildflags.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/autocomplete/document_suggestions_service_factory.h"
#include "chrome/browser/autocomplete/remote_suggestions_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/remote_suggestions_service.h"
#include "components/omnibox/browser/search_provider.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/variations/scoped_variations_ids_provider.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
#include "brave/components/omnibox/browser/search_suggestions/query_check_utils.h"
#endif

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
#include "ui/base/clipboard/test/test_clipboard.h"
#endif

// BraveSearchProviderTest -----------------------------------------------------

namespace {

constexpr char kSuggestionUrlHost[] = "https://defaultturl2/";

// Some code is copied from
// chrome/browser/autocomplete/search_provider_unittest.cc
class TestAutocompleteProviderClient : public ChromeAutocompleteProviderClient {
 public:
  TestAutocompleteProviderClient(Profile* profile,
                                 network::TestURLLoaderFactory* loader_factory)
      : ChromeAutocompleteProviderClient(profile),
        shared_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                loader_factory)) {}
  ~TestAutocompleteProviderClient() override {}

  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory()
      override {
    return shared_factory_;
  }

  bool SearchSuggestEnabled() const override { return true; }

 private:
  scoped_refptr<network::SharedURLLoaderFactory> shared_factory_;
};

std::unique_ptr<KeyedService> BuildRemoteSuggestionsServiceWithURLLoader(
    network::TestURLLoaderFactory* test_url_loader_factory,
    content::BrowserContext* context) {
  return std::make_unique<RemoteSuggestionsService>(
      DocumentSuggestionsServiceFactory::GetForProfile(
          Profile::FromBrowserContext(context), /*create_if_necessary=*/true),
      test_url_loader_factory->GetSafeWeakWrapper());
}

}  // namespace

class BraveSearchProviderTest : public testing::Test {
 public:
  BraveSearchProviderTest() {
    TestingProfile::Builder profile_builder;
    profile_builder.AddTestingFactory(
        HistoryServiceFactory::GetInstance(),
        HistoryServiceFactory::GetDefaultFactory());
    profile_builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));
    profile_builder.AddTestingFactory(
        RemoteSuggestionsServiceFactory::GetInstance(),
        base::BindRepeating(&BuildRemoteSuggestionsServiceWithURLLoader,
                            &test_url_loader_factory_));

    profile_ = profile_builder.Build();
  }

  BraveSearchProviderTest(const BraveSearchProviderTest&) = delete;
  BraveSearchProviderTest& operator=(const BraveSearchProviderTest&) = delete;

  void SetUp() override {
    std::string search_url = "https://defaultturl/{searchTerms}";
    std::string suggestions_url =
        base::StrCat({kSuggestionUrlHost, "{searchTerms}"});
    TemplateURLService* turl_model =
        TemplateURLServiceFactory::GetForProfile(profile_.get());

    turl_model->Load();

    // Reset the default TemplateURL.
    TemplateURLData data;
    data.SetShortName(u"t");
    data.SetURL(search_url);
    data.suggestions_url = suggestions_url;
    default_t_url_ = turl_model->Add(std::make_unique<TemplateURL>(data));
    turl_model->SetUserSelectedDefaultSearchProvider(default_t_url_);
    TemplateURLID default_provider_id = default_t_url_->id();
    ASSERT_NE(0, default_provider_id);

    // Keywords are updated by the InMemoryHistoryBackend only after the message
    // has been processed on the history thread. Block until history processes
    // all requests to ensure the InMemoryDatabase is the state we expect it.
    profile_->BlockUntilHistoryProcessesPendingRequests();

    AutocompleteClassifierFactory::GetInstance()->SetTestingFactoryAndUse(
        profile_.get(),
        base::BindRepeating(&AutocompleteClassifierFactory::BuildInstanceFor));

    client_ = std::make_unique<TestAutocompleteProviderClient>(
        profile_.get(), &test_url_loader_factory_);
    provider_ =
        base::MakeRefCounted<BraveSearchProvider>(client_.get(), nullptr);

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
    test_clipboard_ = ui::TestClipboard::CreateForCurrentThread();
#endif
  }

  void TearDown() override {
    // Shutdown the provider before the profile.
    provider_ = nullptr;
  }

 protected:
  // Adds a search for |term|, using the engine |t_url| to the history, and
  // returns the URL for that search.
  GURL AddSearchToHistory(TemplateURL* t_url,
                          std::u16string term,
                          int visit_count) {
    history::HistoryService* history = HistoryServiceFactory::GetForProfile(
        profile_.get(), ServiceAccessType::EXPLICIT_ACCESS);
    GURL search(t_url->url_ref().ReplaceSearchTerms(
        TemplateURLRef::SearchTermsArgs(term),
        TemplateURLServiceFactory::GetForProfile(profile_.get())
            ->search_terms_data()));
    last_added_time_ =
        std::max(base::Time::Now(), last_added_time_ + base::Microseconds(1));
    history->AddPageWithDetails(search, std::u16string(), visit_count,
                                visit_count, last_added_time_, false,
                                history::SOURCE_BROWSED);
    history->SetKeywordSearchTermsForURL(search, t_url->id(), term);
    return search;
  }

  // Looks for a match in |provider_| with destination |url|.  Sets |match| to
  // it if found.  Returns whether |match| was set.
  bool FindMatchWithDestination(const GURL& url, AutocompleteMatch* match) {
    for (const auto& i : provider_->matches()) {
      if (i.destination_url == url) {
        *match = i;
        return true;
      }
    }
    return false;
  }

  void QueryForInput(const std::u16string& text) {
    AutocompleteInput input(text, metrics::OmniboxEventProto::OTHER,
                            ChromeAutocompleteSchemeClassifier(profile_.get()));
    provider_->Start(input, false);

    // RunUntilIdle so that the task scheduled by SearchProvider to create the
    // URLFetchers runs.
    base::RunLoop().RunUntilIdle();
  }

  void QueryForInputAndSetWYTMatch(const std::u16string& text,
                                   AutocompleteMatch* wyt_match) {
    QueryForInput(text);

    profile_->BlockUntilHistoryProcessesPendingRequests();
    DCHECK(wyt_match);
    ASSERT_GE(provider_->matches().size(), 1u);
    EXPECT_TRUE(FindMatchWithDestination(
        GURL(default_t_url_->url_ref().ReplaceSearchTerms(
            TemplateURLRef::SearchTermsArgs(
                base::CollapseWhitespace(text, false)),
            TemplateURLServiceFactory::GetForProfile(profile_.get())
                ->search_terms_data())),
        wyt_match));
  }

  base::Time last_added_time_;

  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  variations::ScopedVariationsIdsProvider scoped_variations_ids_provider_{
      variations::VariationsIdsProvider::Mode::kUseSignedInState};
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::unique_ptr<TestAutocompleteProviderClient> client_;
  scoped_refptr<BraveSearchProvider> provider_;

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  raw_ptr<ui::TestClipboard> test_clipboard_ = nullptr;
#endif

  raw_ptr<TemplateURL> default_t_url_ = nullptr;

  // If not nullptr, OnProviderUpdate quits the current |run_loop_|.
  raw_ptr<base::RunLoop> run_loop_ = nullptr;
};

// Actual Tests
// ---------------------------------------------------------------

TEST_F(BraveSearchProviderTest, SearchIncludesHistoryWhenHistoryEnabled) {
  profile_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, true);

  GURL term_url_a(AddSearchToHistory(default_t_url_, u"hello", 1));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  AutocompleteMatch wyt_match;
  ASSERT_NO_FATAL_FAILURE(QueryForInputAndSetWYTMatch(u"hel", &wyt_match));
  ASSERT_EQ(2u, provider_->matches().size());
}

TEST_F(BraveSearchProviderTest,
       SearchDoesNotIncludeHistoryWhenHistoryDisabled) {
  profile_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, false);

  GURL term_url_a(AddSearchToHistory(default_t_url_, u"hello", 1));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  AutocompleteMatch wyt_match;
  ASSERT_NO_FATAL_FAILURE(QueryForInputAndSetWYTMatch(u"hel", &wyt_match));
  ASSERT_EQ(1u, provider_->matches().size());
}

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
// Check search suggestion is blocked when input and clipboard text is same.
TEST_F(BraveSearchProviderTest, DontSendClipboardTextToSuggest) {
  // Not blocked when clipboard cont is different from URL bar.
  test_clipboard_->WriteText("best browser");
  ASSERT_NO_FATAL_FAILURE(QueryForInput(u"brave"));
  EXPECT_TRUE(test_url_loader_factory_.IsPending(
      base::StrCat({kSuggestionUrlHost, "brave"})));

  // Blocked when clipboard content is the same.
  test_clipboard_->WriteText("brave_private");
  ASSERT_NO_FATAL_FAILURE(QueryForInput(u"brave_private"));
  EXPECT_FALSE(test_url_loader_factory_.IsPending(
      base::StrCat({kSuggestionUrlHost, "brave_private"})));

  // Blocked when clipboard content is the same after sanitizing.
  test_clipboard_->WriteText(" brave_private ");
  ASSERT_NO_FATAL_FAILURE(QueryForInput(u"brave_private "));
  EXPECT_FALSE(test_url_loader_factory_.IsPending(
      base::StrCat({kSuggestionUrlHost, "brave_private"})));
}
#endif

#if BUILDFLAG(ENABLE_STRICT_QUERY_CHECK_FOR_SEARCH_SUGGESTIONS)
TEST_F(BraveSearchProviderTest, SearchSuggestionsSendTest) {
  struct {
    std::string input;
    const bool expect_to_send_to_default_provider;
  } cases[] = {
      // Brave specific constraints.
      // Block long query longer than 50 chars.
      {"looooooooooooooooooooooooooong loooooooooooong query", false},
      {"https://www.amazon.de/Samsung-MLT-D101S-Toner-Cartridge-Black/dp/"
       "B006WARUYQ",
       false},
      // Block the query that has more than 7 words.
      {"many words in query a b c d e f g", false},
      // Block the query that has long number longer than 7.
      {"long number 12345611 in 892 query", false},
      // Pass the query that has short number.
      {"short number 12341 in query", true},
      // Block the query that has email address.
      {"a email@gmail.com b", false},
      // Block irc scheme.
      {"irc://bravenet", false},
      // Invalid html url.
      {"http://a asdfasdfasdfasdf", false},
  };

  for (size_t i = 0; i < std::size(cases); ++i) {
    ASSERT_NO_FATAL_FAILURE(QueryForInput(base::ASCIIToUTF16(cases[i].input)));
    EXPECT_EQ(cases[i].expect_to_send_to_default_provider,
              test_url_loader_factory_.IsPending(base::StrCat(
                  {kSuggestionUrlHost, base::EscapePath(cases[i].input)})));
  }
}

TEST(SearchSuggestionsTest, IsSuspiciousQueryTest) {
  const char* cases_ok[] = {
      "amazon",
      "bank of america",
      "bild",
      "craigslist",
      "ebay",
      "ebay kleinanzeigen",
      "facebook",
      "finance",
      "gmail",
      "gmx",
      "gmx.de",
      "google docs",
      "google drive",
      "google maps",
      "google translate",
      "maps",
      "netflix",
      "speed test",
      "paypal",
      "postbank",
      "t-online",
      "translate",
      "weather",
      "yahoo mail",
      "youtube",
      "Fu?ball",
      "ma? bier",
      "ma?krug",
      "c# book",
      "c# for dummies",
      "d#nisches bettenlager",
      "kleinanzeigen#",
      "to.be.true vs to.equal(true)",
      "chrome.runtime.id",
      "Yandex.Kit",
      "Node.Js",
      "org.apache.log4j.Logger upgrade",
      "http://a",
      "test query",
      "http://a asdfasdfasdfasdf",
      "http://sinonjs.test/releases/v4.0.0/spies/",
      "one two three four five six seven",
      "a 1234341 b 1234561",
      "seti@home",
      "a seti@home b",
  };
  for (size_t i = 0; i < std::size(cases_ok); ++i) {
    EXPECT_FALSE(search_suggestions::IsSuspiciousQuery(cases_ok[i]));
  }

  const char* cases_no[] = {
      "Dr. Strangelove or: How I Learned to Stop Worrying and Love the Bomb",
      "Intel NUC Kit Barebone NUC7I5BNH Intel Core i5-7260U, Intel Iris Plus "
      "Grafik 640, 2x DDR",
      "Install error - 0x80248007",
      "segfault at 0 ip 00007fb3cdf2afad sp 00007fb3cc2d7ae0 error 6 in "
      "libxul.so",
      "CPU0: Core temperature above threshold, cpu clock throttled (total "
      "events = 340569",
      "http://198.51.100.1/admin/foo/bar/?o=123456",
      "Inplacement - neue Mitarbeiter erfolgreich einarbeiten und integrieren "
      ": wie sie das Potenzial neuer Mitarbeiter erschließen und für ihr "
      "Unternehmen nutzbar machen; eine Arbeitshilfe für Führungskräfte / von ",
      "Mehrere Mütter kommentieren und bewerten eine Arbeit im weißen Raum, im "
      "Atelier des Künstlers Jonathan Meese, das zur mehrdimensionalen "
      "Leinwand wird. In der ersten Virtual-Reality-Produktion des Künstlers "
      "verschwimmen Wirklichkeit und Künstlermythos.",
      "An open label, randomized, two arm phase III study of nivolumab "
      "incombination with ipilimumab versus extreme study regimen as first "
      "linetherapy in recurrent or metastatic squamous cell carcinoma of the "
      "headand neck",
      "2014. The Business Value of Pro-cess Flexibility - An "
      "Optimization Model and its Application in the Service Sector.",
      "Those Magnificent Men in Their Flying Machines or How I Flew from "
      "London to Paris in 25 hours 11 minutes",
      "Critical dependency: require function is used in a way in which "
      "dependencies cannot be statically extracted",
      "Error:Android Source Generator: Error: Can't find bundle for base name "
      "messages.AndroidJpsBundle, locale "
      "de_DEjava.util.MissingResourceException: Can't find bundle for base "
      "name messages.AndroidJpsBundle, locale de_DEat java.ut",
      "one two three four five six seven eight",
      "a 1234341 b 12345611",
      "a 12343411 b 1234561",
      "seti@home.com",
      "a seti@home.com b",
  };
  for (size_t i = 0; i < std::size(cases_no); ++i) {
    EXPECT_TRUE(search_suggestions::IsSuspiciousQuery(cases_no[i]));
  }
}

TEST(SearchSuggestionsTest, IsSafeQueryUrlTest) {
  const char* cases_ok[] = {
      "amazon",
      "bank of america",
      "bild",
      "craigslist",
      "ebay",
      "ebay kleinanzeigen",
      "facebook",
      "finance",
      "gmail",
      "gmx",
      "gmx.de",
      "google docs",
      "google drive",
      "google maps",
      "google translate",
      "maps",
      "netflix",
      "speed test",
      "paypal",
      "postbank",
      "t-online",
      "translate",
      "weather",
      "yahoo mail",
      "youtube",
      "Fu?ball",
      "ma? bier",
      "ma?krug",
      "c# book",
      "c# for dummies",
      "d#nisches bettenlager",
      "kleinanzeigen#",
      "to.be.true vs to.equal(true)",
      "chrome.runtime.id",
      "Yandex.Kit",
      "Node.Js",
      "net.ipv4.tcp_tw_reuse",
      "org.apache.log4j.Logger upgrade",
      "http://a",
      "test query",
      "test query  \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t "
      "\t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t",
      "CPU0: Core temperature above threshold, cpu clock throttled (total "
      "events = 340569",
      "segfault at 0 ip 00007fb3cdf2afad sp 00007fb3cc2d7ae0 error 6 in "
      "libxul.so",
      "Install error - 0x80248007",
      "Intel NUC Kit Barebone NUC7I5BNH Intel Core i5-7260U, Intel Iris Plus "
      "Grafik 640, 2x DDR",
      "Dr. Strangelove or: How I Learned to Stop Worrying and Love the Bomb",
  };
  for (size_t i = 0; i < std::size(cases_ok); ++i) {
    EXPECT_TRUE(search_suggestions::IsSafeQueryUrl(cases_ok[i]));
  }

  const char* cases_no[] = {
      "https://github.test/cliqz/navigation-extension/pull/6200/commits/"
      "74f65ce53e5e163c7ec2770ba51470eaa8d24ca4",
      "https://eu-central-1.console.aws.amazon.test/console/"
      "home?region=eu-central-1#",
      "http://198.51.100.1/admin/foo/bar/?o=123456",
      "http://sinonjs.test/releases/v4.0.0/spies/",
      "Inplacement - neue Mitarbeiter erfolgreich einarbeiten und integrieren "
      ": wie sie das Potenzial neuer Mitarbeiter erschließen und für ihr "
      "Unternehmen nutzbar machen; eine Arbeitshilfe für Führungskräfte / von ",
      "Mehrere Mütter kommentieren und bewerten eine Arbeit im weißen Raum, im "
      "Atelier des Künstlers Jonathan Meese, das zur mehrdimensionalen "
      "Leinwand wird. In der ersten Virtual-Reality-Produktion des Künstlers "
      "verschwimmen Wirklichkeit und Künstlermythos.",
      "An open label, randomized, two arm phase III study of nivolumab "
      "incombination with ipilimumab versus extreme study regimen as first "
      "linetherapy in recurrent or metastatic squamous cell carcinoma of the "
      "headand neck",
      "2014. The Business Value of Pro-cess Flexibility - An "
      "Optimization Model and its Application in the Service Sector.",
      "Those Magnificent Men in Their Flying Machines or How I Flew from "
      "London to Paris in 25 hours 11 minutes",
      "Critical dependency: require function is used in a way in which "
      "dependencies cannot be statically extracted",
      "Error:Android Source Generator: Error: Can't find bundle for base name "
      "messages.AndroidJpsBundle, locale "
      "de_DEjava.util.MissingResourceException: Can't find bundle for base "
      "name messages.AndroidJpsBundle, locale de_DEat java.ut",
      "bit.ly/1h0ceQI",
  };
  for (size_t i = 0; i < std::size(cases_no); ++i) {
    EXPECT_FALSE(search_suggestions::IsSafeQueryUrl(cases_no[i]));
  }
}
#endif
