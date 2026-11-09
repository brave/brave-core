/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_engines/ntp_search_engine_utils.h"

#include "base/strings/utf_string_conversions.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "url/gurl.h"

namespace ntp_search {

bool IsEligibleNtpSearchEngine(const TemplateURL& engine,
                               const TemplateURLService& service) {
  const bool is_prepopulated =
      engine.GetBuiltinEngineType() ==
      BuiltinEngineType::KEYWORD_MODE_PREPOPULATED_ENGINE;
  // Discovery alone does not opt an engine into the widget.
  if (engine.type() != TemplateURL::NORMAL ||
      engine.data().starter_pack_id != 0 ||
      (!is_prepopulated &&
       engine.is_active() != TemplateURLData::ActiveStatus::kTrue) ||
      !engine.search_url_post_params().empty() ||
      !engine.SupportsReplacement(service.search_terms_data())) {
    return false;
  }
  const GURL url(engine.GenerateSearchURL(service.search_terms_data()));
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS();
}

TemplateURL* FindNtpSearchEngine(std::string_view id,
                                 TemplateURLService& service) {
  if (id.starts_with("keyword:")) {
    auto* engine =
        service.GetTemplateURLForKeyword(base::UTF8ToUTF16(id.substr(8)));
    return engine && IsEligibleNtpSearchEngine(*engine, service) ? engine
                                                                 : nullptr;
  }
  // Host IDs belong to built-ins. Resolve from the eligible list so starter
  // packs and custom engines sharing that host cannot shadow the selection.
  for (TemplateURL* engine : service.GetTemplateURLs()) {
    if (engine->prepopulate_id() == 0 ||
        !IsEligibleNtpSearchEngine(*engine, service)) {
      continue;
    }
    if (id == GURL(engine->url()).host() ||
        id == engine->GenerateSearchURL(service.search_terms_data()).host()) {
      return engine;
    }
  }
  return nullptr;
}

}  // namespace ntp_search
