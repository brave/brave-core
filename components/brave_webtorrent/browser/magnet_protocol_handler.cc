/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_webtorrent/browser/magnet_protocol_handler.h"

#include <optional>
#include <string>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#include "brave/components/constants/url_constants.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/weak_document_ptr.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "ui/base/page_transition_types.h"

namespace webtorrent {

namespace {

constexpr char kWebtorrentUsageHistogramName[] = "Brave.WebTorrent.UsageWeekly";

bool IsMagnetProtocol(const GURL& url) {
  return url.SchemeIs(kMagnetScheme);
}

void LoadMagnetURL(const GURL& url,
                   content::WebContents::Getter web_contents_getter,
                   ui::PageTransition page_transition,
                   bool has_user_gesture,
                   bool is_in_fenced_frame_tree,
                   const std::optional<url::Origin>& initiating_origin,
                   content::WeakDocumentPtr initiator_document) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents) {
    return;
  }

  DCHECK(IsMagnetProtocol(url));
  DCHECK(IsWebtorrentEnabled(web_contents->GetBrowserContext()));
  page_transition = ui::PageTransitionIsMainFrame(page_transition)
                        ? page_transition
                        : ui::PageTransition::PAGE_TRANSITION_AUTO_TOPLEVEL;
  web_contents->GetController().LoadURL(url, content::Referrer(),
                                        page_transition, std::string());
}

}  // namespace

GURL TranslateMagnetURL(const GURL& url) {
  GURL extension_page_url(base::StrCat(
      {extensions::kExtensionScheme, "://", brave_webtorrent_extension_id,
       "/extension/brave_webtorrent.html?%s"}));
  std::string translatedSpec(extension_page_url.spec());
  base::ReplaceFirstSubstringAfterOffset(
      &translatedSpec, 0, "%s", base::EscapeQueryParamValue(url.spec(), true));
  return GURL(translatedSpec);
}

GURL TranslateTorrentUIURLReversed(const GURL& url) {
  GURL translated_url(base::UnescapeURLComponent(
      url.query(),
      base::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
          base::UnescapeRule::PATH_SEPARATORS));
  GURL::Replacements replacements;
  replacements.SetRefStr(url.ref_piece());
  return GURL(
      base::StrCat({url::kWebTorrentScheme, ":",
                    translated_url.ReplaceComponents(replacements).spec()}));
}

bool HandleTorrentURLReverseRewrite(GURL* url,
                                    content::BrowserContext* browser_context) {
  if (url->SchemeIs(extensions::kExtensionScheme) &&
      url->host() == brave_webtorrent_extension_id &&
      url->ExtractFileName() == "brave_webtorrent.html" &&
      GURL(url->query()).SchemeIsHTTPOrHTTPS()) {
    *url = TranslateTorrentUIURLReversed(*url);
    return true;
  }

  return false;
}

bool HandleTorrentURLRewrite(GURL* url,
                             content::BrowserContext* browser_context) {
  if (!IsWebtorrentEnabled(browser_context)) {
    return false;
  }

  // The HTTP/HTTPS URL could be modified later by the network delegate if the
  // mime type matches or .torrent is in the path.
  // Handle http and https here for making reverse_on_redirect to be true in
  // BrowserURLHandlerImpl::RewriteURLIfNecessary to trigger ReverseURLRewrite
  // for updating the virtual URL.
  if (url->SchemeIsHTTPOrHTTPS() ||
      (url->SchemeIs(extensions::kExtensionScheme) &&
       url->host() == brave_webtorrent_extension_id &&
       url->ExtractFileName() == "brave_webtorrent.html" &&
       GURL(url->query()).SchemeIsHTTPOrHTTPS())) {
    return true;
  }

  return false;
}

bool HandleMagnetURLRewrite(GURL* url,
                            content::BrowserContext* browser_context) {
  if (IsWebtorrentEnabled(browser_context) && IsMagnetProtocol(*url)) {
    *url = TranslateMagnetURL(*url);
    return true;
  }

  return false;
}

bool HandleMagnetProtocol(const GURL& url,
                          content::WebContents::Getter web_contents_getter,
                          ui::PageTransition page_transition,
                          bool has_user_gesture,
                          bool is_in_fenced_frame_tree,
                          const std::optional<url::Origin>& initiating_origin,
                          content::WeakDocumentPtr initiator_document) {
  if (!IsMagnetProtocol(url)) {
    return false;
  }

  // Handle subframe magnet links only if a user gesture is present.
  if (!ui::PageTransitionIsMainFrame(page_transition) && !has_user_gesture) {
    return false;
  }

  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents ||
      !IsWebtorrentEnabled(web_contents->GetBrowserContext())) {
    return false;
  }

  UMA_HISTOGRAM_BOOLEAN(kWebtorrentUsageHistogramName, true);

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&LoadMagnetURL, url, web_contents_getter, page_transition,
                     has_user_gesture, is_in_fenced_frame_tree,
                     initiating_origin, std::move(initiator_document)));
  return true;
}

}  // namespace webtorrent
