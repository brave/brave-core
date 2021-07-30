/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <XCTest/XCTest.h>

#import "base/containers/flat_map.h"
#import "brave/build/ios/mojom/cpp_transformations.h"
#import "test_foo.h"

@interface DictionaryTransformTest : XCTestCase

@end

@implementation DictionaryTransformTest

- (void)testStringPrimitiveMapToDictionary {
  std::map<std::string, int> m{{"0", 0}, {"1", 1}, {"2", 2}};
  const auto dict = NSDictionaryFromMap(m);
  XCTAssertEqual(dict.count, static_cast<NSUInteger>(3));
  const auto keys = @[ @"0", @"1", @"2" ];
  const auto values = @[ @(0), @(1), @(2) ];
  XCTAssertTrue([dict.allKeys isEqualToArray:keys]);
  XCTAssertTrue([dict.allValues isEqualToArray:values]);
  XCTAssertEqual(dict[@"0"].integerValue, 0);
  XCTAssertEqual(dict[@"1"].integerValue, 1);
  XCTAssertEqual(dict[@"2"].integerValue, 2);
}

- (void)testStringStringMapToDictionary {
  std::map<std::string, std::string> m{
      {"0", "test0"}, {"1", "test1"}, {"2", "test2"}};
  const auto dict = NSDictionaryFromMap(m);
  XCTAssertEqual(dict.count, static_cast<NSUInteger>(3));
  const auto keys = @[ @"0", @"1", @"2" ];
  const auto values = @[ @"test0", @"test1", @"test2" ];
  XCTAssertTrue([dict.allKeys isEqualToArray:keys]);
  XCTAssertTrue([dict.allValues isEqualToArray:values]);
  XCTAssertTrue([dict[@"0"] isEqualToString:@"test0"]);
  XCTAssertTrue([dict[@"1"] isEqualToString:@"test1"]);
  XCTAssertTrue([dict[@"2"] isEqualToString:@"test2"]);
}

- (void)testStringCppStructToDictionary {
  std::map<std::string, CppFoo> m{
      {"0", CppFoo(true, 10, "test", {1.0, 2.0, 3.0})},
      {"1", CppFoo(false, 7, "test2", {3.0, 2.0, 1.0})},
  };
  const auto dict = NSDictionaryFromMap(m, ^TestFoo*(CppFoo foo) {
    return [[TestFoo alloc] initWithCppFoo:foo];
  });

  XCTAssertEqual(dict.count, static_cast<NSUInteger>(2));
  const auto keys = @[ @"0", @"1" ];
  XCTAssertTrue([dict.allKeys isEqualToArray:keys]);

  const auto foo = dict[@"0"];
  XCTAssertNotNil(foo);
  XCTAssertEqual(foo.boolean, true);
  XCTAssertEqual(foo.integer, 10);
  XCTAssertTrue([foo.stringObject isEqualToString:@"test"]);
  const auto numbers = @[ @(1.0), @(2.0), @(3.0) ];
  XCTAssertTrue([foo.numbers isEqualToArray:numbers]);

  const auto foo1 = dict[@"1"];
  XCTAssertNotNil(foo1);
  XCTAssertEqual(foo1.boolean, false);
  XCTAssertEqual(foo1.integer, 7);
  XCTAssertTrue([foo1.stringObject isEqualToString:@"test2"]);
  const auto numbers2 = @[ @(3.0), @(2.0), @(1.0) ];
  XCTAssertTrue([foo1.numbers isEqualToArray:numbers2]);
}

- (void)testStringNSDictionaryToStringMap {
  const auto d = @{@"1" : @"2", @"3" : @"4"};
  base::flat_map<std::string, std::string> map = MapFromNSDictionary(d);
  XCTAssert(map["1"] == "2");
  XCTAssert(map["3"] == "4");
  XCTAssert(map.find("5") == map.end());
}

@end
