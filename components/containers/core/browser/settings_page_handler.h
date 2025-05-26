// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_SETTINGS_PAGE_HANDLER_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_SETTINGS_PAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace containers {

class SettingsPageHandler : public mojom::SettingsPageHandler {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void RemoveContainerData(const std::string& id,
                                     base::OnceClosure callback) = 0;
  };

  SettingsPageHandler(mojo::PendingRemote<mojom::SettingsPage> page,
                      PrefService* prefs,
                      std::unique_ptr<Delegate> delegate);
  ~SettingsPageHandler() override;

  SettingsPageHandler(const SettingsPageHandler&) = delete;
  SettingsPageHandler& operator=(const SettingsPageHandler&) = delete;

  // mojom::SettingsPageHandler
  void GetContainers(GetContainersCallback callback) override;

  void AddContainer(mojom::ContainerPtr container) override;
  void UpdateContainer(mojom::ContainerPtr container) override;
  void RemoveContainer(const std::string& id,
                       RemoveContainerCallback callback) override;

 private:
  void OnContainersChanged();
  void OnContainerDataRemoved(const std::string& id,
                              RemoveContainerCallback callback);

  mojo::Remote<mojom::SettingsPage> page_;
  raw_ptr<PrefService> prefs_;
  std::unique_ptr<Delegate> delegate_;
  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<SettingsPageHandler> weak_ptr_factory_{this};
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_SETTINGS_PAGE_HANDLER_H_
