// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/file_text_extractor_base.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

namespace {

constexpr base::TimeDelta kExtractionTimeout = base::Seconds(30);

std::optional<base::FilePath> WriteBytesToTempFile(
    std::vector<uint8_t> file_bytes,
    base::FilePath::StringType extension) {
  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir)) {
    return std::nullopt;
  }
  base::FilePath temp_path;
  if (!base::CreateTemporaryFileInDir(temp_dir, &temp_path)) {
    return std::nullopt;
  }
  base::FilePath final_path = temp_path.AddExtension(extension);
  if (!base::Move(temp_path, final_path)) {
    base::DeleteFile(temp_path);
    return std::nullopt;
  }
  if (!base::WriteFile(final_path, file_bytes)) {
    base::DeleteFile(final_path);
    return std::nullopt;
  }
  return final_path;
}

void DeleteTempFile(const base::FilePath& path) {
  if (!path.empty()) {
    base::DeleteFile(path);
  }
}

}  // namespace

FileTextExtractorBase::FileTextExtractorBase() = default;

FileTextExtractorBase::~FileTextExtractorBase() {
  Cleanup();
}

void FileTextExtractorBase::ExtractText(
    content::BrowserContext* browser_context,
    const base::FilePath& file_path,
    ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);
  LoadInWebContents(browser_context, file_path);
}

void FileTextExtractorBase::ExtractText(
    content::BrowserContext* browser_context,
    std::vector<uint8_t> file_bytes,
    const base::FilePath::StringType& extension,
    ExtractTextCallback callback) {
  CHECK(!callback_) << "ExtractText called while extraction in progress";
  callback_ = std::move(callback);
  WriteTempFileAndLoad(browser_context, std::move(file_bytes), extension);
}

network::mojom::WebSandboxFlags
FileTextExtractorBase::AdditionalUnsandboxFlags() const {
  return network::mojom::WebSandboxFlags::kNone;
}

GURL FileTextExtractorBase::GetLoadURL(const base::FilePath& file_path) const {
  return net::FilePathToFileURL(file_path);
}

void FileTextExtractorBase::LoadInWebContents(
    content::BrowserContext* browser_context,
    const base::FilePath& file_path) {
  content::WebContents::CreateParams create_params(browser_context);
  create_params.is_never_composited = true;
  create_params.starting_sandbox_flags =
      network::mojom::WebSandboxFlags::kAll &
      ~network::mojom::WebSandboxFlags::kScripts &
      ~network::mojom::WebSandboxFlags::kOrigin &
      ~network::mojom::WebSandboxFlags::kNavigation &
      ~AdditionalUnsandboxFlags();
  web_contents_ = content::WebContents::Create(create_params);
  web_contents_->SetOwnerLocationForDebug(FROM_HERE);
  web_contents_->SetDelegate(this);
  Observe(web_contents_.get());

  timeout_timer_.Start(FROM_HERE, kExtractionTimeout,
                       base::BindOnce(&FileTextExtractorBase::OnTimeout,
                                      base::Unretained(this)));

  const GURL load_url = GetLoadURL(file_path);
  web_contents_->GetController().LoadURL(load_url, content::Referrer(),
                                         ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                         std::string());
}

void FileTextExtractorBase::WriteTempFileAndLoad(
    content::BrowserContext* browser_context,
    std::vector<uint8_t> file_bytes,
    const base::FilePath::StringType& extension) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&WriteBytesToTempFile, std::move(file_bytes), extension),
      base::BindOnce(&FileTextExtractorBase::OnTempFileWritten,
                     weak_ptr_factory_.GetWeakPtr(), browser_context));
}

void FileTextExtractorBase::Finish(std::optional<std::string> result) {
  DVLOG(1) << "FileTextExtractorBase: Finish has_result=" << result.has_value();
  timeout_timer_.Stop();
  Cleanup();
  if (callback_) {
    std::move(callback_).Run(std::move(result));
  }
}

// content::WebContentsObserver:

void FileTextExtractorBase::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }
  // A navigation that doesn't commit means the content triggered a download
  // (e.g. unrenderable MIME type). Fail fast instead of waiting for timeout.
  // Post async because Finish() destroys the WebContents, which cannot happen
  // during observer notification.
  if (!navigation_handle->HasCommitted()) {
    DVLOG(1) << "FileTextExtractorBase: Navigation did not commit (download)";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileTextExtractorBase::Finish,
                       weak_ptr_factory_.GetWeakPtr(), std::nullopt));
  }
}

void FileTextExtractorBase::DocumentOnLoadCompletedInPrimaryMainFrame() {
  OnDocumentReady();
}

void FileTextExtractorBase::PrimaryMainFrameRenderProcessGone(
    base::TerminationStatus status) {
  DVLOG(1) << "FileTextExtractorBase: Renderer process gone";
  Finish(std::nullopt);
}

void FileTextExtractorBase::OnTempFileWritten(
    content::BrowserContext* browser_context,
    std::optional<base::FilePath> temp_path) {
  if (!temp_path || !callback_) {
    Finish(std::nullopt);
    return;
  }
  temp_file_path_ = *temp_path;
  LoadInWebContents(browser_context, temp_file_path_);
}

void FileTextExtractorBase::OnTimeout() {
  DVLOG(1) << "FileTextExtractorBase: Extraction timed out";
  Finish(std::nullopt);
}

void FileTextExtractorBase::Cleanup() {
  // Invalidate weak ptrs first to prevent OnTempFileWritten from firing
  // after cleanup (e.g. if timeout triggers while temp file write is pending).
  weak_ptr_factory_.InvalidateWeakPtrs();
  if (web_contents_) {
    Observe(nullptr);
    web_contents_->SetDelegate(nullptr);
    web_contents_.reset();
  }
  if (!temp_file_path_.empty()) {
    base::ThreadPool::PostTask(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&DeleteTempFile, std::move(temp_file_path_)));
    temp_file_path_.clear();
  }
}

}  // namespace ai_chat
