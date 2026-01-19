# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import itertools
import re
from datetime import datetime

# Usage:
#   To generate search_engine_constants.h, run:
#     python3 search_engine_constants.py | pbcopy
# This will copy the generated C++ header to your clipboard for easy pasting.

hosts = set()

entries = []

current_comment = None


def set_comment(comment):
    global current_comment
    current_comment = comment


def emit_with_query_param_name(engine_name, host, serp_url, query_param_name):
    if host not in hosts:
        hosts.add(host)
        entries.append((
            current_comment,
            (f'{{"{host}", {{"{engine_name}", '
             f'"{serp_url}", "{query_param_name}"}}}}'),
        ))


def emit(engine_name, host, serp_url_regex):
    if host not in hosts:
        hosts.add(host)
        entries.append(
            (current_comment,
             f'{{"{host}", {{"{engine_name}", "{serp_url_regex}", {{}}}}}}'))


set_comment("Brave")
emit_with_query_param_name("Brave", "search.brave.com",
                           "https://search.brave.com/search", "q")

set_comment("Google")
google_tlds = [
    "ac", "ad", "ae", "al", "am", "as", "at", "az", "ba", "be", "bf", "bg",
    "bi", "bj", "bs", "bt", "ca", "cat", "cd", "cf", "cg", "ch", "ci", "cl",
    "cm", "cn", "co.bw", "co.ck", "co.cr", "co.id", "co.il", "co.im", "co.in",
    "co.je", "co.jp", "co.ke", "co.kr", "co.ls", "co.ma", "co.mz", "co.nz",
    "co.th", "co.tz", "co.ug", "co.uk", "co.uz", "co.ve", "co.vi", "co.za",
    "co.zm", "co.zw", "com", "com.af", "com.ag", "com.ai", "com.ar", "com.au",
    "com.bd", "com.bh", "com.bn", "com.bo", "com.br", "com.by", "com.bz",
    "com.co", "com.cu", "com.cy", "com.do", "com.ec", "com.eg", "com.et",
    "com.fj", "com.gh", "com.gi", "com.gt", "com.hk", "com.jm", "com.kg",
    "com.kh", "com.kw", "com.lb", "com.ly", "com.mt", "com.mx", "com.my",
    "com.na", "com.nf", "com.ng", "com.ni", "com.np", "com.om", "com.pa",
    "com.pe", "com.pg", "com.ph", "com.pk", "com.pr", "com.py", "com.qa",
    "com.sa", "com.sb", "com.sg", "com.sl", "com.sv", "com.tj", "com.tr",
    "com.tw", "com.ua", "com.uy", "com.vc", "com.vn", "cv", "cz", "de", "dj",
    "dk", "dm", "dz", "ee", "es", "fi", "fm", "fr", "ga", "ge", "gg", "gl",
    "gm", "gp", "gr", "gy", "hn", "hr", "ht", "hu", "ie", "iq", "is", "it",
    "it.ao", "jo", "ki", "kz", "la", "li", "lk", "lt", "lu", "lv", "md", "me",
    "mg", "mk", "ml", "mn", "ms", "mu", "mv", "mw", "ne", "nl", "no", "nr",
    "nu", "pl", "pn", "ps", "pt", "ro", "rs", "ru", "rw", "sc", "se", "sh",
    "si", "sk", "sm", "sn", "so", "sr", "st", "td", "tg", "tk", "tl", "tm",
    "tn", "to", "tt", "vg", "vu", "ws"
]
for tld in google_tlds:
    emit_with_query_param_name("Google", f"www.google.{tld}",
                               f"https://www.google.{tld}/search", "q")

set_comment("DuckDuckGo")
emit_with_query_param_name("DuckDuckGo", "duckduckgo.com",
                           "https://duckduckgo.com/", "q")

set_comment("Qwant")
emit_with_query_param_name("Qwant", "www.qwant.com", "https://www.qwant.com/",
                           "q")

set_comment("Bing")
emit_with_query_param_name("Bing", "www.bing.com",
                           "https://www.bing.com/search", "q")

set_comment("Startpage")
emit("Startpage", "www.startpage.com", "https://www.startpage.com/sp/search")

set_comment("Ecosia")
emit_with_query_param_name("Ecosia", "www.ecosia.org",
                           "https://www.ecosia.org/search", "q")

set_comment("Ask")
emit_with_query_param_name("Ask", "www.ask.com", "https://www.ask.com/web",
                           "q")

set_comment("Baidu")
emit_with_query_param_name("Baidu", "www.baidu.com", "https://www.baidu.com/s",
                           "wd")

set_comment("ChatGPT")
emit("ChatGPT", "chatgpt.com", "https://chatgpt.com/c/.*")

set_comment("CocCoc")
emit_with_query_param_name("CocCoc", "coccoc.com", "https://coccoc.com/search",
                           "query")

set_comment("Daum")
emit_with_query_param_name("Daum", "search.daum.net",
                           "https://search.daum.net/search", "q")

set_comment("Dogpile")
emit_with_query_param_name("Dogpile", "www.dogpile.com",
                           "https://www.dogpile.com/serp", "q")

set_comment("Excite")
emit_with_query_param_name("Excite", "results.excite.com",
                           "https://results.excite.com/serp", "q")
emit_with_query_param_name("Excite", "search.excite.com",
                           "https://search.excite.com/serp", "q")
emit_with_query_param_name("Excite", "www.excite.com",
                           "https://www.excite.com/", "dummy")

