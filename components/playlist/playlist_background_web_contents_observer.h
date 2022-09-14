/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_BACKGROUND_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_BACKGROUND_WEB_CONTENTS_OBSERVER_H_

#include "base/containers/flat_map.h"
#include "brave/components/brave_shields/common/brave_shields.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace playlist {

class PlaylistBackgroundWebContentsObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<
          PlaylistBackgroundWebContentsObserver> {
 public:
  explicit PlaylistBackgroundWebContentsObserver(
      content::WebContents* web_contents);
  PlaylistBackgroundWebContentsObserver(
      const PlaylistBackgroundWebContentsObserver&) = delete;
  PlaylistBackgroundWebContentsObserver& operator=(
      const PlaylistBackgroundWebContentsObserver&) = delete;
  ~PlaylistBackgroundWebContentsObserver() override;

 private:
  friend class content::WebContentsUserData<
      PlaylistBackgroundWebContentsObserver>;

  using BraveShieldsRemotesMap = base::flat_map<
      content::RenderFrameHost*,
      mojo::AssociatedRemote<brave_shields::mojom::BraveShields>>;

  // content::WebContentsObserver overrides.
  void RenderFrameCreated(content::RenderFrameHost* rfh) override;
  void RenderFrameDeleted(content::RenderFrameHost* rfh) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_rfh,
                              content::RenderFrameHost* new_rfh) override;

  mojo::AssociatedRemote<brave_shields::mojom::BraveShields>&
  GetBraveShieldsRemote(content::RenderFrameHost* rfh);

  BraveShieldsRemotesMap brave_shields_remotes_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_BACKGROUND_WEB_CONTENTS_OBSERVER_H_
