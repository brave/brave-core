/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/playlist/playlist_color_mixer.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/leo/colors.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"

namespace playlist {

void AddLightThemeColorMixer(ui::ColorProvider* provider,
                             const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorBravePlaylistAddedIcon] = {leo::GetColor(
      leo::Color::kColorSystemfeedbackSuccessIcon, leo::Theme::kLight)};
  mixer[kColorBravePlaylistCheckedIcon] = {
      leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kLight)};
  mixer[kColorBravePlaylistSelectedBackground] = {leo::GetColor(
      leo::Color::kColorContainerInteractiveBackground, leo::Theme::kLight)};
  mixer[kColorBravePlaylistListBorder] = {
      leo::GetColor(leo::Color::kColorDividerSubtle, leo::Theme::kLight)};
  mixer[kColorBravePlaylistMoveDialogDescription] = {
      leo::GetColor(leo::Color::kColorTextSecondary, leo::Theme::kLight)};
  mixer[kColorBravePlaylistMoveDialogCreatePlaylistAndMoveTitle] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kLight)};
}

void AddDarkThemeColorMixer(ui::ColorProvider* provider,
                            const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorBravePlaylistAddedIcon] = {leo::GetColor(
      leo::Color::kColorSystemfeedbackSuccessIcon, leo::Theme::kDark)};
  mixer[kColorBravePlaylistCheckedIcon] = {
      leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kDark)};
  mixer[kColorBravePlaylistSelectedBackground] = {leo::GetColor(
      leo::Color::kColorContainerInteractiveBackground, leo::Theme::kDark)};
  mixer[kColorBravePlaylistListBorder] = {
      leo::GetColor(leo::Color::kColorDividerSubtle, leo::Theme::kDark)};
  mixer[kColorBravePlaylistMoveDialogDescription] = {
      leo::GetColor(leo::Color::kColorTextSecondary, leo::Theme::kDark)};
  mixer[kColorBravePlaylistMoveDialogCreatePlaylistAndMoveTitle] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kDark)};
}

}  // namespace playlist
