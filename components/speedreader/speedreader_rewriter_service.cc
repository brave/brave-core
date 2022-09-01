/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_rewriter_service.h"

#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/files/file_path_watcher.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/task/bind_post_task.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/common/url_readable_hints.h"
#include "brave/components/speedreader/rust/ffi/speedreader.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "components/grit/brave_components_resources.h"
#include "crypto/sha2.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace speedreader {

namespace {

constexpr const char kSpeedreaderStylesheet[] = "speedreader-stylesheet";

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
    VLOG(1) << "Failed to read speedreader override stylesheet from "
            << stylesheet_path;
    stylesheet = ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
        IDR_SPEEDREADER_STYLE_DESKTOP);
  }

  base::ReplaceChars(stylesheet, "\r\n", "\n", &stylesheet);

  return WrapStylesheetWithCSP(stylesheet);
}

base::FilePathWatcher* CreateAndStartFilePathWatcher(
    const base::FilePath& watch_path,
    const base::FilePathWatcher::Callback& callback) {
  auto watcher = std::make_unique<base::FilePathWatcher>();
  if (!watcher->Watch(watch_path, base::FilePathWatcher::Type::kNonRecursive,
                      callback)) {
    return nullptr;
  }

  return watcher.release();
}

}  // namespace

SpeedreaderRewriterService::SpeedreaderRewriterService()
    : speedreader_(new speedreader::SpeedReader) {
  // Load the built-in stylesheet as the default
  content_stylesheet_ = WrapStylesheetWithCSP(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_SPEEDREADER_STYLE_DESKTOP));

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kSpeedreaderStylesheet)) {
    stylesheet_override_path_ =
        base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
            kSpeedreaderStylesheet);
    watch_task_runner_ =
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()});

    // Manually trigger the initial stylesheet load
    OnFileChanged(stylesheet_override_path_, false);

    base::PostTaskAndReplyWithResult(
        watch_task_runner_.get(), FROM_HERE,
        base::BindOnce(
            &CreateAndStartFilePathWatcher, stylesheet_override_path_,
            base::BindPostTask(
                base::SequencedTaskRunnerHandle::Get(),
                base::BindRepeating(&SpeedreaderRewriterService::OnFileChanged,
                                    weak_factory_.GetWeakPtr()))),
        base::BindOnce(&SpeedreaderRewriterService::OnWatcherStarted,
                       weak_factory_.GetWeakPtr()));
  }
}

SpeedreaderRewriterService::~SpeedreaderRewriterService() {
  if (file_watcher_) {
    watch_task_runner_->DeleteSoon(FROM_HERE, file_watcher_);
  }
}

void SpeedreaderRewriterService::OnFileChanged(const base::FilePath& path,
                                               bool error) {
  DCHECK_EQ(path, stylesheet_override_path_);
  if (!error) {
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&GetDistilledPageStylesheet, path),
        base::BindOnce(&SpeedreaderRewriterService::OnLoadStylesheet,
                       weak_factory_.GetWeakPtr()));
  }
}

void SpeedreaderRewriterService::OnWatcherStarted(
    base::FilePathWatcher* file_watcher) {
  file_watcher_ = file_watcher;
}

void SpeedreaderRewriterService::OnLoadStylesheet(std::string stylesheet) {
  VLOG(2) << "Speedreader stylesheet loaded";
  content_stylesheet_ = stylesheet;
}

bool SpeedreaderRewriterService::URLLooksReadable(const GURL& url) {
  // TODO(keur): Once implemented, check against the "maybe-speedreadable"
  // list here.

  // Check URL against precompiled regexes
  return IsURLLooksReadable(url);
}

std::unique_ptr<Rewriter> SpeedreaderRewriterService::MakeRewriter(
    const GURL& url,
    const std::string& theme) {
  auto rewriter =
      speedreader_->MakeRewriter(url.spec(), RewriterType::RewriterReadability);
  rewriter->SetMinOutLength(speedreader::kSpeedreaderMinOutLengthParam.Get());
  rewriter->SetTheme(theme);
  return rewriter;
}

const std::string& SpeedreaderRewriterService::GetContentStylesheet() {
  return content_stylesheet_;
}

}  // namespace speedreader
