// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/label_button.h"

// Rename MdTextButton to MdTextButtonBase
#define MdTextButton MdTextButtonBase

// Redefine UpdateTextColor as protected virtual so we can override it.
#define UpdateTextColor     \
  UpdateTextColor_Unused(); \
                            \
 protected:                 \
  virtual void UpdateTextColor

#define UpdateColors virtual UpdateColors

#include "src/ui/views/controls/button/md_text_button.h"  // IWYU pragma: export

#undef UpdateColors
#undef UpdateTextColor
#undef MdTextButton

namespace views {
// Make visual changes to MdTextButton in line with Brave visual style:
//  - More rounded rectangle (for regular border, focus ring and ink drop)
//  - Different hover text and boder color for non-prominent button
//  - Differenet hover bg color for prominent background
//  - No shadow for prominent background
class VIEWS_EXPORT MdTextButton : public MdTextButtonBase {
  METADATA_HEADER(MdTextButton, views::MdTextButtonBase)

 public:
  struct ButtonColors {
    SkColor background_color;
    SkColor stroke_color;
    SkColor text_color;
  };

  explicit MdTextButton(
      PressedCallback callback = PressedCallback(),
      const std::u16string& text = std::u16string(),
      int button_context = style::CONTEXT_BUTTON_MD,
      bool use_text_color_for_icon = true,
      std::unique_ptr<LabelButtonImageContainer> image_container =
          std::make_unique<SingleImageContainer>());
  MdTextButton(const MdTextButton&) = delete;
  MdTextButton& operator=(const MdTextButton&) = delete;
  ~MdTextButton() override;

  SkPath GetHighlightPath() const;

  void SetIcon(const gfx::VectorIcon* icon, int icon_size = 0);

  bool GetLoading() const;
  void SetLoading(bool loading);
  void set_use_default_for_tonal(bool use_default) {
    use_default_for_tonal_ = use_default;
  }

  // MdTextButtonBase:
  void UpdateTextColor() override;
  void UpdateBackgroundColor() override;
  void UpdateColors() override;

 private:
  ButtonColors GetButtonColors();
  ui::ButtonStyle GetBraveStyle() const;

  bool loading_ = false;

  // By default, use kDefault style for kTonal because
  // it's not suitable to our style. Use default style instead.
  bool use_default_for_tonal_ = true;

  int icon_size_ = 0;
  raw_ptr<const gfx::VectorIcon> icon_ = nullptr;
};

}  // namespace views

DEFINE_VIEW_BUILDER(VIEWS_EXPORT, MdTextButton)

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
