// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_SERVICE_H_
#define BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_SERVICE_H_

#include <string>
#include <vector>

#include "ui/base/models/image_model.h"

struct ContainerItem {
  int id = -1;  // Unique identifier for the container.
  ui::ImageModel icon;
  std::u16string name;
};

// Test / In development purpose only mock service to simulate the Containers
// functionality.
class MockContainersService {
 public:
  static MockContainersService& GetInstance();
  MockContainersService(const MockContainersService&) = delete;
  MockContainersService& operator=(const MockContainersService&) = delete;
  ~MockContainersService();

  const std::vector<ContainerItem>& containers() const { return containers_; }

  void set_selected_container_id(int id) { selected_container_id_ = id; }
  int current_tab_container_id() const { return selected_container_id_; }

  void AddContainer(const ContainerItem& item);
  void RemoveContainerById(int id);

 private:
  MockContainersService();

  std::vector<ContainerItem> containers_ = {
      {.id = 0, .icon = {}, .name = u"Example Container 1"},
      {.id = 1, .icon = {}, .name = u"Example Container 2"}};
  int selected_container_id_ = -1;
};

#endif  // BRAVE_BROWSER_UI_CONTAINERS_MOCK_CONTAINERS_SERVICE_H_
