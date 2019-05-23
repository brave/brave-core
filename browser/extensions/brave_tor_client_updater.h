/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_TOR_CLIENT_UPDATER_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_TOR_CLIENT_UPDATER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

class BraveTorClientUpdaterTest;

using brave_component_updater::BraveComponent;

// TODO(bridiver) - this doesn't belong under extensions
namespace extensions {

#if defined(OS_WIN)
extern const char kTorClientComponentName[];
extern const char kTorClientComponentId[];
#elif defined(OS_MACOSX)
extern const char kTorClientComponentName[];
extern const char kTorClientComponentId[];
extern const char kTorClientComponentBase64PublicKey[];
#elif defined(OS_LINUX)
extern const char kTorClientComponentName[];
extern const char kTorClientComponentId[];
extern const char kTorClientComponentBase64PublicKey[];
#endif

class BraveTorClientUpdater : public BraveComponent {
 public:
  explicit BraveTorClientUpdater(BraveComponent::Delegate* delegate);
  ~BraveTorClientUpdater() override;

  void Register();
  base::FilePath GetExecutablePath() const;
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

 protected:
  void OnComponentReady(const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;

 private:
  friend class ::BraveTorClientUpdaterTest;
  static std::string g_tor_client_component_name_;
  static std::string g_tor_client_component_id_;
  static std::string g_tor_client_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  void InitExecutablePath(const base::FilePath& install_dir);
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool registered_;
  base::FilePath executable_path_;

  DISALLOW_COPY_AND_ASSIGN(BraveTorClientUpdater);
};

// Creates the BraveTorClientUpdater
std::unique_ptr<BraveTorClientUpdater>
BraveTorClientUpdaterFactory(BraveComponent::Delegate* delegate);

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_TOR_CLIENT_UPDATER_H_
