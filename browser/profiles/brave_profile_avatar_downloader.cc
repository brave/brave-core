/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_avatar_downloader.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "ui/gfx/image/image.h"

namespace {
void DummyCallback(gfx::Image image,
                   const std::string& key,
                   const base::FilePath& image_path) {}
}  // namespace

BraveProfileAvatarDownloader::BraveProfileAvatarDownloader(
    size_t icon_index,
    FetchCompleteCallback callback)
    : ProfileAvatarDownloader(icon_index, base::BindOnce(DummyCallback)),
      callback_(std::move(callback)) {}

void BraveProfileAvatarDownloader::Start() {
  std::move(callback_).Run(gfx::Image(), "", base::FilePath());
}

BraveProfileAvatarDownloader::~BraveProfileAvatarDownloader() {}
