/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_BRAVE_TOR_CLIENT_UPDATER_H_
#define BRAVE_COMPONENTS_TOR_BRAVE_TOR_CLIENT_UPDATER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

class BraveTorClientUpdaterTest;
class PrefService;

using brave_component_updater::BraveComponent;

namespace tor {

#if defined(OS_WIN)
extern const char kTorClientComponentName[];
extern const char kTorClientComponentId[];
#elif defined(OS_MAC)
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
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnExecutableReady(const base::FilePath& path) = 0;

   protected:
    ~Observer() override = default;
  };

  BraveTorClientUpdater(BraveComponent::Delegate* component_delegate,
                        PrefService* local_state,
                        const base::FilePath& user_data_dir);
  BraveTorClientUpdater(const BraveTorClientUpdater&) = delete;
  BraveTorClientUpdater& operator=(const BraveTorClientUpdater&) = delete;
  ~BraveTorClientUpdater() override;

  void Register();
  void Unregister();
  void Cleanup();
  base::FilePath GetExecutablePath() const;
  base::FilePath GetTorrcPath() const;
  base::FilePath GetTorDataPath() const;
  base::FilePath GetTorWatchPath() const;
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void OnComponentReady(const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;
  bool IsTorDisabled();

 private:
  friend class ::BraveTorClientUpdaterTest;

  void RemoveObsoleteFiles();

  static std::string g_tor_client_component_name_;
  static std::string g_tor_client_component_id_;
  static std::string g_tor_client_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  // <tor executable, torrc>
  void SetTorPath(const std::pair<base::FilePath, base::FilePath>&);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool registered_;
  base::FilePath executable_path_;
  base::FilePath torrc_path_;
  base::ObserverList<Observer> observers_;
  PrefService* local_state_;
  base::FilePath user_data_dir_;

  base::WeakPtrFactory<BraveTorClientUpdater> weak_ptr_factory_;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_BRAVE_TOR_CLIENT_UPDATER_H_
