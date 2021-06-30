/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "test_foo.h"
#import "brave/build/ios/mojom/cpp_transformations.h"

CppFoo::CppFoo(const CppFoo& foo)
    : boolean(foo.boolean),
      integer(foo.integer),
      stringObject(foo.stringObject),
      numbers(foo.numbers) {}
CppFoo::CppFoo(bool b, int i, std::string s, std::vector<double> ds)
    : boolean(b), integer(i), stringObject(s), numbers(ds) {}
CppFoo::~CppFoo() {}

@implementation TestFoo

- (instancetype)initWithCppFoo:(const CppFoo&)foo {
  if ((self = [super init])) {
    self.boolean = foo.boolean;
    self.integer = foo.integer;
    self.numbers = NSArrayFromVector(foo.numbers);
    self.stringObject =
        [NSString stringWithUTF8String:foo.stringObject.c_str()];
  }
  return self;
}

@end
