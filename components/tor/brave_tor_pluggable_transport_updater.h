/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_BRAVE_TOR_PLUGGABLE_TRANSPORT_UPDATER_H_
#define BRAVE_COMPONENTS_TOR_BRAVE_TOR_PLUGGABLE_TRANSPORT_UPDATER_H_

#include <cstdint>
#include <iterator>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "build/build_config.h"
#include "crypto/sha2.h"

class PrefService;

using brave_component_updater::BraveComponent;

namespace tor {

extern const char kSnowflakeExecutableName[];
extern const char kObfs4ExecutableName[];

#if BUILDFLAG(IS_WIN)
inline constexpr uint8_t kTorPluggableTransportComponentPublicKeySHA256[32] = {
    0x3d, 0xa2, 0x07, 0x7c, 0x52, 0x0d, 0xca, 0x97, 0xd9, 0x49, 0xee,
    0xc3, 0x87, 0x55, 0xe4, 0x5c, 0xd2, 0x8f, 0x09, 0xac, 0x4e, 0x37,
    0x4d, 0xdb, 0xa3, 0x1e, 0x30, 0x48, 0x7f, 0xff, 0xa8, 0x70};
static_assert(std::size(kTorPluggableTransportComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_MAC)
inline constexpr uint8_t kTorPluggableTransportComponentPublicKeySHA256[32] = {
    0x48, 0xd5, 0xd3, 0x9d, 0x22, 0xce, 0xe7, 0x2d, 0x6c, 0xbb, 0x3f,
    0xc4, 0xbb, 0x46, 0x99, 0xda, 0x47, 0x09, 0xe4, 0xe9, 0x80, 0x65,
    0xaa, 0xb0, 0xf4, 0xda, 0x33, 0x5c, 0x23, 0x2a, 0x37, 0xd9};
static_assert(std::size(kTorPluggableTransportComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_LINUX)
inline constexpr uint8_t kTorPluggableTransportComponentPublicKeySHA256[32] = {
    0x0f, 0x56, 0x68, 0x05, 0xe1, 0x0a, 0x90, 0x7d, 0xa2, 0x78, 0x42,
    0x1e, 0xc9, 0x68, 0x6a, 0xad, 0x02, 0x8f, 0x08, 0xf0, 0xb1, 0x3e,
    0xcd, 0xac, 0x31, 0xd6, 0x39, 0xd8, 0xe2, 0xeb, 0xc9, 0xc5};
static_assert(std::size(kTorPluggableTransportComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#endif

class BraveTorPluggableTransportUpdater : public BraveComponent {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPluggableTransportReady(bool success) {}

   protected:
    ~Observer() override = default;
  };

  BraveTorPluggableTransportUpdater(
      BraveComponent::Delegate* component_delegate,
      PrefService* local_state,
      const base::FilePath& user_data_dir);
  BraveTorPluggableTransportUpdater(const BraveTorPluggableTransportUpdater&) =
      delete;
  BraveTorPluggableTransportUpdater& operator=(
      const BraveTorPluggableTransportUpdater&) = delete;
  ~BraveTorPluggableTransportUpdater() override;

  void Register();
  void Unregister();
  void Cleanup();

  bool IsReady() const;
  const base::FilePath& GetSnowflakeExecutable() const;
  const base::FilePath& GetObfs4Executable() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  void OnInitialized(const base::FilePath& install_dir, bool succeeded);

  bool registered_ = false;
  bool is_ready_ = false;

  base::ObserverList<Observer> observers_;
  raw_ptr<PrefService> local_state_ = nullptr;
  base::FilePath user_data_dir_;
  base::FilePath snowflake_path_;  // Relative to the user data dir
  base::FilePath obfs4_path_;      // Relative to the user data dir

  base::WeakPtrFactory<BraveTorPluggableTransportUpdater> weak_ptr_factory_{
      this};
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_BRAVE_TOR_PLUGGABLE_TRANSPORT_UPDATER_H_
