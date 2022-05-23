// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

namespace base {

// Some empty class
class EmptyClass {
};

class SimpleClass {
  // first function
  void function1();

  // second function
  static void function2();
  void function3(int value);
  void function4(const SimpleClass& value1, const SimpleClass* value1, int value2);
};

};
