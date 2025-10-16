/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_WIDGET_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_WIDGET_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/frame/browser_widget.h"
#include "ui/color/color_provider_key.h"

class CustomThemeSupplier;

class BraveBrowserWidget : public BrowserWidget {
 public:
  explicit BraveBrowserWidget(BrowserView* browser_view);
  BraveBrowserWidget(const BraveBrowserWidget&) = delete;
  BraveBrowserWidget& operator=(const BraveBrowserWidget&) = delete;
  ~BraveBrowserWidget() override;

  const BrowserView* browser_view() const { return view_; }

  // BrowserFrame overrides:
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  const ui::NativeTheme* GetNativeTheme() const override;
#endif
  ui::ColorProviderKey::ThemeInitializerSupplier* GetCustomTheme()
      const override;
  views::internal::RootView* CreateRootView() override;
  void SetTabDragKind(TabDragKind kind) override;
  ui::ColorProviderKey GetColorProviderKey() const override;

 private:
  raw_ptr<BrowserView> view_ = nullptr;
  scoped_refptr<CustomThemeSupplier> theme_supplier_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_WIDGET_H_
