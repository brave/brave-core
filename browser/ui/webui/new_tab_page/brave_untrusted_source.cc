// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_untrusted_source.h"

#include <unordered_map>
#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/common/url_constants.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/template_expressions.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace {

constexpr int kMaxUriDecodeLen = 2048;

std::string FormatTemplate(int resource_id,
                           const ui::TemplateReplacements& replacements) {
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  scoped_refptr<base::RefCountedMemory> bytes =
      bundle.LoadDataResourceBytes(resource_id);
  base::StringPiece string_piece(reinterpret_cast<const char*>(bytes->front()),
                                 bytes->size());
  return ui::ReplaceTemplateExpressions(
      string_piece, replacements,
      /* skip_unexpected_placeholder_check= */ true);
}

}  // namespace

BraveUntrustedSource::~BraveUntrustedSource() = default;

std::string BraveUntrustedSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) {
  if (directive == network::mojom::CSPDirectiveName::FrameAncestors) {
    return base::StringPrintf("frame-ancestors %s %s",
                              chrome::kChromeUINewTabPageURL,
                              chrome::kChromeUINewTabURL);
  }

  return UntrustedSource::GetContentSecurityPolicy(directive);
}

void BraveUntrustedSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback callback) {
  const std::string path = url.has_path() ? url.path().substr(1) : "";
  if (path == "brave_custom_background_image") {
    // Parse all query parameters to hash map and decode values.
    std::unordered_map<std::string, std::string> params;
    url::Component query(0, url.query().length());
    url::Component key, value;
    while (
        url::ExtractQueryKeyValue(url.query().c_str(), &query, &key, &value)) {
      url::RawCanonOutputW<kMaxUriDecodeLen> output;
      url::DecodeURLEscapeSequences(
          url.query().c_str() + value.begin, value.len,
          url::DecodeURLMode::kUTF8OrIsomorphic, &output);
      params.insert(
          {url.query().substr(key.begin, key.len),
           base::UTF16ToUTF8(std::u16string(output.data(), output.length()))});
    }
    ui::TemplateReplacements replacements;
    replacements["url"] = params["url"];
    std::string html = FormatTemplate(
        IDR_BRAVE_NEW_TAB_CUSTOM_BACKGROUND_IMAGE_HTML, replacements);
    std::move(callback).Run(base::RefCountedString::TakeString(&html));
    return;
  }

  if (path == "custom_background_image.js") {
    ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
    std::move(callback).Run(bundle.LoadDataResourceBytes(
        IDR_BRAVE_NEW_TAB_CUSTOM_BACKGROUND_IMAGE_JS));
    return;
  }

  UntrustedSource::StartDataRequest(url, wc_getter, std::move(callback));
}

bool BraveUntrustedSource::ShouldServiceRequest(
    const GURL& url,
    content::BrowserContext* browser_context,
    int render_process_id) {
  if (!url.SchemeIs(content::kChromeUIUntrustedScheme) || !url.has_path()) {
    return false;
  }
  const std::string path = url.path().substr(1);
  if (path == "brave_custom_background_image" ||
      path == "custom_background_image.js") {
    return true;
  }

  return UntrustedSource::ShouldServiceRequest(url, browser_context,
                                               render_process_id);
}
