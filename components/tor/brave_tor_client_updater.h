/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_BRAVE_TOR_CLIENT_UPDATER_H_
#define BRAVE_COMPONENTS_TOR_BRAVE_TOR_CLIENT_UPDATER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "build/build_config.h"

class PrefService;

using brave_component_updater::BraveComponent;

namespace tor {

extern const char kTorClientComponentId[];

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
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

  const base::FilePath& install_dir() const { return install_dir_; }
  const base::FilePath& executable() const { return executable_; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void OnComponentReady(const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;
  bool IsTorDisabled();

 private:
  void RemoveObsoleteFiles();

  // Called with a response for the search for the executable path.
  void OnExecutablePathFound(base::FilePath path);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool registered_;

  // The path where the binary has been ultimately installed.
  base::FilePath install_dir_;

  // The path for the tor executable.
  base::FilePath executable_;

  base::ObserverList<Observer> observers_;
  raw_ptr<PrefService> local_state_ = nullptr;
  base::FilePath user_data_dir_;

  base::WeakPtrFactory<BraveTorClientUpdater> weak_ptr_factory_;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_BRAVE_TOR_CLIENT_UPDATER_H_
