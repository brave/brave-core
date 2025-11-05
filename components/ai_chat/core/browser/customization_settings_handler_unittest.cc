// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/customization_settings_handler.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::StrictMock;

namespace ai_chat {

class MockCustomizationSettingsUI : public mojom::CustomizationSettingsUI {
 public:
  MOCK_METHOD(void,
              OnCustomizationsChanged,
              (mojom::CustomizationsPtr),
              (override));
  MOCK_METHOD(void,
              OnMemoriesChanged,
              (const std::vector<std::string>&),
              (override));
};

class CustomizationSettingsHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    prefs::RegisterProfilePrefs(pref_service_->registry());

    handler_ =
        std::make_unique<CustomizationSettingsHandler>(pref_service_.get());
  }

  void TearDown() override {
    handler_.reset();
    pref_service_.reset();
  }

 protected:
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<CustomizationSettingsHandler> handler_;
  base::test::TaskEnvironment task_environment_;
};

TEST_F(CustomizationSettingsHandlerTest, GetCustomizations) {
  // Empty customizations
  {
    base::test::TestFuture<mojom::CustomizationsPtr> future;
    handler_->GetCustomizations(future.GetCallback());
    const mojom::CustomizationsPtr& result = future.Get();
    EXPECT_TRUE(result->name.empty());
    EXPECT_TRUE(result->job.empty());
    EXPECT_TRUE(result->tone.empty());
    EXPECT_TRUE(result->other.empty());
  }

  // Non-empty customizations
  {
    prefs::SetCustomizationsToPrefs(
        mojom::Customizations::New("John Doe", "Software Engineer",
                                   "Professional", "Loves coding"),
        *pref_service_);

    base::test::TestFuture<mojom::CustomizationsPtr> future;
    handler_->GetCustomizations(future.GetCallback());
    const mojom::CustomizationsPtr& result = future.Get();
    EXPECT_EQ(result->name, "John Doe");
    EXPECT_EQ(result->job, "Software Engineer");
    EXPECT_EQ(result->tone, "Professional");
    EXPECT_EQ(result->other, "Loves coding");
  }
}

TEST_F(CustomizationSettingsHandlerTest, SetCustomizations_Valid) {
  // Create and bind the mock UI to the handler
  auto mock_ui = std::make_unique<StrictMock<MockCustomizationSettingsUI>>();
  mojo::Receiver<mojom::CustomizationSettingsUI> receiver(mock_ui.get());
  handler_->BindUI(receiver.BindNewPipeAndPassRemote());

  auto customizations = mojom::Customizations::New(
      "John Doe", "Software Engineer", "Professional", "Loves coding");

  // Use RunLoop with Quit to wait for the callback
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_ui, OnCustomizationsChanged(_))
      .Times(1)
      .WillOnce([&](mojom::CustomizationsPtr result) {
        EXPECT_EQ(customizations, result);
        run_loop.Quit();
      });

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->SetCustomizations(customizations.Clone(), future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());  // No error

  // Wait for the callback to be called
  run_loop.Run();

  // Verify the preferences were set
  EXPECT_EQ(prefs::GetCustomizationsFromPrefs(*pref_service_), customizations);
}

TEST_F(CustomizationSettingsHandlerTest, SetCustomizations_InvalidLength) {
  auto customizations = mojom::Customizations::New();
  customizations->name =
      std::string(mojom::kMaxMemoryRecordLength + 1, 'a');  // Too long
  customizations->job = "Software Engineer";
  customizations->tone = "Professional";
  customizations->other = "Loves coding";

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->SetCustomizations(customizations.Clone(), future.GetCallback());
  EXPECT_TRUE(future.Get().has_value());
  EXPECT_EQ(future.Get().value(),
            mojom::CustomizationOperationError::kInvalidLength);
}

