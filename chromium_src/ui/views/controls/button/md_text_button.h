// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_

// Rename MdTextButton to MdTextButtonBase
#include "absl/types/optional.h"
#include "ui/gfx/vector_icon_types.h"
#define MdTextButton MdTextButtonBase

// Define a Brave-specific method we can get called from UpdateColors() to
// extend its functionality instead of defining UpdateColors() "virtual" and
// overriding it in our version of the MdTextButton class because there are some
// subclasses that define their own UpdateColors() method (OmniboxChipButton)
// now, which would not work with the virtual + override approach.
#define UpdateColors                       \
  UpdateColors_Unused();                   \
                                           \
 protected:                                \
  virtual void UpdateColorsForBrave() = 0; \
  virtual void UpdateIconForBrave() = 0;   \
  void UpdateColors

#include "src/ui/views/controls/button/md_text_button.h"

#undef UpdateColors
#undef MdTextButton

namespace views {

// Make visual changes to MdTextButton in line with Brave visual style:
//  - More rounded rectangle (for regular border, focus ring and ink drop)
//  - Different hover text and boder color for non-prominent button
//  - Differenet hover bg color for prominent background
//  - No shadow for prominent background
class VIEWS_EXPORT MdTextButton : public MdTextButtonBase {
 public:
  struct ButtonStyle {
    absl::optional<SkColor> background_color;
    absl::optional<SkColor> border_color;
    SkColor text_color;
  };

  struct ButtonTheme {
    ButtonStyle normal_light;
    ButtonStyle normal_dark;

    ButtonStyle hover_light;
    ButtonStyle hover_dark;

    ButtonStyle disabled_light;
    ButtonStyle disabled_dark;

    ButtonStyle loading_light;
    ButtonStyle loading_dark;
  };

  enum Kind { OLD, PRIMARY, SECONDARY, TERTIARY };

  explicit MdTextButton(PressedCallback callback = PressedCallback(),
                        const std::u16string& text = std::u16string(),
                        int button_context = style::CONTEXT_BUTTON_MD);
  MdTextButton(const MdTextButton&) = delete;
  MdTextButton& operator=(const MdTextButton&) = delete;
  ~MdTextButton() override;

  SkPath GetHighlightPath() const;

  Kind GetKind() const;
  void SetKind(Kind kind);

  void SetIcon(const gfx::VectorIcon* icon);

  // Until we decide to update the whole UI to use the new Leo colors, we
  // need to keep this logic around. Currently the new colors are opt-in only.
  void UpdateOldColorsForBrave();

  // MdTextButtonBase:
  void UpdateColorsForBrave() override;
  void UpdateIconForBrave() override;

 protected:
  // views::Views
  void OnPaintBackground(gfx::Canvas* canvas) override;

 private:
  Kind kind_ = OLD;
  absl::optional<ButtonTheme> theme_;
  raw_ptr<const gfx::VectorIcon> icon_ = nullptr;
};

}  // namespace views

DEFINE_VIEW_BUILDER(VIEWS_EXPORT, MdTextButton)

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
