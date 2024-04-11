/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_TEST_MOCK_PLAYLIST_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_PLAYLIST_TEST_MOCK_PLAYLIST_SERVICE_OBSERVER_H_

#include <string>
#include <vector>

#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

class MockPlaylistServiceObserver
    : public playlist::mojom::PlaylistServiceObserver {
 public:
  MockPlaylistServiceObserver();
  ~MockPlaylistServiceObserver() override;

  mojo::PendingRemote<playlist::mojom::PlaylistServiceObserver> GetRemote() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  // playlist::mojom::PlaylistServiceObserver:
  MOCK_METHOD(void,
              OnEvent,
              (playlist::mojom::PlaylistEvent event,
               const std::string& playlist_id),
              (override));

  MOCK_METHOD(void,
              OnItemCreated,
              (playlist::mojom::PlaylistItemPtr item),
              (override));
  MOCK_METHOD(void,
              OnItemLocalDataDeleted,
              (const std::string& id),
              (override));
  MOCK_METHOD(void,
              OnItemAddedToList,
              (const std::string& playlist_id, const std::string& item_id),
              (override));
  MOCK_METHOD(void,
              OnItemRemovedFromList,
              (const std::string& playlist_id, const std::string& item_id),
              (override));
  MOCK_METHOD(void,
              OnItemCached,
              (playlist::mojom::PlaylistItemPtr item),
              (override));
  MOCK_METHOD(void,
              OnItemUpdated,
              (playlist::mojom::PlaylistItemPtr item),
              (override));

  MOCK_METHOD(void,
              OnPlaylistUpdated,
              (playlist::mojom::PlaylistPtr),
              (override));

  MOCK_METHOD(void,
              OnMediaFileDownloadScheduled,
              (const std::string& id),
              (override));

  MOCK_METHOD(void,
              OnMediaFileDownloadProgressed,
              (const std::string& id,
               int64_t total_bytes,
               int64_t received_bytes,
               int8_t percent_complete,
               const std::string& time_remaining),
              (override));
  MOCK_METHOD(void,
              OnMediaFilesUpdated,
              (const GURL& page_url,
               std::vector<playlist::mojom::PlaylistItemPtr> items),
              (override));

 private:
  mojo::Receiver<playlist::mojom::PlaylistServiceObserver> observer_receiver_{
      this};
};

#endif  // BRAVE_BROWSER_PLAYLIST_TEST_MOCK_PLAYLIST_SERVICE_OBSERVER_H_
