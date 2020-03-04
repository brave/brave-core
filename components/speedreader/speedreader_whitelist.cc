/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_whitelist.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/task/post_task.h"
#include "brave/components/speedreader/rust/ffi/include/speedreader.hpp"
#include "url/gurl.h"

namespace speedreader {

namespace {

constexpr base::FilePath::CharType kDatFileName[] =
    FILE_PATH_LITERAL("\rs-SpeedreaderWhitelist.dat");

constexpr char kComponentName[] = "Speedreader";
// TODO(iefremov):
constexpr char kComponentId[] = "";
constexpr char kComponentPublicKey[] = "";

}  // namespace

SpeedreaderWhitelist::SpeedreaderWhitelist(Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate),
      speedreader_(new speedreader::SpeedReader) {
  Register(kComponentName, kComponentId, kComponentPublicKey);
}

SpeedreaderWhitelist::~SpeedreaderWhitelist() = default;

void SpeedreaderWhitelist::OnComponentReady(const std::string& component_id,
                                            const base::FilePath& install_dir,
                                            const std::string& manifest) {
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(
          &brave_component_updater::LoadDATFileData<speedreader::SpeedReader>,
          install_dir.Append(kDatFileName)),
      base::BindOnce(&SpeedreaderWhitelist::OnGetDATFileData,
                     weak_factory_.GetWeakPtr()));
}

bool SpeedreaderWhitelist::IsWhitelisted(const GURL& url) {
  return speedreader_->ReadableURL(url.spec());
}

void SpeedreaderWhitelist::OnGetDATFileData(GetDATFileDataResult result) {
  speedreader_ = std::move(result.first);
}

}  // namespace speedreader
