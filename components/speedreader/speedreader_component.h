/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_COMPONENT_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_COMPONENT_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

namespace base {
class FilePath;
}

class GURL;

namespace speedreader {

class SpeedreaderComponent
    : public brave_component_updater::BraveComponent,
      public base::SupportsWeakPtr<SpeedreaderComponent> {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnStylesheetReady(const base::FilePath& path) = 0;

   protected:
    ~Observer() override = default;
  };

  explicit SpeedreaderComponent(Delegate* delegate);
  ~SpeedreaderComponent() override;

  SpeedreaderComponent(const SpeedreaderComponent&) = delete;
  SpeedreaderComponent& operator=(const SpeedreaderComponent&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  const base::FilePath& GetStylesheetPath() { return stylesheet_path_; }

 private:
  // brave_component_updater::BraveComponent:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  base::ObserverList<Observer> observers_;
  base::FilePath stylesheet_path_;
  base::WeakPtrFactory<SpeedreaderComponent> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_COMPONENT_H_
