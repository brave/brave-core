// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/screenshot_preview_dialog.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/platform_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/ui_base_types.mojom-shared.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace screenshot {

namespace {

// The preview always renders at this width, scaled to fit; only the height
// varies with the capture's aspect ratio, capped below and scrolling past
// that cap. Sized to fit comfortably on a small laptop display (e.g.
// 1280x800/1366x768) alongside the OS chrome and the dialog's title/button
// rows.
constexpr int kPreviewWidth = 800;
constexpr int kPreviewMaxHeight = 500;

std::unique_ptr<views::View> CreatePreviewScrollView(
    base::span<const uint8_t> png) {
  auto scroll_view = std::make_unique<views::ScrollView>();
  // Bounding the height. The image will be scrollable if it overflows
  scroll_view->ClipHeightTo(0, kPreviewMaxHeight);

  // The image is always scaled to the viewport's width, so there's never
  // horizontal overflow to scroll.
  scroll_view->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);

  auto* image_view =
      scroll_view->SetContents(std::make_unique<views::ImageView>());
  SkBitmap bitmap = gfx::PNGCodec::Decode(png);
  gfx::ImageSkia image = gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
  image_view->SetImage(ui::ImageModel::FromImageSkia(image));

  // Scale to exactly fill kPreviewWidth, preserving aspect ratio; height
  // follows and may exceed kPreviewMaxHeight, in which case it scrolls.
  int scaled_height = image.height() * kPreviewWidth / image.width();
  image_view->SetImageSize(gfx::Size(kPreviewWidth, scaled_height));

  return scroll_view;
}

class ScreenshotPreviewDialogDelegate : public views::DialogDelegate {
 public:
  explicit ScreenshotPreviewDialogDelegate(std::vector<uint8_t> png)
      : png_(std::move(png)) {
    SetOwnershipOfNewWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
    SetModalType(ui::mojom::ModalType::kWindow);
    SetTitle(
        l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_PREVIEW_DIALOG_TITLE));
    // Only the Download action is a button; dismissal happens via the frame's
    // close (X) control or Esc, both surfaced as a non-accept ClosedReason to
    // ScreenshotPreviewDialogHolder::OnClosed() below.
    SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk));
    SetButtonLabel(ui::mojom::DialogButton::kOk,
                   l10n_util::GetStringUTF16(
                       IDS_BRAVE_SCREENSHOT_PREVIEW_DIALOG_DOWNLOAD_BUTTON));
    SetShowCloseButton(true);

    // Same content margins Chrome's own screenshot-captured bubble uses
    // (chrome/browser/ui/views/sharing_hub/screenshot_captured_bubble.cc).
    set_margins(views::LayoutProvider::Get()->GetDialogInsetsForContentType(
        views::DialogContentType::kControl, views::DialogContentType::kText));

    SetContentsView(CreatePreviewScrollView(png_));
  }

  // Only meaningful to call once, when the user accepts the dialog.
  std::vector<uint8_t> TakePng() { return std::move(png_); }

 private:
  std::vector<uint8_t> png_;
};

// Owns ScreenshotPreviewDialogDelegate and views::Widget of the dialog.
// cleans them up when the dialog closes.
class ScreenshotPreviewDialogHolder {
 public:
  ScreenshotPreviewDialogHolder(
      gfx::NativeWindow parent,
      std::vector<uint8_t> png,
      base::OnceCallback<void(std::vector<uint8_t>)> on_download,
      base::OnceClosure on_cancel)
      : delegate_(
            std::make_unique<ScreenshotPreviewDialogDelegate>(std::move(png))),
        on_download_(std::move(on_download)),
        on_cancel_(std::move(on_cancel)) {
    widget_.reset(views::DialogDelegate::CreateDialogWidget(
        delegate_.get(), gfx::NativeWindow(),
        platform_util::GetViewForWindow(parent)));
    widget_->MakeCloseSynchronous(base::BindOnce(
        &ScreenshotPreviewDialogHolder::OnClosed, base::Unretained(this)));
    widget_->Show();
  }

 private:
  void OnClosed(views::Widget::ClosedReason reason) {
    // Per Widget::MakeCloseSynchronous()'s contract, this is the client's cue
    // to actually close the widget. `delegate_` (and its `png_`) outlives
    // this, so it's still safe to pull the bytes back out below.
    widget_.reset();
    if (reason == views::Widget::ClosedReason::kAcceptButtonClicked) {
      std::move(on_download_).Run(delegate_->TakePng());
    } else {
      std::move(on_cancel_).Run();
    }
    delete this;
  }

  std::unique_ptr<ScreenshotPreviewDialogDelegate> delegate_;
  std::unique_ptr<views::Widget> widget_;
  base::OnceCallback<void(std::vector<uint8_t>)> on_download_;
  base::OnceClosure on_cancel_;
};

}  // namespace

void ShowScreenshotPreviewDialog(
    gfx::NativeWindow parent,
    std::vector<uint8_t> png,
    base::OnceCallback<void(std::vector<uint8_t>)> on_download,
    base::OnceClosure on_cancel) {
  new ScreenshotPreviewDialogHolder(
      parent, std::move(png), std::move(on_download), std::move(on_cancel));
}

}  // namespace screenshot
