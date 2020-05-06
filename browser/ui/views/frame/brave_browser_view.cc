/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "extensions/buildflags/buildflags.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

#if BUILDFLAG(ENABLE_SPARKLE)
#include "brave/browser/ui/views/update_recommended_message_box_mac.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#endif

BraveBrowserView::BraveBrowserView(std::unique_ptr<Browser> browser)
    : BrowserView(std::move(browser)) {}

BraveBrowserView::~BraveBrowserView() {
  if (ctrl_released_event_handler_.get()) {
    // We are still MRU cycling (probably closed the window while cycling)
    // Disable checks that ensure that the event handler is not registered
    ui::EventHandler::DisableCheckTargets();

    static_cast<BraveTabStripModel*>(browser()->tab_strip_model())
        ->StopMRUCycling();
  }
}

void BraveBrowserView::SetStarredState(bool is_starred) {
  BookmarkButton* button =
      static_cast<BraveToolbarView*>(toolbar())->bookmark_button();
  if (button)
    button->SetToggled(is_starred);
}

void BraveBrowserView::ShowUpdateChromeDialog() {
#if BUILDFLAG(ENABLE_SPARKLE)
  // On mac, sparkle frameworks's relaunch api is used.
  UpdateRecommendedMessageBoxMac::Show(GetNativeWindow());
#else
  BrowserView::ShowUpdateChromeDialog();
#endif
}

// The translate bubble will be shown if ENABLE_BRAVE_TRANSLATE_GO or
// ENABLE_BRAVE_TRANSLATE_EXTENSIONS build flag is enabled and Google Translate
// is not installed. In ENABLE_BRAVE_TRANSLATE case, we utilize chromium's
// translate UI directly along with go-translate. In
// ENABLE_BRAVE_TRANSLATE_EXTENSION case, we repurpose the translate bubble to
// offer Google Translate extension installation, and the bubble will only be
// shown when Google Translate is not installed.
ShowTranslateBubbleResult BraveBrowserView::ShowTranslateBubble(
    content::WebContents* web_contents,
    translate::TranslateStep step,
    const std::string& source_language,
    const std::string& target_language,
    translate::TranslateErrors::Type error_type,
    bool is_user_gesture) {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  return BrowserView::ShowTranslateBubble(web_contents,
                                          step,
                                          source_language,
                                          target_language,
                                          error_type,
                                          is_user_gesture);
#elif BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  if (!extensions::ExtensionRegistry::Get(GetProfile())
      ->GetInstalledExtension(google_translate_extension_id)) {
    return BrowserView::ShowTranslateBubble(web_contents,
                                            step,
                                            source_language,
                                            target_language,
                                            error_type,
                                            is_user_gesture);
  }
#endif
  return ShowTranslateBubbleResult::BROWSER_WINDOW_NOT_VALID;
}

void BraveBrowserView::StartMRUCycling() {
  ctrl_released_event_handler_ = std::make_unique<CtrlReleaseHandler>(
      static_cast<BraveTabStripModel*>(browser()->tab_strip_model()), this);

  // Add the event handler
#if defined(OS_MACOSX)
  if (GetWidget()->GetRootView())
    GetWidget()->GetRootView()->AddPreTargetHandler(
        ctrl_released_event_handler_.get());
#else
  if (GetWidget()->GetNativeWindow())
    GetWidget()->GetNativeWindow()->AddPreTargetHandler(
        ctrl_released_event_handler_.get());
#endif
}

BraveBrowserView::CtrlReleaseHandler::CtrlReleaseHandler(
    BraveTabStripModel* model,
    BraveBrowserView* browser_view)
    : model_(model), browser_view_(browser_view) {}

BraveBrowserView::CtrlReleaseHandler::~CtrlReleaseHandler() = default;

void BraveBrowserView::CtrlReleaseHandler::OnKeyEvent(ui::KeyEvent* event) {
  if (event->key_code() == ui::VKEY_CONTROL &&
      event->type() == ui::ET_KEY_RELEASED) {
    // Ctrl key was released, stop the MRU cycling

    // Remove event handler
#if defined(OS_MACOSX)
    if (browser_view_->GetWidget()->GetRootView())
      browser_view_->GetWidget()->GetRootView()->RemovePreTargetHandler(this);
#else
    if (browser_view_->GetWidget()->GetNativeWindow())
      browser_view_->GetWidget()->GetNativeWindow()->RemovePreTargetHandler(
          this);
#endif

    model_->StopMRUCycling();

  } else if (!((event->key_code() == ui::VKEY_TAB &&
                event->type() == ui::ET_KEY_PRESSED) ||
               (event->key_code() == ui::VKEY_PRIOR &&
                event->type() == ui::ET_KEY_PRESSED) ||
               (event->key_code() == ui::VKEY_NEXT &&
                event->type() == ui::ET_KEY_PRESSED))) {
    // Block all keys while cycling except tab,pg previous, pg next keys
    event->StopPropagation();
  }
}
