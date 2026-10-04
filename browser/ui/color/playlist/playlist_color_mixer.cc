/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/playlist/playlist_color_mixer.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"

namespace playlist {

void AddThemeColorMixer(ui::ColorProvider* provider,
                        const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorBravePlaylistAddedIcon] = {nala::kColorSystemfeedbackSuccessIcon};
  mixer[kColorBravePlaylistCheckedIcon] = {nala::kColorIconInteractive};
  mixer[kColorBravePlaylistSelectedBackground] = {
      nala::kColorContainerInteractive};
  mixer[kColorBravePlaylistListBorder] = {nala::kColorDividerSubtle};
  mixer[kColorBravePlaylistMoveDialogDescription] = {nala::kColorTextSecondary};
  mixer[kColorBravePlaylistMoveDialogCreatePlaylistAndMoveTitle] = {
      nala::kColorTextPrimary};
  mixer[kColorBravePlaylistNewPlaylistDialogNameLabel] = {
      nala::kColorTextPrimary};
  mixer[kColorBravePlaylistNewPlaylistDialogItemsLabel] = {
      nala::kColorTextSecondary};
  mixer[kColorBravePlaylistTextInteractive] = {nala::kColorTextInteractive};
}

}  // namespace playlist
