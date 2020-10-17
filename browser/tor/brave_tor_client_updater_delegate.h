/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_BRAVE_TOR_CLIENT_UPDATER_DELEGATE_H_
#define BRAVE_BROWSER_TOR_BRAVE_TOR_CLIENT_UPDATER_DELEGATE_H_

#include "base/files/file_path.h"
#include "brave/components/tor/brave_tor_client_updater.h"

namespace tor {

class BraveTorClientUpdaterDelegate : public BraveTorClientUpdater::Delegate {
 public:
  explicit BraveTorClientUpdaterDelegate(const base::FilePath& user_data_dir);
  ~BraveTorClientUpdaterDelegate() override;

  // BraveTorClientUpdater::Delegate
  void Cleanup(const char* component_id) override;
  bool IsTorDisabled() override;

 private:
  base::FilePath user_data_dir_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  BraveTorClientUpdaterDelegate(const BraveTorClientUpdaterDelegate&) = delete;
  BraveTorClientUpdaterDelegate& operator=(
      const BraveTorClientUpdaterDelegate&) = delete;
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_BRAVE_TOR_CLIENT_UPDATER_DELEGATE_H_
