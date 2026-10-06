// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/element_picker/element_picker_localized_texts.h"

@implementation ElementPickerLocalizedTexts

- (instancetype)initWithCreateDisabledText:(NSString*)createDisabledText
                         createEnabledText:(NSString*)createEnabledText
                                manageText:(NSString*)manageText
                             showRulesText:(NSString*)showRulesText
                             hideRulesText:(NSString*)hideRulesText
                                  quitText:(NSString*)quitText {
  self = [super init];
  if (self) {
    _createDisabledText = [createDisabledText copy];
    _createEnabledText = [createEnabledText copy];
    _manageText = [manageText copy];
    _showRulesText = [showRulesText copy];
    _hideRulesText = [hideRulesText copy];
    _quitText = [quitText copy];
  }
  return self;
}

@end
