/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/playlist/playlist_color_mixer.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"

namespace playlist {

void AddThemeColorMixer(ui::ColorProvider* provider,
                        leo::Theme theme,
                        const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorBravePlaylistAddedIcon] = {
      leo::GetColor(leo::Color::kColorSystemfeedbackSuccessIcon, theme)};
  mixer[kColorBravePlaylistCheckedIcon] = {
      leo::GetColor(leo::Color::kColorIconInteractive, theme)};
  mixer[kColorBravePlaylistSelectedBackground] = {
      leo::GetColor(leo::Color::kColorContainerInteractive, theme)};
  mixer[kColorBravePlaylistListBorder] = {
      leo::GetColor(leo::Color::kColorDividerSubtle, theme)};
  mixer[kColorBravePlaylistMoveDialogDescription] = {
      leo::GetColor(leo::Color::kColorTextSecondary, theme)};
  mixer[kColorBravePlaylistMoveDialogCreatePlaylistAndMoveTitle] = {
      leo::GetColor(leo::Color::kColorTextPrimary, theme)};
  mixer[kColorBravePlaylistNewPlaylistDialogNameLabel] = {
      leo::GetColor(leo::Color::kColorTextPrimary, theme)};
  mixer[kColorBravePlaylistNewPlaylistDialogItemsLabel] = {
      leo::GetColor(leo::Color::kColorTextSecondary, theme)};
  mixer[kColorBravePlaylistTextInteractive] = {
      leo::GetColor(leo::Color::kColorTextInteractive, theme)};
}

}  // namespace playlist
