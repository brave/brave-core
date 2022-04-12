/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_rewriter_service.h"

#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/speedreader/features.h"
#include "brave/components/speedreader/rust/ffi/speedreader.h"
#include "brave/components/speedreader/speedreader_component.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "components/grit/brave_components_resources.h"
#include "crypto/sha2.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace speedreader {

namespace {

std::string WrapStylesheetWithCSP(const std::string& stylesheet) {
  const std::string style_hash = crypto::SHA256HashString(stylesheet);
  const std::string style_hash_b64 =
      base::Base64Encode(base::as_bytes(base::make_span(style_hash)));

  return "<meta http-equiv=\"Content-Security-Policy\" content=\""
         "script-src 'none'; style-src 'sha256-" +
         style_hash_b64 +
         "'\">\n"
         "<style id=\"brave_speedreader_style\">" +
         stylesheet + "</style>";
}

std::string GetDistilledPageStylesheet(const base::FilePath& stylesheet_path) {
  std::string stylesheet;
  const bool success = base::ReadFileToString(stylesheet_path, &stylesheet);

  if (!success || stylesheet.empty()) {
    VLOG(1) << "Failed to read stylesheet from component: " << stylesheet_path;
    stylesheet = ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
        IDR_SPEEDREADER_STYLE_DESKTOP);
  }

  return WrapStylesheetWithCSP(stylesheet);
}

}  // namespace

SpeedreaderRewriterService::SpeedreaderRewriterService(
    brave_component_updater::BraveComponent::Delegate* delegate)
    : component_(new speedreader::SpeedreaderComponent(delegate)),
      speedreader_(new speedreader::SpeedReader) {
  // Load the built-in stylesheet as the default
  content_stylesheet_ = WrapStylesheetWithCSP(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_SPEEDREADER_STYLE_DESKTOP));

  // Check the paths from the component as observer may register
  // later than the paths were available in the component.
  const auto stylesheet_path = component_->GetStylesheetPath();
  if (!stylesheet_path.empty())
    OnStylesheetReady(stylesheet_path);

  component_->AddObserver(this);
}

SpeedreaderRewriterService::~SpeedreaderRewriterService() {
  component_->RemoveObserver(this);
}

void SpeedreaderRewriterService::OnStylesheetReady(const base::FilePath& path) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&GetDistilledPageStylesheet, path),
      base::BindOnce(&SpeedreaderRewriterService::OnLoadStylesheet,
                     weak_factory_.GetWeakPtr()));
}

bool SpeedreaderRewriterService::URLLooksReadable(const GURL& url) {
  // Only HTTP is readable.
  if (!url.SchemeIsHTTPOrHTTPS())
    return false;

  // @pes research has shown basically no landing pages are readable.
  if (!url.has_path() || url.path() == "/")
    return false;

  // TODO(keur): Once implemented, check against the "maybe-speedreadable"
  // list here.

  // Check URL against precompiled regexes
  return URLReadableHintExtractor::GetInstance()->HasHints(url);
}

std::unique_ptr<Rewriter> SpeedreaderRewriterService::MakeRewriter(
    const GURL& url) {
  auto rewriter =
      speedreader_->MakeRewriter(url.spec(), RewriterType::RewriterReadability);
  rewriter->SetMinOutLength(speedreader::kSpeedreaderMinOutLengthParam.Get());
  return rewriter;
}

const std::string& SpeedreaderRewriterService::GetContentStylesheet() {
  return content_stylesheet_;
}

void SpeedreaderRewriterService::OnLoadStylesheet(std::string stylesheet) {
  VLOG(2) << "Speedreader stylesheet loaded";
  content_stylesheet_ = stylesheet;
}

}  // namespace speedreader
