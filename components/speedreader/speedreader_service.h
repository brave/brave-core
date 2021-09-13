/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_

#include <memory>

#include "brave/components/speedreader/common/speedreader_ui.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefRegistrySimple;
class PrefService;

namespace speedreader {

class SpeedreaderService : public KeyedService, public mojom::SpeedreaderUI {
 public:
  explicit SpeedreaderService(PrefService* prefs);
  ~SpeedreaderService() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // mojom::SpeedreaderUI:
  void AddObserver(
      mojo::PendingRemote<mojom::SpeedreaderUIObserver> observer) override;

  void Bind(mojo::PendingReceiver<mojom::SpeedreaderUI> receiver);

  void ToggleSpeedreader();
  void DisableSpeedreaderForTest();
  bool IsEnabled();
  bool ShouldPromptUserToEnable() const;
  void IncrementPromptCount();

  SpeedreaderService(const SpeedreaderService&) = delete;
  SpeedreaderService& operator=(const SpeedreaderService&) = delete;

 private:
  PrefService* prefs_ = nullptr;
  mojo::RemoteSet<mojom::SpeedreaderUIObserver> mojo_observers_;
  mojo::ReceiverSet<mojom::SpeedreaderUI> receivers_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
