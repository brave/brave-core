/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/search_engines/brave_prepopulated_engines.h"

#include "build/build_config.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/search_engine_type.h"

namespace TemplateURLPrepopulateData {

// IMPORTANT! Make sure to bump this value if you make changes to the
// engines below or add/remove engines.
const int kBraveCurrentDataVersion = 30;

// The version is important to increment because Chromium will cache the list
// of search engines that are shown. When the version is incremented, Chromium
// will conditionally merge changes from the new version of the list.
//
// For more info, see:
// https://source.chromium.org/chromium/chromium/src/+/main:components/search_engines/util.cc;l=53-125;drc=17b1d05d2ccda19c3ebd903075227bc8e851acf0

// DO NOT CHANGE THIS ONE. Used for backfilling kBraveDefaultSearchVersion.
const int kBraveFirstTrackedDataVersion = 6;

namespace {

PrepopulatedEngine MakeBravePrepopulatedEngine(const char16_t* const name,
                                               const char16_t* const keyword,
                                               const char* const favicon_url,
                                               const char* const search_url,
                                               const char* const encoding,
                                               const char* const suggest_url,
                                               SearchEngineType type,
                                               const int id) {
  return {name,    keyword, favicon_url, search_url, encoding, suggest_url,
          nullptr, nullptr, nullptr,     nullptr,    nullptr,  nullptr,
          nullptr, nullptr, nullptr,     nullptr,    nullptr,  nullptr,
          nullptr, nullptr, nullptr,     0,          nullptr,  0,
          type,    nullptr, nullptr,     id};
}

// Maps BravePrepopulatedEngineID to Chromium's PrepopulatedEngine.
const std::map<BravePrepopulatedEngineID, const PrepopulatedEngine*>
    brave_engines_map = {
        {PREPOPULATED_ENGINE_ID_GOOGLE, &google},
        {PREPOPULATED_ENGINE_ID_YANDEX, &brave_yandex},
        {PREPOPULATED_ENGINE_ID_BING, &brave_bing},
        {PREPOPULATED_ENGINE_ID_NAVER, &naver},
        {PREPOPULATED_ENGINE_ID_DAUM, &daum},
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO, &duckduckgo},
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE, &duckduckgo_de},
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE, &duckduckgo_au_nz_ie},
        {PREPOPULATED_ENGINE_ID_QWANT, &qwant},
        {PREPOPULATED_ENGINE_ID_STARTPAGE, &startpage},
        {PREPOPULATED_ENGINE_ID_ECOSIA, &brave_ecosia},
        {PREPOPULATED_ENGINE_ID_BRAVE, &brave_search},
};

PrepopulatedEngine ModifyEngineParams(const PrepopulatedEngine& engine,
                                      const char16_t* const name,
                                      const char16_t* const keyword,
                                      const char* const search_url,
                                      const char* const suggest_url,
                                      const char* const image_url,
                                      int id) {
  return {name ? name : engine.name,
          keyword ? keyword : engine.keyword,
          engine.favicon_url,
          search_url ? search_url : engine.search_url,
          engine.encoding,
          suggest_url ? suggest_url : engine.suggest_url,
          image_url ? image_url : engine.image_url,
          engine.image_translate_url,
          engine.new_tab_url,
          engine.contextual_search_url,
          engine.logo_url,
          engine.doodle_url,
          engine.search_url_post_params,
          engine.suggest_url_post_params,
          engine.image_url_post_params,
          engine.side_search_param,
          engine.side_image_search_param,
          engine.image_translate_source_language_param_key,
          engine.image_translate_target_language_param_key,
          engine.image_search_branding_label,
          engine.search_intent_params,
          engine.search_intent_params_size,
          engine.alternate_urls,
          engine.alternate_urls_size,
          engine.type,
          engine.preconnect_to_search_url,
          engine.prefetch_likely_navigations,
          id > 0 ? id : engine.id};
}

}  // namespace

const PrepopulatedEngine duckduckgo = MakeBravePrepopulatedEngine(
    u"DuckDuckGo",
    u":d",
    "https://duckduckgo.com/favicon.ico",
    "https://duckduckgo.com/?q={searchTerms}&t=brave",
    "UTF-8",
    "https://ac.duckduckgo.com/ac/?q={searchTerms}&type=list",
    SEARCH_ENGINE_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO);

const PrepopulatedEngine duckduckgo_de =
    ModifyEngineParams(duckduckgo,
                       nullptr,
                       nullptr,
                       "https://duckduckgo.com/?q={searchTerms}&t=bravened",
                       nullptr,
                       nullptr,
                       PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE);

const PrepopulatedEngine duckduckgo_au_nz_ie =
    ModifyEngineParams(duckduckgo,
                       nullptr,
                       nullptr,
                       "https://duckduckgo.com/?q={searchTerms}&t=braveed",
                       nullptr,
                       nullptr,
                       PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE);

