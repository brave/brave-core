// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_test_utils.h"

#include <utility>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

MockContainersServiceDelegate::MockContainersServiceDelegate() {
  ON_CALL(*this, GetReferencedContainerIds(testing::_))
      .WillByDefault([this](OnReferencedContainerIdsReadyCallback callback) {
        std::move(callback).Run(referenced_container_ids_);
      });
  ON_CALL(*this, DeleteContainerStorage(testing::_, testing::_))
      .WillByDefault([this](const std::string& id,
                            DeleteContainerStorageCallback callback) {
        delete_requests_.push_back(id);
        std::move(callback).Run(delete_result_);
      });
}

MockContainersServiceDelegate::~MockContainersServiceDelegate() = default;

mojom::ContainerPtr MakeContainer(std::string id,
                                  std::string name,
                                  mojom::Icon icon,
                                  SkColor color) {
  return mojom::Container::New(std::move(id), std::move(name), icon, color);
}

void ExpectContainer(const mojom::ContainerPtr& container,
                     const std::string& id,
                     const std::string& name,
                     mojom::Icon icon,
                     SkColor color) {
  ASSERT_TRUE(container);
  EXPECT_THAT(*container, testing::FieldsAre(id, name, icon, color));
}

}  // namespace containers
