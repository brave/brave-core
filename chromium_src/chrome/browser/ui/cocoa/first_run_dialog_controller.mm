/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/cocoa/first_run_dialog_controller.h"

#include "base/i18n/rtl.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "ui/base/cocoa/controls/button_utils.h"
#include "ui/base/cocoa/controls/textfield_utils.h"
#include "ui/base/l10n/l10n_util_mac.h"

@implementation FirstRunDialogViewController {
  BOOL _setAsDefaultBrowser;
}

- (instancetype)initWithStatsCheckboxInitiallyChecked:(BOOL)checked
                        defaultBrowserCheckboxVisible:(BOOL)visible {
  if ((self = [super init])) {
    _setAsDefaultBrowser = NO;
  }
  return self;
}

- (void)loadView {
  constexpr int kDialogWidth = 400;
  constexpr int kPadding = 24;
  constexpr int kLabelSpacing = 16;
  constexpr int kContentsWidth = kDialogWidth - (2 * kPadding);
  constexpr int kTopPadding = 20;
  constexpr int kButtonTopMargin = 40;
  constexpr int kButtonBottomMargin = 20;
  constexpr int kButtonHorizontalMargin = 20;

  // After calculating all controls' proper height based on |kContentsWidth|,
  // window's height will be calculated.
  std::u16string headerString =
      brave_l10n::GetLocalizedResourceUTF16String(IDS_FIRSTRUN_DLG_HEADER_TEXT);
  base::i18n::AdjustStringForLocaleDirection(&headerString);
  NSTextField* headerLabel =
      [TextFieldUtils labelWithString:base::SysUTF16ToNSString(headerString)];
  [headerLabel setFont:[NSFont systemFontOfSize:16.0
                                         weight:NSFontWeightSemibold]];
  [headerLabel sizeToFit];
  int defaultHeight = NSHeight(headerLabel.frame);
  int numOfLines =
      static_cast<int>(NSWidth(headerLabel.frame)) / kContentsWidth + 1;
  [headerLabel
      setFrame:NSMakeRect(0, 0, kContentsWidth, defaultHeight * numOfLines)];

  std::u16string contentsString = brave_l10n::GetLocalizedResourceUTF16String(
      IDS_FIRSTRUN_DLG_CONTENTS_TEXT);
  base::i18n::AdjustStringForLocaleDirection(&contentsString);
  NSTextField* contentsLabel =
      [TextFieldUtils labelWithString:base::SysUTF16ToNSString(contentsString)];
  [contentsLabel setFont:[NSFont systemFontOfSize:15.0
                                           weight:NSFontWeightRegular]];
  [contentsLabel sizeToFit];
  defaultHeight = NSHeight(contentsLabel.frame);
  numOfLines =
      static_cast<int>(NSWidth(contentsLabel.frame)) / kContentsWidth + 1;
  [contentsLabel
      setFrame:NSMakeRect(0, 0, kContentsWidth, defaultHeight * numOfLines)];

  NSButton* maybeLaterButton =
      [ButtonUtils buttonWithTitle:l10n_util::GetNSString(
                                       IDS_FIRSTRUN_DLG_CANCEL_BUTTON_LABEL)
                            action:@selector(cancel:)
                            target:self];
  [maybeLaterButton setKeyEquivalent:kKeyEquivalentEscape];
  [maybeLaterButton sizeToFit];

  NSButton* makeDefaultButton = [ButtonUtils
      buttonWithTitle:l10n_util::GetNSString(IDS_FIRSTRUN_DLG_OK_BUTTON_LABEL)
               action:@selector(ok:)
               target:self];
  [makeDefaultButton setKeyEquivalent:kKeyEquivalentReturn];
  [makeDefaultButton sizeToFit];

  // It's time to calculate window's height as we can get all controls' final
  // heights.
  const int windowHeight = kTopPadding + NSHeight(headerLabel.frame) +
                           kLabelSpacing + NSHeight(contentsLabel.frame) +
                           kButtonTopMargin + NSHeight(maybeLaterButton.frame) +
                           kButtonBottomMargin;

  BOOL isDarkMode = NO;
  if (@available(macOS 10.14, *)) {
    NSAppearanceName appearance =
        [[NSApp effectiveAppearance] bestMatchFromAppearancesWithNames:@[
          NSAppearanceNameAqua, NSAppearanceNameDarkAqua
        ]];
    isDarkMode = [appearance isEqual:NSAppearanceNameDarkAqua];
  }
  NSColor* backgroundColor = isDarkMode
                                 ? [NSColor colorWithCalibratedRed:0x32 / 255.0
                                                             green:0x36 / 255.0
                                                              blue:0x39 / 255.0
                                                             alpha:1.0]
                                 : [NSColor whiteColor];

  base::scoped_nsobject<NSView> content_view([[NSView alloc]
      initWithFrame:NSMakeRect(0, 0, kDialogWidth, windowHeight)]);
  self.view = content_view.get();
  [self.view setValue:backgroundColor forKey:@"backgroundColor"];
  [self.view addSubview:headerLabel];
  [self.view addSubview:contentsLabel];
  [self.view addSubview:maybeLaterButton];
  [self.view addSubview:makeDefaultButton];

  // Set each control's position
  NSRect frame = headerLabel.frame;
  frame.origin.x = kPadding;
  frame.origin.y = windowHeight - kTopPadding - NSHeight(frame);
  [headerLabel setFrame:frame];

  frame = contentsLabel.frame;
  frame.origin.x = kPadding;
  frame.origin.y = NSMinY(headerLabel.frame) - kLabelSpacing - NSHeight(frame);
  [contentsLabel setFrame:frame];

  frame = makeDefaultButton.frame;
  frame.origin.x = kDialogWidth - kButtonHorizontalMargin - NSWidth(frame);
  if (base::i18n::IsRTL()) {
    frame.origin.x = kButtonHorizontalMargin;
  }
  frame.origin.y =
      NSMinY(contentsLabel.frame) - kButtonTopMargin - NSHeight(frame);
  [makeDefaultButton setFrame:frame];

  constexpr int kButtonSpacing = 3;
  frame = maybeLaterButton.frame;
  frame.origin.x =
      NSMinX(makeDefaultButton.frame) - kButtonSpacing - NSWidth(frame);
  if (base::i18n::IsRTL()) {
    frame.origin.x = NSMaxX(makeDefaultButton.frame) + kButtonSpacing;
  }
  frame.origin.y = NSMinY(makeDefaultButton.frame);
  [maybeLaterButton setFrame:frame];
}

- (NSString*)windowTitle {
  return l10n_util::GetNSString(IDS_FIRST_RUN_DIALOG_WINDOW_TITLE);
}

- (BOOL)isStatsReportingEnabled {
  // Give default value because we don't provide checkbox for this option.
  return GetDefaultPrefValueForMetricsReporting();
}

- (BOOL)isMakeDefaultBrowserEnabled {
  return _setAsDefaultBrowser;
}

- (void)ok:(id)sender {
  _setAsDefaultBrowser = YES;
  [[[self view] window] close];
  [NSApp stopModal];
}

- (void)cancel:(id)sender {
  [[[self view] window] close];
  [NSApp stopModal];
}

@end
