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

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace image_editor {
struct ScreenshotCaptureResult;
class ScreenshotFlow;
}  // namespace image_editor

namespace screenshot {
class PrintPreviewExtractor;

// Owned by BrowserWindowFeatures (one per Browser). Orchestrates capture of a
// screenshot (visible area or full page), encodes the result as PNG, and
// drives a Save As dialog.
class ScreenshotController : public ui::SelectFileDialog::Listener {
 public:
  enum class Error {
    kNoTab,
    kBusy,
    kCaptureFailed,
    kEncodeFailed,
    kUserCancelled,
    kWriteFailed,
  };
  using ResultCallback =
      base::OnceCallback<void(base::expected<base::FilePath, Error>)>;

  // `parent_window_getter` is invoked just-in-time at Save-As dialog
  // creation, after the browser window is fully initialized.
  using NativeWindowGetter = base::RepeatingCallback<gfx::NativeWindow()>;

  ScreenshotController(content::BrowserContext* profile,
                       NativeWindowGetter parent_window_getter);
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  ScreenshotController(content::BrowserContext* profile,
                       NativeWindowGetter parent_window_getter,
                       std::unique_ptr<screenshot::PrintPreviewExtractor>
                           print_preview_extractor);
#endif
  ScreenshotController(const ScreenshotController&) = delete;
  ScreenshotController& operator=(const ScreenshotController&) = delete;
  ~ScreenshotController() override;

  // capture is already in flight.
  base::expected<void, Error> CanCapture(
      content::WebContents* web_contents) const;

  // Captures the visible viewport area or the full page of `web_contents`,
  // encodes the result as PNG, and drives a Save As dialog.
  void CaptureVisibleArea(content::WebContents* web_contents,
                          ResultCallback done);

  // Starts an interactive region-selection UI over `web_contents`. Once the
  // user drags a rectangle, the selected region is encoded as PNG and a Save
  // As dialog is shown.
  void CaptureSelectedArea(content::WebContents* web_contents,
                           ResultCallback done);

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  void CaptureFullPage(content::WebContents* web_contents, ResultCallback done);
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

  void set_download_dir_for_testing(const base::FilePath& path) {
    download_dir_for_testing_ = path;
  }

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
  void OnRegionCaptured(const image_editor::ScreenshotCaptureResult& result);

  void OnEncoded(std::optional<std::vector<uint8_t>> png);
  void ShowSaveDialog(std::vector<uint8_t> png);
  void ShowSaveDialogWithPath(const base::FilePath& default_path);
  // Reply callback for WritePngFile posted from FileSelected().
  void OnFileWritten(const base::FilePath& path, bool ok);
  void FinishWithError(Error error);
  void Reset();

  NativeWindowGetter parent_window_getter_;

  // Pending operation state. Set on entry, cleared on Reset().
  ResultCallback pending_callback_;
  std::vector<uint8_t> pending_png_;

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  std::unique_ptr<screenshot::PrintPreviewExtractor> print_preview_extractor_;
#endif

  std::optional<base::FilePath> download_dir_for_testing_;

  // Owns the interactive region-selection overlay during CaptureSelectedArea().
  // Reset as soon as the selection callback fires.
  std::unique_ptr<image_editor::ScreenshotFlow> screenshot_flow_;

  raw_ptr<content::BrowserContext> profile_;

  scoped_refptr<ui::SelectFileDialog> select_dialog_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ScreenshotController> weak_factory_{this};
};

}  // namespace screenshot

#endif  // BRAVE_BROWSER_UI_SCREENSHOT_SCREENSHOT_CONTROLLER_H_
