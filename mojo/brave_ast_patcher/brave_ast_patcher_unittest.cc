/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/mojo/brave_ast_patcher/test_module.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

// This file contains patched mojom usage to ensure that all new definitions,
// values and members are properly added using the patching mechanism.

namespace brave_ast_patcher {
namespace mojom {

// Create interface implementations to ensure new methods exist and are
// overridable.
class GlobalInterfaceImpl : public GlobalInterface {
 public:
  GlobalInterfaceImpl() = default;
  ~GlobalInterfaceImpl() override = default;

  void TestNestedEnum(TestNestedEnumCallback callback) override {}
  void TestNewNestedEnum(TestNewNestedEnumCallback callback) override {}
  void TestNewGlobalEnum(NewGlobalEnum new_global_enum,
                         TestNewGlobalEnumCallback callback) override {}
  void TestNewGlobalStruct(NewGlobalStructPtr new_global_struct,
                           TestNewGlobalStructCallback callback) override {}
  void TestNewGlobalUnion(NewGlobalUnionPtr new_global_union,
                          TestNewGlobalUnionCallback callback) override {}
};

class NewGlobalInterfaceImpl : public NewGlobalInterface {
 public:
  NewGlobalInterfaceImpl() = default;
  ~NewGlobalInterfaceImpl() override = default;

  void TestNestedEnum(TestNestedEnumCallback callback) override {}
  void TestNewGlobalEnum(NewGlobalEnum new_global_enum,
                         TestNewGlobalEnumCallback callback) override {}
  void TestNewGlobalStruct(NewGlobalStructPtr new_global_struct,
                           TestNewGlobalStructCallback callback) override {}
  void TestNewGlobalUnion(NewGlobalUnionPtr new_global_union,
                          TestNewGlobalUnionCallback callback) override {}
};

// Test existing types have new values, members.
TEST(BraveAstPatcherTest, OriginalTypes) {
  EXPECT_EQ(static_cast<int>(GlobalEnum::VALUE), 0);
  EXPECT_EQ(static_cast<int>(GlobalEnum::VALUE_GLOBAL_CONSTANT),
            kGlobalConstant1);

  EXPECT_EQ(static_cast<int>(GlobalStruct::NestedEnum::VALUE), 0);
  EXPECT_EQ(static_cast<int>(GlobalStruct::NestedEnum::VALUE_NESTED_CONSTANT),
            GlobalStruct::kNestedConstant1);

  GlobalStruct global_struct;
  EXPECT_EQ(global_struct.global_enum_member,
            GlobalEnum::VALUE_GLOBAL_CONSTANT);
  EXPECT_EQ(global_struct.nested_enum_member,
            GlobalStruct::NestedEnum::VALUE_NESTED_CONSTANT);

  GlobalUnion global_union;
  global_union.set_bool_value(false);
  global_union.set_string_value(std::string());

  EXPECT_EQ(static_cast<int>(GlobalInterface::NestedEnum::VALUE), 0);
  EXPECT_EQ(
      static_cast<int>(GlobalInterface::NestedEnum::VALUE_NESTED_CONSTANT),
      GlobalInterface::kNestedConstant1);
}

// Test new types are added.
TEST(BraveAstPatcherTest, NewTypes) {
  EXPECT_EQ(static_cast<int>(NewGlobalEnum::VALUE), 0);
  EXPECT_EQ(static_cast<int>(NewGlobalEnum::VALUE_GLOBAL_CONSTANT),
            kGlobalConstant2);

  EXPECT_EQ(static_cast<int>(NewGlobalStruct::NestedEnum::VALUE), 0);
  EXPECT_EQ(
      static_cast<int>(NewGlobalStruct::NestedEnum::VALUE_NESTED_CONSTANT),
      NewGlobalStruct::kNestedConstant1);

  NewGlobalStruct new_global_struct;
  EXPECT_EQ(new_global_struct.new_global_enum_member,
            NewGlobalEnum::VALUE_GLOBAL_CONSTANT);
  EXPECT_EQ(new_global_struct.nested_enum_member,
            NewGlobalStruct::NestedEnum::VALUE_NESTED_CONSTANT);
  new_global_struct.new_global_enum_member = NewGlobalEnum::VALUE;
  new_global_struct.nested_enum_member = NewGlobalStruct::NestedEnum::VALUE;

  NewGlobalUnion new_global_union;
  new_global_union.set_int32_value(0);
  new_global_union.set_float_value(0.0f);

  EXPECT_EQ(static_cast<int>(NewGlobalInterface::NestedEnum::VALUE), 0);
  EXPECT_EQ(
      static_cast<int>(NewGlobalInterface::NestedEnum::VALUE_NESTED_CONSTANT),
      NewGlobalInterface::kNestedConstant1);
}

// Test existing types are extended with new values, members.
TEST(BraveAstPatcherTest, ExtendedTypes) {
  EXPECT_EQ(static_cast<int>(GlobalEnum::NEW_VALUE), 1);
  EXPECT_EQ(static_cast<int>(GlobalEnum::NEW_VALUE), 1);
  EXPECT_EQ(static_cast<int>(GlobalEnum::NEW_VALUE_GLOBAL_CONSTANT),
            kGlobalConstant2);

  EXPECT_EQ(static_cast<int>(GlobalStruct::NestedEnum::NEW_VALUE), 1);
  EXPECT_EQ(
      static_cast<int>(GlobalStruct::NestedEnum::NEW_VALUE_NESTED_CONSTANT),
      GlobalStruct::kNestedConstant2);

  EXPECT_EQ(static_cast<int>(GlobalStruct::NewNestedEnum::VALUE), 1);
  EXPECT_EQ(
      static_cast<int>(GlobalStruct::NewNestedEnum::VALUE_NESTED_CONSTANT),
      GlobalStruct::kNestedConstant2);

  GlobalStruct global_struct;
  EXPECT_TRUE(global_struct.new_string_member.empty());
  EXPECT_EQ(global_struct.new_global_enum_member,
            NewGlobalEnum::VALUE_GLOBAL_CONSTANT);
  EXPECT_EQ(global_struct.new_nested_enum_member,
            GlobalStruct::NewNestedEnum::VALUE_NESTED_CONSTANT);

  GlobalUnion global_union;
  global_union.set_int32_value(0);
  global_union.set_float_value(0.0f);

  EXPECT_EQ(static_cast<int>(GlobalInterface::NestedEnum::NEW_VALUE), 1);
  EXPECT_EQ(
      static_cast<int>(GlobalInterface::NestedEnum::NEW_VALUE_NESTED_CONSTANT),
      GlobalInterface::kNestedConstant2);

  EXPECT_EQ(static_cast<int>(GlobalInterface::NewNestedEnum::VALUE), 1);
  EXPECT_EQ(
      static_cast<int>(GlobalInterface::NewNestedEnum::VALUE_NESTED_CONSTANT),
      GlobalInterface::kNestedConstant2);
}

// Test interfaces can properly instantiated.
TEST(BraveAstPatcherTest, InterfaceInstantiations) {
  GlobalInterfaceImpl global_interface_impl;
  NewGlobalInterfaceImpl new_global_interface_impl;
}

}  // namespace mojom
}  // namespace brave_ast_patcher