#if BUILDFLAG(IS_ANDROID)
const PrepopulatedEngine duckduckgo_lite = MakeBravePrepopulatedEngine(
    u"DuckDuckGo Lite",
    u":dl",
    "https://duckduckgo.com/favicon.ico",
    "https://duckduckgo.com/lite/?q={searchTerms}&t=brave",
    "UTF-8",
    "https://ac.duckduckgo.com/ac/?q={searchTerms}&type=list",
    SEARCH_ENGINE_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE);
#endif  // BUILDFLAG(IS_ANDROID)

const PrepopulatedEngine brave_ecosia =
    ModifyEngineParams(ecosia,
                       nullptr,
                       u":e",
                       "https://www.ecosia.org/search?tt="
#if BUILDFLAG(IS_ANDROID)
                       "42b8ae98"
#else
                       "e8eb07a6"
#endif
                       "&q={searchTerms}&addon=brave",
                       "https://ac.ecosia.org/?q={searchTerms}",
                       nullptr,
                       PREPOPULATED_ENGINE_ID_ECOSIA);

const PrepopulatedEngine qwant = MakeBravePrepopulatedEngine(
    u"Qwant",
    u":q",
    "https://www.qwant.com/favicon.ico",
    "https://www.qwant.com/?q={searchTerms}&client=brz-brave",
    "UTF-8",
    "https://api.qwant.com/api/suggest/?q={searchTerms}&client=opensearch",
    SEARCH_ENGINE_QWANT,
    PREPOPULATED_ENGINE_ID_QWANT);

const PrepopulatedEngine startpage = MakeBravePrepopulatedEngine(
    u"Startpage",
    u":sp",
    "https://www.startpage.com/favicon.ico",
    "https://www.startpage.com/do/"
    "search?q={searchTerms}&segment=startpage.brave",
    "UTF-8",
    "https://www.startpage.com/cgi-bin/"
    "csuggest?query={searchTerms}&limit=10&format=json",
    SEARCH_ENGINE_OTHER,
    PREPOPULATED_ENGINE_ID_STARTPAGE);

const PrepopulatedEngine brave_yandex =
    ModifyEngineParams(yandex_com,
                       u"Yandex",
                       nullptr,
                       "https://yandex.ru/search/?clid="
#if BUILDFLAG(IS_ANDROID)
                       "2423859"
#else
                       "2353835"
#endif
                       "&text={searchTerms}",
                       "https://suggest.yandex.ru/suggest-ff.cgi?"
                       "part={searchTerms}&v=3&sn=5&srv=brave_desktop",
                       nullptr,
                       PREPOPULATED_ENGINE_ID_YANDEX);

const PrepopulatedEngine brave_search = MakeBravePrepopulatedEngine(
    u"Brave",
    u":br",
    "https://cdn.search.brave.com/serp/favicon.ico",
    "https://search.brave.com/search?q={searchTerms}&source="
#if BUILDFLAG(IS_ANDROID)
    "android",
#else
    "desktop",
#endif
    "UTF-8",
    "https://search.brave.com/api/"
    "suggest?q={searchTerms}&rich=true&source="
#if BUILDFLAG(IS_ANDROID)
    "android",
#else
    "desktop",
#endif
    SEARCH_ENGINE_OTHER,
    PREPOPULATED_ENGINE_ID_BRAVE);

const PrepopulatedEngine brave_search_tor = ModifyEngineParams(
    brave_search,
    nullptr,
    u":search.brave4u7jddbv7cyviptqjc7jusxh72uik7zt6adtckl5f4nwy2v72qd.onion",
    "https://"
    "search.brave4u7jddbv7cyviptqjc7jusxh72uik7zt6adtckl5f4nwy2v72qd.onion/"
    "search?q={searchTerms}",
    "https://"
    "search.brave4u7jddbv7cyviptqjc7jusxh72uik7zt6adtckl5f4nwy2v72qd.onion/api/"
    "suggest?q={searchTerms}",
    nullptr,
    PREPOPULATED_ENGINE_ID_BRAVE_TOR);

const PrepopulatedEngine brave_bing = ModifyEngineParams(
    bing,
    u"Bing",
    nullptr,
    "https://www.bing.com/search?q={searchTerms}",
    "https://www.bing.com/osjson.aspx?query={searchTerms}&language={language}",
    "https://www.bing.com/images/detail/search?iss=sbiupload#enterInsights",
    PREPOPULATED_ENGINE_ID_BING);

const std::map<BravePrepopulatedEngineID, const PrepopulatedEngine*>&
GetBraveEnginesMap() {
  return brave_engines_map;
}

}  // namespace TemplateURLPrepopulateData
