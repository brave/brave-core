// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_WEBUI_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_HANDLER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/history_embeddings/history_embeddings.mojom.h"
#include "brave/components/history_embeddings/core/history_embeddings_search.h"
#include "content/public/browser/document_user_data.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

namespace brave_history_embeddings {

class HistoryEmbeddingsHandler
    : public brave_history_embeddings::mojom::PageHandler {
 public:
  HistoryEmbeddingsHandler(
      mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
          pending_page_handler,
      mojo::PendingRemote<brave_history_embeddings::mojom::Page> pending_page,
      base::WeakPtr<Profile> profile);
  HistoryEmbeddingsHandler(const HistoryEmbeddingsHandler&) = delete;
  HistoryEmbeddingsHandler& operator=(const HistoryEmbeddingsHandler&) = delete;
  ~HistoryEmbeddingsHandler() override;

  // brave_history_embeddings::mojom::PageHandler:
  void Search(brave_history_embeddings::mojom::SearchQueryPtr query) override;

 private:
  // Builds mojom result and publishes it to the browser page UI.
  void PublishResultToPage(
      const brave_history_embeddings::SearchResult& native_search_result);

  // Callback for querying `HistoryEmbeddingsService::Search()`.
  void OnReceivedSearchResult(brave_history_embeddings::SearchResult result);

  mojo::Receiver<brave_history_embeddings::mojom::PageHandler> page_handler_;
  mojo::Remote<brave_history_embeddings::mojom::Page> page_;

  // The profile is used to get the HistoryEmbeddingsService to fulfill
  // search requests.
  const base::WeakPtr<Profile> profile_;

  brave_history_embeddings::SearchResult last_result_;

  base::WeakPtrFactory<HistoryEmbeddingsHandler> weak_ptr_factory_{this};
};

// Creates the brave://history document's HistoryEmbeddingsHandler. Owned by
// that document, so the handler goes away with it.
class HistoryEmbeddingsPageHandlerFactory
    : public content::DocumentUserData<HistoryEmbeddingsPageHandlerFactory>,
      public mojom::PageHandlerFactory {
 public:
  HistoryEmbeddingsPageHandlerFactory(
      const HistoryEmbeddingsPageHandlerFactory&) = delete;
  HistoryEmbeddingsPageHandlerFactory& operator=(
      const HistoryEmbeddingsPageHandlerFactory&) = delete;
  ~HistoryEmbeddingsPageHandlerFactory() override;

  // Binds `receiver` for `render_frame_host` if it hosts brave://history and
  // the profile's HistoryEmbeddingsService was built.
  static void BindForFrame(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingReceiver<mojom::PageHandlerFactory> receiver);

  // mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<mojom::Page> page,
      mojo::PendingReceiver<mojom::PageHandler> handler) override;

 private:
  friend class content::DocumentUserData<HistoryEmbeddingsPageHandlerFactory>;
  DOCUMENT_USER_DATA_KEY_DECL();

  explicit HistoryEmbeddingsPageHandlerFactory(
      content::RenderFrameHost* render_frame_host);

  mojo::Receiver<mojom::PageHandlerFactory> receiver_{this};
  std::unique_ptr<HistoryEmbeddingsHandler> handler_;
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_BROWSER_UI_WEBUI_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_HANDLER_H_
