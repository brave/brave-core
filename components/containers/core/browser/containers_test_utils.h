// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_TEST_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_TEST_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/skia/include/core/SkColor.h"

namespace containers {

class MockContainersServiceDelegate : public ContainersService::Delegate {
 public:
  MockContainersServiceDelegate();
  ~MockContainersServiceDelegate() override;

  MOCK_METHOD(void,
              GetReferencedContainerIds,
              (OnReferencedContainerIdsReadyCallback),
              (override));
  MOCK_METHOD(void,
              DeleteContainerStorage,
              (const std::string&, DeleteContainerStorageCallback),
              (override));

  void SetReferencedContainersIds(base::flat_set<std::string> ids) {
    referenced_container_ids_ = std::move(ids);
  }

  void set_delete_result(bool delete_result) { delete_result_ = delete_result; }
  const std::vector<std::string>& delete_requests() const {
    return delete_requests_;
  }

 private:
  base::flat_set<std::string> referenced_container_ids_;
  bool delete_result_ = true;
  std::vector<std::string> delete_requests_;
};

mojom::ContainerPtr MakeContainer(std::string id,
                                  std::string name,
                                  mojom::Icon icon = mojom::Icon::kDefault,
                                  SkColor color = SK_ColorBLUE);

void ExpectContainer(const mojom::ContainerPtr& container,
                     const std::string& id,
                     const std::string& name,
                     mojom::Icon icon = mojom::Icon::kDefault,
                     SkColor color = SK_ColorBLUE);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_TEST_UTILS_H_
