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

// Handles container management operations from the settings page. This class
// implements the mojom::SettingsPageHandler interface to process requests from
// WebUI and manages container data persistence through prefs.
class SettingsPageHandler : public mojom::SettingsPageHandler {
 public:
  // Delegate interface for container data cleanup operations that need to be
  // handled by the browser process.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Removes all data associated with the specified container.
    // - `id`: The ID of the container whose data should be removed.
    // - `callback`: Called when data removal is complete
    virtual void RemoveContainerData(const std::string& id,
                                     base::OnceClosure callback) = 0;
  };

  SettingsPageHandler(mojo::PendingRemote<mojom::SettingsPage> page,
                      PrefService* prefs,
                      std::unique_ptr<Delegate> delegate);
  ~SettingsPageHandler() override;

  SettingsPageHandler(const SettingsPageHandler&) = delete;
  SettingsPageHandler& operator=(const SettingsPageHandler&) = delete;

  // mojom::SettingsPageHandler:
  //
  // Retrieves the current list of containers from preferences.
  void GetContainers(GetContainersCallback callback) override;
  // Creates a new container and stores it in preferences.
  void AddContainer(mojom::ContainerPtr container) override;
  // Updates an existing container's properties in preferences.
  void UpdateContainer(mojom::ContainerPtr container) override;
  // Removes a container and all its associated data. Returns async response
  // after data cleanup is complete.
  void RemoveContainer(const std::string& id,
                       RemoveContainerCallback callback) override;

 private:
  // Called when the containers list in preferences changes.
  void OnContainersChanged();

  // Called when container data removal is complete.
  void OnContainerDataRemoved(const std::string& id,
                              RemoveContainerCallback callback);

  // Interface to communicate with the settings page in the renderer.
  mojo::Remote<mojom::SettingsPage> page_;

  // Profile preferences service for container data persistence.
  raw_ptr<PrefService> prefs_ = nullptr;

  // Delegate for browser-side container operations.
  std::unique_ptr<Delegate> delegate_;

  // Watches for changes to container-related preferences.
  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<SettingsPageHandler> weak_ptr_factory_{this};
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_SETTINGS_PAGE_HANDLER_H_