TEST_F(CustomizationSettingsHandlerTest, AddMemory_Valid) {
  // Create and bind the mock UI to the handler
  auto mock_ui = std::make_unique<StrictMock<MockCustomizationSettingsUI>>();
  mojo::Receiver<mojom::CustomizationSettingsUI> receiver(mock_ui.get());
  handler_->BindUI(receiver.BindNewPipeAndPassRemote());

  base::RunLoop run_loop;
  EXPECT_CALL(*mock_ui, OnMemoriesChanged(_))
      .Times(1)
      .WillOnce([&](const std::vector<std::string>& result) {
        EXPECT_EQ(result,
                  std::vector<std::string>{"I like creative solutions"});
        run_loop.Quit();
      });

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->AddMemory("I like creative solutions", future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());

  // Wait for the callback to be called
  run_loop.Run();

  // Verify the preferences were set
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"I like creative solutions"});
}

TEST_F(CustomizationSettingsHandlerTest, AddMemory_Duplicate) {
  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->AddMemory("I like creative solutions", future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future2;
  handler_->AddMemory("I like creative solutions", future2.GetCallback());
  EXPECT_FALSE(future2.Get().has_value());

  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"I like creative solutions"});
}

TEST_F(CustomizationSettingsHandlerTest, AddMemory_Empty) {
  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->AddMemory("", future.GetCallback());  // Empty memory
  EXPECT_TRUE(future.Get().has_value());
  EXPECT_EQ(future.Get().value(),
            mojom::CustomizationOperationError::kInvalidLength);
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{});
}

TEST_F(CustomizationSettingsHandlerTest, AddMemory_TooLong) {
  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->AddMemory(std::string(mojom::kMaxMemoryRecordLength + 1, 'a'),
                      future.GetCallback());
  EXPECT_TRUE(future.Get().has_value());
  EXPECT_EQ(future.Get().value(),
            mojom::CustomizationOperationError::kInvalidLength);
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{});
}

TEST_F(CustomizationSettingsHandlerTest, EditMemory_Success) {
  // Create and bind the mock UI to the handler
  auto mock_ui = std::make_unique<StrictMock<MockCustomizationSettingsUI>>();
  mojo::Receiver<mojom::CustomizationSettingsUI> receiver(mock_ui.get());
  handler_->BindUI(receiver.BindNewPipeAndPassRemote());

  base::RunLoop run_loop;
  EXPECT_CALL(*mock_ui, OnMemoriesChanged(_))
      .Times(1)
      .WillOnce([&](const std::vector<std::string>& result) {
        EXPECT_EQ(result, std::vector<std::string>{"Old memory"});
        run_loop.Quit();
      });

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->AddMemory("Old memory", future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());  // First add should succeed

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_ui.get());

  base::RunLoop run_loop2;
  EXPECT_CALL(*mock_ui, OnMemoriesChanged(_))
      .Times(1)
      .WillOnce([&](const std::vector<std::string>& result) {
        EXPECT_EQ(result, std::vector<std::string>{"New memory"});
        run_loop2.Quit();
      });

  // Edit the memory
  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future2;
  handler_->EditMemory("Old memory", "New memory", future2.GetCallback());

  std::optional<mojom::CustomizationOperationError> error = future2.Get();
  EXPECT_FALSE(error.has_value());  // No error

  run_loop2.Run();

  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"New memory"});
}

TEST_F(CustomizationSettingsHandlerTest, EditMemory_NotFound) {
  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->EditMemory("Non-existent memory", "New memory",
                       future.GetCallback());

  std::optional<mojom::CustomizationOperationError> error = future.Get();
  EXPECT_TRUE(error.has_value());
  EXPECT_EQ(error.value(), mojom::CustomizationOperationError::kNotFound);
}

TEST_F(CustomizationSettingsHandlerTest, EditMemory_EmptyNewMemory) {
  // Add initial memory
  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->AddMemory("Old memory", future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());  // First add should succeed

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future2;
  handler_->EditMemory("Old memory", "",
                       future2.GetCallback());  // Empty new memory

  std::optional<mojom::CustomizationOperationError> error = future2.Get();
  EXPECT_TRUE(error.has_value());
  EXPECT_EQ(error.value(), mojom::CustomizationOperationError::kInvalidLength);

  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"Old memory"});
}

