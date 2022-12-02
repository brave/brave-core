/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/types/strong_alias.h"

namespace playlist {

struct PlaylistChangeParams {
  enum class Type {
    kNone,
    kItemAdded,             // a new playlist item added but not ready state
    kItemThumbnailReady,    // Thumbnail ready to use for playlist
    kItemThumbnailFailed,   // Failed to fetch thumbnail
    kItemCached,            // The item is cached in local storage
    kItemDeleted,           // An item deleted
    kItemAborted,           // Aborted during the creation process
    kItemLocalDataRemoved,  // Local data removed

    kListCreated,  // A list is created
    kListRemoved,  // A list is removed
    kAllDeleted,   // All playlist are deleted
  };
  static std::string GetPlaylistChangeTypeAsString(Type type);

  PlaylistChangeParams();
  PlaylistChangeParams(Type type, const std::string& id);
  ~PlaylistChangeParams();

  Type change_type = Type::kNone;
  std::string playlist_id;

  bool operator==(const PlaylistChangeParams& rhs) const;

  friend std::ostream& operator<<(std::ostream& os,
                                  const PlaylistChangeParams& change_params) {
    return os << "{ "
              << GetPlaylistChangeTypeAsString(change_params.change_type)
              << " }";
  }
};

struct PlaylistItemInfo {
  using Title = base::StrongAlias<struct TitleTag, std::string>;
  using ThumbnailPath = base::StrongAlias<struct ThumbnailPathTag, std::string>;
  using MediaFilePath = base::StrongAlias<struct MediaFilePathTag, std::string>;

  PlaylistItemInfo();
  PlaylistItemInfo(const Title& title,
                   const ThumbnailPath& thumbnail_path,
                   const MediaFilePath& media_file_path);
  PlaylistItemInfo(const PlaylistItemInfo& rhs);
  PlaylistItemInfo& operator=(const PlaylistItemInfo& rhs);
  PlaylistItemInfo(PlaylistItemInfo&& rhs) noexcept;
  PlaylistItemInfo& operator=(PlaylistItemInfo&& rhs) noexcept;
  ~PlaylistItemInfo();

  std::string id;
  std::string title;

  // These are origin urls from web page.
  std::string page_src;
  std::string thumbnail_src;
  std::string media_src;

  // These are either local path or web url.
  std::string thumbnail_path;
  std::string media_file_path;
  bool media_file_cached{false};

  // Could be null
  base::TimeDelta duration;
  std::string author;

  friend std::ostream& operator<<(std::ostream& os,
                                  const PlaylistItemInfo& item) {
    return os << "{ media_file_path: " << item.media_file_path
              << ", title: " << item.title
              << ", thumbnail_path: " << item.thumbnail_path
              << ", duration: " << item.duration << ", author: " << item.author
              << "}";
  }
};

struct PlaylistInfo {
  PlaylistInfo();
  PlaylistInfo(const PlaylistInfo& rhs);
  PlaylistInfo& operator=(const PlaylistInfo& rhs);
  PlaylistInfo(PlaylistInfo&& rhs) noexcept;
  PlaylistInfo& operator=(PlaylistInfo&& rhs) noexcept;
  ~PlaylistInfo();

  std::string id;
  std::string name;
  std::vector<PlaylistItemInfo> items;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
