// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_ICON_VIEW_H_

#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class PrefService;

class SpeedreaderIconView : public PageActionIconView {
 public:
  METADATA_HEADER(SpeedreaderIconView);
  SpeedreaderIconView(CommandUpdater* command_updater,
                      IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
                      PageActionIconView::Delegate* page_action_icon_delegate,
                      PrefService* pref_service);
  SpeedreaderIconView(const SpeedreaderIconView&) = delete;
  SpeedreaderIconView& operator=(const SpeedreaderIconView&) = delete;
  ~SpeedreaderIconView() override;

 protected:
  // PageActionIconView:
  const gfx::VectorIcon& GetVectorIcon() const override;
  void OnExecuting(PageActionIconView::ExecuteSource execute_source) override;
  views::BubbleDialogDelegate* GetBubble() const override;
  std::u16string GetTextForTooltipAndAccessibleName() const override;
  void UpdateImpl() override;

 private:
  speedreader::DistillState GetDistillState() const;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_ICON_VIEW_H_