set_comment("Fireball")
emit_with_query_param_name("Fireball", "fireball.com",
                           "https://fireball.com/search/", "q")

set_comment("Freespoke")
emit_with_query_param_name("Freespoke", "freespoke.com",
                           "https://freespoke.com/search/web", "q")

set_comment("Info.com")
emit_with_query_param_name("info.com", "www.info.com",
                           "https://www.info.com/serp", "q")
emit_with_query_param_name("info.com", "info.com", "https://info.com/serp",
                           "dummy")

set_comment("Kagi")
emit_with_query_param_name("Kagi", "kagi.com", "https://kagi.com/search", "q")

set_comment("Karma Search")
emit_with_query_param_name("Karma Search", "karmasearch.org",
                           "https://karmasearch.org/search", "q")

set_comment("Lilo")
emit_with_query_param_name("Lilo", "search.lilo.org",
                           "https://search.lilo.org/", "q")

set_comment("Metacrawler")
emit_with_query_param_name("Metacrawler", "www.metacrawler.com",
                           "https://www.metacrawler.com/serp", "q")

set_comment("Mail.ru")
emit_with_query_param_name("Mail.ru", "mail.ru", "https://mail.ru/search",
                           "search_source")

set_comment("Mojeek")
emit_with_query_param_name("Mojeek", "www.mojeek.com",
                           "https://www.mojeek.com/search", "q")

set_comment("Naver")
emit_with_query_param_name("Naver", "search.naver.com",
                           "https://search.naver.com/search.naver", "query")
emit_with_query_param_name("Naver", "www.naver.com",
                           "https://www.naver.com/search", "dummy")

set_comment("Nona")
emit_with_query_param_name("Nona", "www.nona.de", "https://www.nona.de/", "q")

set_comment("Perplexity")
emit("Perplexity", "www.perplexity.ai",
     "https://www.perplexity.ai/search/([^/]+)")

set_comment("PrivacyWall")
emit_with_query_param_name("PrivacyWall", "www.privacywall.org",
                           "https://www.privacywall.org/search/secure", "q")

set_comment("Quendu")
emit_with_query_param_name("Quendu", "quendu.com", "https://quendu.com/search",
                           "q")

set_comment("Seznam")
emit_with_query_param_name("Seznam", "search.seznam.cz",
                           "https://search.seznam.cz/", "q")

set_comment("360 Search")
emit_with_query_param_name("360 Search", "www.so.com", "https://www.so.com/s",
                           "q")

set_comment("Sogou")
emit_with_query_param_name("Sogou", "www.sogou.com",
                           "https://www.sogou.com/web", "query")

set_comment("WebCrawler")
emit_with_query_param_name("WebCrawler", "www.webcrawler.com",
                           "https://www.webcrawler.com/serp", "q")

set_comment("Yahoo")
yahoo_search_country_codes = [
    "ar", "at", "au", "br", "ca", "ch", "cl", "co", "de", "dk", "emea", "es",
    "fi", "fr", "hk", "id", "in", "it", "mx", "malaysia", "nl", "nz", "pe",
    "ph", "se", "sg", "th", "tr", "tw", "uk"
]
for country_code in yahoo_search_country_codes:
    domain = f"{country_code}.search.yahoo.com"
    emit_with_query_param_name("Yahoo", domain, f"https://{domain}/search.*",
                               "p")

emit_with_query_param_name("Yahoo", "search.yahoo.com",
                           "https://search.yahoo.com/search.*", "p")

set_comment("Yahoo! JAPAN")
emit_with_query_param_name("Yahoo! JAPAN", "search.yahoo.co.jp",
                           "https://search.yahoo.co.jp/search.*", "p")
emit_with_query_param_name("Yahoo! JAPAN", "www.yahoo.co.jp",
                           "https://www.yahoo.co.jp", "dummy")

set_comment("Yandex")
yandex_tlds = ["by", "com", "kz", "ru", "com.tr"]
for tld in yandex_tlds:
    domain = f"yandex.{tld}"
    emit_with_query_param_name("Yandex", domain, f"https://{domain}/search/",
                               "text")

set_comment("Yep")
emit_with_query_param_name("Yep", "yep.com", "https://yep.com/web", "q")

set_comment("You.com")
emit_with_query_param_name("You.com", "you.com", "https://you.com/search", "q")

current_year = datetime.now().year

print(
    f"""/* Copyright (c) {current_year} The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SEARCH_ENGINE_CONSTANTS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SEARCH_ENGINE_CONSTANTS_H_

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "brave/components/serp_metrics/search_engine_info.h"

namespace metrics {{

// AUTO-GENERATED FILE: This file is generated by the script
//   `generate_search_engine_constants.py`
// Do NOT edit this file manually. Any changes will be overwritten. Modify the
// script, rerun it, and replace the contents of this file with the output.
//
// Used to determine whether a URL corresponds to a search engine home page or a
// search results page.

inline constexpr auto kSearchEngines =
    base::MakeFixedFlatMap</*host*/ std::string_view, SearchEngineInfo>(
        {{
""")

last_comment = None
for i, (entry_comment, entry) in enumerate(entries):
    if entry_comment != last_comment:
        print(f"            /* {entry_comment} */")
        last_comment = entry_comment
    comma = "," if i < len(entries) - 1 else ""
    print("            " + entry + comma)

print("""        }});

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SEARCH_ENGINE_CONSTANTS_H_
""")
