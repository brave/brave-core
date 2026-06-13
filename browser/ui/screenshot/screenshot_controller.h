// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SCREENSHOT_SCREENSHOT_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SCREENSHOT_SCREENSHOT_CONTROLLER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/types/expected.h"
#include "printing/buildflags/buildflags.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/native_ui_types.h"
#include "ui/shell_dialogs/select_file_dialog.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace screenshot {
class PrintPreviewExtractor;

// Owned by BrowserWindowFeatures (one per Browser). Orchestrates capture of a
// screenshot (visible area or full page), encodes the result as PNG, and
// drives a Save As dialog. Does not expose ai_chat:: types to callers.
class ScreenshotController : public ui::SelectFileDialog::Listener {
 public:
  enum class Error {
    kNoTab,
    kCaptureFailed,
    kEncodeFailed,
    kUserCancelled,
    kWriteFailed,
  };
  using ResultCallback =
      base::OnceCallback<void(base::expected<base::FilePath, Error>)>;

  // `parent_window_getter` is invoked just-in-time at Save-As dialog
  // creation, after the browser window is fully initialized. Constructing the
  // controller during BraveToolbarView::Init() (when browser->window() is
  // still null) is intentional.
  using NativeWindowGetter = base::RepeatingCallback<gfx::NativeWindow()>;

  // Returns the directory used as the default Save As path. If empty, falls
  // back to DownloadPrefs::FromBrowserContext(profile_)->DownloadPath().
  // Tests can inject a simpler implementation to avoid the full download
  // service stack.
  using DownloadDirGetter = base::RepeatingCallback<base::FilePath()>;

  ScreenshotController(Profile* profile,
                       NativeWindowGetter parent_window_getter,
                       DownloadDirGetter download_dir_getter = {});
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  ScreenshotController(Profile* profile,
                       NativeWindowGetter parent_window_getter,
                       DownloadDirGetter download_dir_getter,
                       std::unique_ptr<screenshot::PrintPreviewExtractor>
                           print_preview_extractor);
#endif
  ScreenshotController(const ScreenshotController&) = delete;
  ScreenshotController& operator=(const ScreenshotController&) = delete;
  ~ScreenshotController() override;

  bool busy() const { return busy_; }

  // Returns false if `web_contents` has no live RenderWidgetHostView or a
  // capture is already in flight.
  bool CanCapture(content::WebContents* web_contents) const;

  // Captures the visible viewport area or the full page of `web_contents`,
  // encodes the result as PNG, and drives a Save As dialog.
  void CaptureVisibleArea(content::WebContents* web_contents,
                          ResultCallback done);
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  void CaptureFullPage(content::WebContents* web_contents, ResultCallback done);
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

  // ui::SelectFileDialog::Listener:
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void FileSelectionCanceled() override;

 private:
  friend class ScreenshotControllerTest;

  using ChunkResult =
      base::expected<std::vector<std::vector<uint8_t>>, std::string>;

  void OnVisibleAreaCopied(SkBitmap bitmap);
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  void OnFullPageChunks(ChunkResult result);
#endif

  void OnEncoded(std::optional<std::vector<uint8_t>> png);
  void ShowSaveDialog(std::vector<uint8_t> png);
  void ShowSaveDialogWithPath(base::FilePath default_path);
  // Reply callback for WritePngFile posted from FileSelected().
  void OnFileWritten(base::FilePath path, bool ok);
  void FinishWithError(Error error);
  void Reset();

  raw_ptr<Profile> profile_;
  NativeWindowGetter parent_window_getter_;
  DownloadDirGetter download_dir_getter_;
  bool busy_ = false;

  // Pending operation state. Set on entry, cleared on Reset().
  ResultCallback pending_callback_;
  std::vector<uint8_t> pending_png_;

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  std::unique_ptr<screenshot::PrintPreviewExtractor> print_preview_extractor_;
#endif

  scoped_refptr<ui::SelectFileDialog> select_dialog_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ScreenshotController> weak_factory_{this};
};

}  // namespace screenshot

#endif  // BRAVE_BROWSER_UI_SCREENSHOT_SCREENSHOT_CONTROLLER_H_
