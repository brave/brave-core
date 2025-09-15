/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/tabs/tab_content_extractor.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/dom_distiller/core/extraction_utils.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/dom_distiller_js/dom_distiller.pb.h"
#include "third_party/dom_distiller_js/dom_distiller_json_converter.h"

namespace tab_content_extractor {

namespace {

// JavaScript to extract description meta tags with fallback order
constexpr char kDescriptionExtractionScript[] = R"(
  (function() {
    var description = '';

    // Try description meta tag first
    var descMeta = document.querySelector('meta[name="description"]');
    if (descMeta && descMeta.content) {
      description = descMeta.content.trim();
    }

    // Fallback to og:description
    if (!description) {
      var ogDesc = document.querySelector('meta[property="og:description"]');
      if (ogDesc && ogDesc.content) {
        description = ogDesc.content.trim();
      }
    }

    // Fallback to twitter:description
    if (!description) {
      var twitterDesc = document.querySelector('meta[name="twitter:description"]');
      if (twitterDesc && twitterDesc.content) {
        description = twitterDesc.content.trim();
      }
    }

    return description;
  })();
)";

// NavigationObserver that waits for page load completion before executing
// JavaScript
class ContentExtractionObserver : public content::WebContentsObserver {
 public:
  ContentExtractionObserver(
      content::WebContents* web_contents,
      int tab_index,
      base::OnceCallback<void(std::pair<int, ExtractedData>)> callback)
      : WebContentsObserver(web_contents),
        tab_index_(tab_index),
        callback_(std::move(callback)) {}

  void DocumentOnLoadCompletedInPrimaryMainFrame() override {
    VLOG(1) << "Tab " << tab_index_ << " load completed, executing JavaScript";
    ExecuteScript();
  }

  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override {
    VLOG(1) << "Tab " << tab_index_
            << " render process gone, calling callback with empty content";
    std::move(callback_).Run(std::make_pair(tab_index_, ExtractedData{}));
    delete this;
  }

  void StartExtraction() { ExecuteScript(); }

 private:
  // TODO(darkdh): Move script executing into renderer.
  void ExecuteScript() {
    if (!web_contents() || !web_contents()->GetPrimaryMainFrame() ||
        !web_contents()->GetPrimaryMainFrame()->IsRenderFrameLive()) {
      VLOG(1) << "Tab " << tab_index_
              << " not ready, calling callback with empty content";
      std::move(callback_).Run(std::make_pair(tab_index_, ExtractedData{}));
      delete this;
      return;
    }

    // Create distiller options to extract text only
    dom_distiller::proto::DomDistillerOptions options;
    options.set_extract_text_only(true);
    options.set_debug_level(0);  // No debug output

    // Get distiller script with text-only option
    std::string script = dom_distiller::GetDistillerScriptWithOptions(options);

    // First execute the distiller script
    web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
        base::UTF8ToUTF16(script),
        base::BindOnce(
            [](base::OnceCallback<void(std::pair<int, ExtractedData>)> callback,
               int tab_index, ContentExtractionObserver* observer,
               content::WebContents* web_contents, base::Value result) {
              ExtractedData extracted_data;

              // Use proper protobuf deserialization for content
              dom_distiller::proto::DomDistillerResult distiller_result;
              if (dom_distiller::proto::json::DomDistillerResult::ReadFromValue(
                      result, &distiller_result)) {
                if (distiller_result.has_distilled_content() &&
                    distiller_result.distilled_content().has_html()) {
                  extracted_data.content =
                      distiller_result.distilled_content().html();
                  VLOG(1) << "Tab " << tab_index
                          << " extracted content (length: "
                          << extracted_data.content.length() << ")";
                } else {
                  VLOG(2) << "Tab " << tab_index
                          << " - no distilled content or html found";
                }
              } else {
                VLOG(2) << "Tab " << tab_index << " - ReadFromValue failed";
              }

              // Now extract description from meta tags
              web_contents->GetPrimaryMainFrame()
                  ->ExecuteJavaScriptInIsolatedWorld(
                      base::UTF8ToUTF16(
                          std::string(kDescriptionExtractionScript)),
                      base::BindOnce(
                          [](base::OnceCallback<void(
                                 std::pair<int, ExtractedData>)> callback,
                             int tab_index, ContentExtractionObserver* observer,
                             ExtractedData extracted_data,
                             base::Value description_result) {
                            // Extract description from the result
                            if (description_result.is_string()) {
                              extracted_data.description =
                                  description_result.GetString();
                              VLOG(1) << "Tab " << tab_index
                                      << " extracted description: "
                                      << extracted_data.description;
                            } else {
                              VLOG(2) << "Tab " << tab_index
                                      << " - no description found";
                            }

                            std::move(callback).Run(
                                std::make_pair(tab_index, extracted_data));
                            delete observer;  // Clean up the observer
                          },
                          std::move(callback), tab_index, observer,
                          extracted_data),
                      ISOLATED_WORLD_ID_CHROME_INTERNAL);
            },
            std::move(callback_), tab_index_, this, web_contents()),
        ISOLATED_WORLD_ID_CHROME_INTERNAL);
  }

  int tab_index_;
  base::OnceCallback<void(std::pair<int, ExtractedData>)> callback_;
};

}  // namespace

void ExtractTextContent(
    content::WebContents* web_contents,
    const GURL& tab_url,
    int tab_index,
    base::OnceCallback<void(std::pair<int, ExtractedData>)> callback) {
  // Check if WebContents and frame are valid
  if (!web_contents || !web_contents->GetPrimaryMainFrame()) {
    VLOG(1) << "Tab " << tab_index
            << " has invalid WebContents or frame, skipping";
    std::move(callback).Run(std::make_pair(tab_index, ExtractedData{}));
    return;
  }

  // Check if the tab is loading
  if (web_contents->IsLoading()) {
    VLOG(1) << "Tab " << tab_index
            << " is still loading, skipping JavaScript execution";
    std::move(callback).Run(std::make_pair(tab_index, ExtractedData{}));
    return;
  }

  // Create the observer
  auto* observer = new ContentExtractionObserver(web_contents, tab_index,
                                                 std::move(callback));

  // Check if the tab needs to be loaded
  if (web_contents->WasDiscarded() ||
      !web_contents->GetPrimaryMainFrame()->IsRenderFrameLive()) {
    VLOG(1) << "Tab " << tab_index
            << " needs to be loaded, starting navigation to: "
            << tab_url.spec();

    // Start navigation and wait for completion
    content::NavigationController::LoadURLParams load_params(tab_url);
    web_contents->GetController().LoadURLWithParams(load_params);
  } else {
    // Tab is already alive, execute JavaScript immediately
    VLOG(1) << "Tab " << tab_index << " is already alive, executing JavaScript";
    observer->StartExtraction();
  }
}

}  // namespace tab_content_extractor
