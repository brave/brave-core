/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_JAVASCRIPT_TAB_MODAL_DIALOG_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_JAVASCRIPT_TAB_MODAL_DIALOG_VIEW_VIEWS_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/views/javascript_tab_modal_dialog_view_views.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/widget/widget_observer.h"

namespace web_modal {
class WebContentsModalDialogHost;
}  // namespace web_modal

// This class overrides JavaScriptTabModalDialogViewViews to customize the
// position of the dialog. In split view mode, we want dialogs to be centered to
// its relevant web view.
class BraveJavaScriptTabModalDialogViewViews
    : public JavaScriptTabModalDialogViewViews,
      public views::WidgetObserver {
  METADATA_HEADER(BraveJavaScriptTabModalDialogViewViews,
                  JavaScriptTabModalDialogViewViews)
 public:
  BraveJavaScriptTabModalDialogViewViews(
      content::WebContents* parent_web_contents,
      content::WebContents* alerting_web_contents,
      const std::u16string& title,
      content::JavaScriptDialogType dialog_type,
      const std::u16string& message_text,
      const std::u16string& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback dialog_callback,
      base::OnceClosure dialog_force_closed_callback);
  ~BraveJavaScriptTabModalDialogViewViews() override;

 private:
  friend class JavaScriptTabModalDialogManagerDelegateDesktop;

  web_modal::WebContentsModalDialogHost& GetModalDialogHost();

  void UpdateWidgetBounds();

  // This returns point in dialog host's widget coordinate.
  gfx::Point GetDesiredPositionConsideringSplitView();

  raw_ref<content::WebContents, DanglingUntriaged> web_contents_;

  base::WeakPtrFactory<BraveJavaScriptTabModalDialogViewViews>
      weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_JAVASCRIPT_TAB_MODAL_DIALOG_VIEW_VIEWS_H_
