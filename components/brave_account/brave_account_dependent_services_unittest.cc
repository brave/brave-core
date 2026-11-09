/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_dependent_services.h"

#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_account/mock_brave_account_dependent_services_observer.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/dependent_services.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using testing::ElementsAre;
using testing::InSequence;
using testing::IsEmpty;
using testing::StrictMock;

class BraveAccountDependentServicesTest : public testing::Test {
 protected:
  void SetUp() override {
    dependent_services_.BindInterface(
        services_remote_.BindNewPipeAndPassReceiver());
    dependent_services_.BindInterface(
        registrar_remote_.BindNewPipeAndPassReceiver());
  }

  void AddObserver(MockBraveAccountDependentServicesObserver& observer) {
    services_remote_->AddObserver(observer.BindAndGetRemote());
    services_remote_.FlushForTesting();
  }

  mojo::Remote<mojom::DependentServiceRegistration> Register(
      mojom::Service service) {
    mojo::Remote<mojom::DependentServiceRegistration> registration;
    registrar_remote_->Register(service,
                                registration.BindNewPipeAndPassReceiver());
    registrar_remote_.FlushForTesting();
    return registration;
  }

  base::test::TaskEnvironment task_environment_;
  BraveAccountDependentServices dependent_services_;
  // The test, acting as the Account UI, adds observers to the registry.
  mojo::Remote<mojom::DependentServices> services_remote_;
  // The test, acting as a dependent service, registers with the registry.
  mojo::Remote<mojom::DependentServiceRegistrar> registrar_remote_;
};

namespace {

struct NewObserverTestCase {
  std::string test_name;
  std::vector<mojom::Service> registered_services;
  std::vector<mojom::Service> expected_services;
};

}  // namespace

class BraveAccountDependentServicesNewObserverTest
    : public BraveAccountDependentServicesTest,
      public testing::WithParamInterface<NewObserverTestCase> {};

TEST_P(BraveAccountDependentServicesNewObserverTest,
       NotifiesOfRegisteredServices) {
  const auto& test_case = GetParam();

  std::vector<mojo::Remote<mojom::DependentServiceRegistration>> registrations;
  for (const auto service : test_case.registered_services) {
    registrations.push_back(Register(service));
  }

  StrictMock<MockBraveAccountDependentServicesObserver> observer;
  EXPECT_CALL(observer,
              OnDependentServicesChanged(test_case.expected_services));

  AddObserver(observer);
  observer.FlushForTesting();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountDependentServicesTests,
    BraveAccountDependentServicesNewObserverTest,
    testing::Values(NewObserverTestCase{.test_name = "no_services",
                                        .registered_services = {},
                                        .expected_services = {}},
                    NewObserverTestCase{
                        .test_name = "one_service",
                        .registered_services = {mojom::Service::kEmailAliases},
                        .expected_services = {mojom::Service::kEmailAliases}},
                    NewObserverTestCase{
                        .test_name = "services_in_order",
                        .registered_services = {mojom::Service::kSync,
                                                mojom::Service::kEmailAliases},
                        .expected_services = {mojom::Service::kEmailAliases,
                                              mojom::Service::kSync}},
                    NewObserverTestCase{
                        .test_name = "service_registered_twice_listed_once",
                        .registered_services = {mojom::Service::kEmailAliases,
                                                mojom::Service::kEmailAliases},
                        .expected_services = {mojom::Service::kEmailAliases}}),
    [](const auto& info) { return info.param.test_name; });

TEST_F(BraveAccountDependentServicesTest, NotifiesObserverOfChanges) {
  StrictMock<MockBraveAccountDependentServicesObserver> observer;
  base::RunLoop run_loop;
  {
    InSequence sequence;
    EXPECT_CALL(observer, OnDependentServicesChanged(IsEmpty()));
    EXPECT_CALL(observer, OnDependentServicesChanged(
                              ElementsAre(mojom::Service::kEmailAliases)));
    EXPECT_CALL(observer,
                OnDependentServicesChanged(ElementsAre(
                    mojom::Service::kEmailAliases, mojom::Service::kSync)));
    EXPECT_CALL(observer,
                OnDependentServicesChanged(ElementsAre(mojom::Service::kSync)))
        .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));
  }

  AddObserver(observer);
  auto email_aliases_registration = Register(mojom::Service::kEmailAliases);
  const auto sync_registration = Register(mojom::Service::kSync);
  email_aliases_registration.reset();
  run_loop.Run();
}

TEST_F(BraveAccountDependentServicesTest,
       ListsServiceOnceWhileAnyOfItsRegistrationsIsOpen) {
  StrictMock<MockBraveAccountDependentServicesObserver> observer;
  base::RunLoop run_loop;
  {
    InSequence sequence;
    EXPECT_CALL(observer, OnDependentServicesChanged(IsEmpty()));
    EXPECT_CALL(observer, OnDependentServicesChanged(
                              ElementsAre(mojom::Service::kEmailAliases)))
        .Times(3);
    EXPECT_CALL(observer, OnDependentServicesChanged(IsEmpty()))
        .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));
  }

  AddObserver(observer);
  auto registration1 = Register(mojom::Service::kEmailAliases);
  auto registration2 = Register(mojom::Service::kEmailAliases);
  registration1.reset();
  registration2.reset();
  run_loop.Run();
}

}  // namespace brave_account
