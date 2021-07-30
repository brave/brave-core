/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <XCTest/XCTest.h>
#import "brave/build/ios/mojom/cpp_transformations.h"
#import "test_foo.h"

@interface VectorTransformTest : XCTestCase
@end

@implementation VectorTransformTest

- (void)testPrimitiveVectorToNSNumberArray {
  std::vector<int> v{1, 2, 3};
  auto array = NSArrayFromVector(v);
  XCTAssertTrue(array.count == 3);
  XCTAssertTrue(array[0].intValue == 1 && array[1].intValue == 2 &&
                array[2].intValue == 3);
}

- (void)testNSNumberArrayToPrimitiveVector {
  NSArray<NSNumber*>* a = @[ @(1), @(2), @(3) ];
  std::vector<int> v = VectorFromNSArray<int>(a);
  XCTAssertTrue(v.size() == 3);
  XCTAssertTrue(v[0] == 1 && v[1] == 2 && v[2] == 3);
}

- (void)testStringVectorToStringArray {
  std::vector<std::string> v{"1", "2", "3"};
  auto array = NSArrayFromVector(v);
  XCTAssertTrue(array.count == 3);
  XCTAssertTrue([array[0] isEqualToString:@"1"] &&
                [array[1] isEqualToString:@"2"] &&
                [array[2] isEqualToString:@"3"]);
}

- (void)testStringArrayToStringVector {
  NSArray<NSString*>* a = @[ @"1", @"2", @"3" ];
  std::vector<std::string> v = VectorFromNSArray(a);
  XCTAssertTrue(v.size() == 3);
  XCTAssertTrue(v[0] == "1" && v[1] == "2" && v[2] == "3");
}

- (void)testVectorObjectsToArrayObjects {
  std::vector<CppFoo> foos{
      CppFoo(true, 10, "test", {1.0, 2.0, 3.0}),
      CppFoo(false, 7, "tset", {3.0, 2.0, 1.0}),
  };
  const auto array = NSArrayFromVector(foos, ^TestFoo*(const CppFoo& foo) {
    return [[TestFoo alloc] initWithCppFoo:foo];
  });
  XCTAssertTrue(array.count == 2);
  XCTAssertTrue(array[0].boolean == true && array[0].integer == 10 &&
                [array[0].stringObject isEqualToString:@"test"] &&
                array[0].numbers.count == 3);
  XCTAssertTrue(array[1].boolean == false && array[1].integer == 7 &&
                [array[1].stringObject isEqualToString:@"tset"] &&
                array[1].numbers.count == 3);
}

@end
