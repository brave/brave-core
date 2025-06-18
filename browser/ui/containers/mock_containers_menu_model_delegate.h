// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_MENU_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_MENU_MODEL_DELEGATE_H_

#include "brave/browser/ui/containers/containers_menu_model.h"
#include "testing/gmock/include/gmock/gmock.h"

class MockContainerMenuModelDelegate : public ContainersMenuModel::Delegate {
 public:
  MockContainerMenuModelDelegate();
  ~MockContainerMenuModelDelegate() override;

  MOCK_METHOD(void, OnContainerSelected, (int container_id), (override));
};

#endif  // BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_MENU_MODEL_DELEGATE_H_
