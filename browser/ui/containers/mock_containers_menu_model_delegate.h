// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_MENU_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_MENU_MODEL_DELEGATE_H_

#include <optional>

#include "brave/browser/ui/containers/containers_menu_model.h"
#include "testing/gmock/include/gmock/gmock.h"

class MockContainerMenuModelDelegate : public ContainersMenuModel::Delegate {
 public:
  MockContainerMenuModelDelegate();
  ~MockContainerMenuModelDelegate() override;

  MOCK_METHOD(void,
              OnContainerSelected,
              (const containers::mojom::ContainerPtr& container),
              (override));

  MOCK_METHOD(std::optional<std::string_view>,
              GetCurrentContainerId,
              (),
              (override const));
};

#endif  // BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_MENU_MODEL_DELEGATE_H_
