/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_HISTORY_BRAVE_HISTORY_EMBEDDINGS_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_HISTORY_BRAVE_HISTORY_EMBEDDINGS_PAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/webui/history/brave_history_embeddings.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;
class Profile;

class BraveHistoryEmbeddingsPageHandler
    : public brave_history_embeddings::mojom::PageHandler {
 public:
  // The two facts the chrome://history UI runs on.
  struct State {
    // The setting, which is what the toggle shows.
    bool enabled = false;
    // Whether the setting is waiting on a relaunch to take effect, which is
    // when the side bar offers its "Relaunch" button.
    bool needs_restart = false;

    // Whether a search can run this session: the setting is on and the
    // embeddings service it needs was built. Upstream gates every call into
    // that service on this, including the `Search()` that CHECKs it is
    // non-null.
    bool search_enabled() const { return enabled && !needs_restart; }
  };

  // The one place the two facts are read, so the pref observer and
  // BraveHistoryUI's loadTimeData snapshot report the same values.
  static State GetState(Profile* profile);

  BraveHistoryEmbeddingsPageHandler(
      mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
          receiver,
      mojo::PendingRemote<brave_history_embeddings::mojom::Page> page,
      Profile* profile,
      PrefService* local_state);

  BraveHistoryEmbeddingsPageHandler(const BraveHistoryEmbeddingsPageHandler&) =
      delete;
  BraveHistoryEmbeddingsPageHandler& operator=(
      const BraveHistoryEmbeddingsPageHandler&) = delete;

  ~BraveHistoryEmbeddingsPageHandler() override;

  // brave_history_embeddings::mojom::PageHandler:
  void SetEnabled(bool enabled) override;

 private:
  void OnPrefChanged();

  mojo::Receiver<brave_history_embeddings::mojom::PageHandler> receiver_;
  mojo::Remote<brave_history_embeddings::mojom::Page> page_;
  const raw_ptr<Profile> profile_;
  const raw_ptr<PrefService> local_state_;
  PrefChangeRegistrar pref_change_registrar_;
  PrefChangeRegistrar local_state_change_registrar_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_HISTORY_BRAVE_HISTORY_EMBEDDINGS_PAGE_HANDLER_H_