TEST_F(CustomizationSettingsHandlerTest, EditMemory_NewMemoryTooLong) {
  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->AddMemory("Old memory", future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());  // First add should succeed

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future2;
  handler_->EditMemory("Old memory",
                       std::string(mojom::kMaxMemoryRecordLength + 1, 'a'),
                       future2.GetCallback());
  EXPECT_TRUE(future2.Get().has_value());
  EXPECT_EQ(future2.Get().value(),
            mojom::CustomizationOperationError::kInvalidLength);

  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"Old memory"});
}

TEST_F(CustomizationSettingsHandlerTest, GetMemories) {
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{});
  prefs::AddMemoryToPrefs("Memory 1", *pref_service_);
  prefs::AddMemoryToPrefs("Memory 2", *pref_service_);

  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            (std::vector<std::string>{"Memory 1", "Memory 2"}));
}

TEST_F(CustomizationSettingsHandlerTest, DeleteMemory_Success) {
  prefs::AddMemoryToPrefs("Memory 1", *pref_service_);
  prefs::AddMemoryToPrefs("Memory 2", *pref_service_);
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            (std::vector<std::string>{"Memory 1", "Memory 2"}));
  handler_->DeleteMemory("Memory 1");
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"Memory 2"});
}

TEST_F(CustomizationSettingsHandlerTest, DeleteMemory_NonExistent) {
  prefs::AddMemoryToPrefs("Memory 1", *pref_service_);
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"Memory 1"});
  handler_->DeleteMemory("Non-existent memory");
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"Memory 1"});
}

TEST_F(CustomizationSettingsHandlerTest, DeleteAllMemories) {
  // Add memories
  prefs::AddMemoryToPrefs("Memory 1", *pref_service_);
  prefs::AddMemoryToPrefs("Memory 2", *pref_service_);
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            (std::vector<std::string>{"Memory 1", "Memory 2"}));

  // Delete all memories
  handler_->DeleteAllMemories();
  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{});
}

TEST_F(CustomizationSettingsHandlerTest, BindUI_Notifications) {
  // Create and bind the mock UI to the handler
  auto mock_ui = std::make_unique<StrictMock<MockCustomizationSettingsUI>>();
  mojo::Receiver<mojom::CustomizationSettingsUI> receiver(mock_ui.get());
  handler_->BindUI(receiver.BindNewPipeAndPassRemote());

  auto customizations = mojom::Customizations::New();
  customizations->name = "Test Name";

  base::RunLoop run_loop;
  EXPECT_CALL(*mock_ui, OnCustomizationsChanged(_))
      .Times(1)
      .WillOnce([&](mojom::CustomizationsPtr result) {
        EXPECT_EQ(customizations, result);
        run_loop.Quit();
      });

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future;
  handler_->SetCustomizations(customizations.Clone(), future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());  // First add should succeed

  run_loop.Run();

  EXPECT_EQ(prefs::GetCustomizationsFromPrefs(*pref_service_), customizations);
  testing::Mock::VerifyAndClearExpectations(mock_ui.get());

  base::RunLoop run_loop2;
  EXPECT_CALL(*mock_ui, OnMemoriesChanged(_))
      .Times(1)
      .WillOnce([&](const std::vector<std::string>& result) {
        EXPECT_EQ(result, std::vector<std::string>{"Test Memory"});
        run_loop2.Quit();
      });

  base::test::TestFuture<std::optional<mojom::CustomizationOperationError>>
      future2;
  handler_->AddMemory("Test Memory", future2.GetCallback());
  EXPECT_FALSE(future2.Get().has_value());

  run_loop2.Run();

  EXPECT_EQ(prefs::GetMemoriesFromPrefs(*pref_service_),
            std::vector<std::string>{"Test Memory"});
}

}  // namespace ai_chat
