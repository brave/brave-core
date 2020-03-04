/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_WHITELIST_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_WHITELIST_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"

namespace speedreader {
class SpeedReader;
}

class GURL;

namespace speedreader {

class SpeedreaderWhitelist : public brave_component_updater::BraveComponent {
 public:
  explicit SpeedreaderWhitelist(Delegate* delegate);
  ~SpeedreaderWhitelist() override;

  SpeedreaderWhitelist(const SpeedreaderWhitelist&) = delete;
  SpeedreaderWhitelist& operator=(const SpeedreaderWhitelist&) = delete;

  bool IsWhitelisted(const GURL& url);

 private:
  // brave_component_updater::BraveComponent:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  using GetDATFileDataResult =
      brave_component_updater::LoadDATFileDataResult<speedreader::SpeedReader>;

  void OnGetDATFileData(GetDATFileDataResult result);

  std::unique_ptr<speedreader::SpeedReader> speedreader_;
  base::WeakPtrFactory<SpeedreaderWhitelist> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_WHITELIST_H_
