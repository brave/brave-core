/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <string>
#import <vector>

NS_ASSUME_NONNULL_BEGIN

struct CppFoo {
  CppFoo(const CppFoo &);
  CppFoo(bool b, int i, std::string s, std::vector<double> ds);
  ~CppFoo();

  bool boolean;
  int integer;
  std::string stringObject;
  std::vector<double> numbers;
};

@interface TestFoo : NSObject
@property (nonatomic, assign) BOOL boolean;
@property (nonatomic, assign) int integer;
@property (nonatomic, copy) NSString *stringObject;
@property (nonatomic, copy) NSArray<NSNumber *> *numbers;
- (instancetype)initWithCppFoo:(const CppFoo&)foo;
@end

NS_ASSUME_NONNULL_END
