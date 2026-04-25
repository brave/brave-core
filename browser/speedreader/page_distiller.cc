/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/page_distiller.h"

#include <string>
#include <utility>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/browser/web_contents.h"
#include "third_party/re2/src/re2/re2.h"

namespace speedreader {

PageDistiller::PageDistiller(content::WebContents* web_contents)
    : web_contents_(web_contents) {}

PageDistiller::~PageDistiller() = default;

PageDistiller::State PageDistiller::GetState() const {
  return state_;
}

void PageDistiller::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void PageDistiller::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void PageDistiller::GetDistilledHTML(DistillContentCallback callback) {
  StartDistill(base::BindOnce(&PageDistiller::AddStyleSheet,
                              weak_factory_.GetWeakPtr(), std::move(callback)));
}

void PageDistiller::GetDistilledText(DistillContentCallback callback) {
  StartDistill(base::BindOnce(&PageDistiller::ExtractText,
                              weak_factory_.GetWeakPtr(), std::move(callback)));
}

void PageDistiller::GetTextToSpeak(TextToSpeechContentCallback callback) {
  if (state_ != State::kDistilled) {
    return std::move(callback).Run(base::Value());
  }

  static constexpr char16_t kGetTextToSpeak[] =
      uR"js( speedreaderUtils.extractTextToSpeak() )js";

  web_contents_->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      kGetTextToSpeak,
      base::BindOnce(&PageDistiller::OnGetTextToSpeak,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void PageDistiller::UpdateState(State state) {
  state_ = state;
  for (auto& observer : observers_) {
    observer.OnPageDistillStateChanged(state_);
  }
}

void PageDistiller::SetWebContents(content::WebContents* web_contents) {
  web_contents_ = web_contents;
}

void PageDistiller::StartDistill(DistillContentCallback callback) {
  if (!web_contents_) {
    return std::move(callback).Run(false, {});
  }

  static constexpr char16_t kGetDocumentSource[] =
      uR"js( document.documentElement.outerHTML )js";

  static constexpr char16_t kGetBodySource[] =
      uR"js( document.body.outerHTML )js";

  web_contents_->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      (state_ != State::kDistilled) ? kGetDocumentSource : kGetBodySource,
      base::BindOnce(&PageDistiller::OnGetOuterHTML, weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void PageDistiller::OnGetOuterHTML(DistillContentCallback callback,
                                   base::Value result) {
  if (!web_contents_ || !result.is_string()) {
    return std::move(callback).Run(false, {});
  }
  if (state_ == State::kDistilled) {
    return std::move(callback).Run(true, std::move(result).TakeString());
  } else {
    auto* speedreader_service = SpeedreaderServiceFactory::GetForBrowserContext(
        web_contents_->GetBrowserContext());
    auto* speedreader_service_rewriter =
        g_brave_browser_process->speedreader_rewriter_service();
    if (!speedreader_service || !speedreader_service_rewriter) {
      return std::move(callback).Run(false, {});
    }

    DistillPage(
        web_contents_->GetLastCommittedURL(), std::move(result).TakeString(),
        speedreader_service, speedreader_service_rewriter,
        base::BindOnce(&PageDistiller::OnPageDistilled,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
  }
}

void PageDistiller::OnGetTextToSpeak(TextToSpeechContentCallback callback,
                                     base::Value result) {
  if (!result.is_dict()) {
    return std::move(callback).Run(base::Value());
  }
  std::move(callback).Run(std::move(result));
}

void PageDistiller::OnPageDistilled(DistillContentCallback callback,
                                    DistillationResult result,
                                    std::string original_data,
                                    std::string transformed) {
  if (!web_contents_ || result != DistillationResult::kSuccess) {
    return std::move(callback).Run(false, {});
  }

  return std::move(callback).Run(true, std::move(transformed));
}

void PageDistiller::AddStyleSheet(DistillContentCallback callback,
                                  bool success,
                                  std::string html_content) {
  auto* speedreader_service_rewriter =
      g_brave_browser_process->speedreader_rewriter_service();

  if (!success || !speedreader_service_rewriter || html_content.empty()) {
    return std::move(callback).Run(false, {});
  }

  std::move(callback).Run(true,
                          speedreader_service_rewriter->GetContentStylesheet() +
                              std::move(html_content));
}

void PageDistiller::ExtractText(DistillContentCallback callback,
                                bool success,
                                std::string html_content) {
  if (!success || html_content.empty()) {
    return std::move(callback).Run(false, {});
  }

  re2::RE2::GlobalReplace(&html_content, "<[^>]*>", " ");
  std::move(callback).Run(true, html_content);
}

}  // namespace speedreader
