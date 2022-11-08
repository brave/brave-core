// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/vector_icon_types.h"

// Rename MdTextButton to MdTextButtonBase
#define MdTextButton MdTextButtonBase

// Define a Brave-specific method we can get called from UpdateColors() to
// extend its functionality instead of defining UpdateColors() "virtual" and
// overriding it in our version of the MdTextButton class because there are some
// subclasses that define their own UpdateColors() method (OmniboxChipButton)
// now, which would not work with the virtual + override approach.
#define UpdateTextColor                    \
  UpdateTextColor_Unused();                \
                                           \
 protected:                                \
  virtual void UpdateColorsForBrave() = 0; \
  virtual void UpdateIconForBrave() = 0;   \
  void UpdateTextColor

#include "src/ui/views/controls/button/md_text_button.h"

#undef UpdateTextColor
#undef MdTextButton

namespace views {

// Make visual changes to MdTextButton in line with Brave visual style:
//  - More rounded rectangle (for regular border, focus ring and ink drop)
//  - Different hover text and boder color for non-prominent button
//  - Differenet hover bg color for prominent background
//  - No shadow for prominent background
class VIEWS_EXPORT MdTextButton : public MdTextButtonBase {
 public:
  enum Kind { kOld, kPrimary, kSecondary, kTertiary };

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

  bool GetLoading() const;
  void SetLoading(bool loading);

  // Until we decide to update the whole UI to use the new Leo colors, we
  // need to keep this logic around. Currently the new colors are opt-in only.
  void UpdateOldColorsForBrave();

  // MdTextButtonBase:
  void UpdateBackgroundColor() override;
  void UpdateColorsForBrave() override;
  void UpdateIconForBrave() override;

 protected:
  // views::Views
  void OnPaintBackground(gfx::Canvas* canvas) override;

 private:
  Kind kind_ = kOld;
  bool loading_ = false;
  raw_ptr<const gfx::VectorIcon> icon_ = nullptr;
};

}  // namespace views

DEFINE_VIEW_BUILDER(VIEWS_EXPORT, MdTextButton)

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
