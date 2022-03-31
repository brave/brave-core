/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/cocoa/first_run_dialog_controller.h"

#include "base/i18n/rtl.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "ui/base/cocoa/controls/button_utils.h"
#include "ui/base/cocoa/controls/textfield_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace {

// Return the internationalized message |message_id|, with the product name
// substituted in for $1.
NSString* NSStringWithProductName(int message_id) {
  return l10n_util::GetNSStringF(message_id,
                                 l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
}

// Reflows buttons. Requires them to be passed in vertical order, top down.
CGFloat VerticallyReflowButtons(NSArray<NSButton*>* buttons) {
  CGFloat localVerticalShift = 0;
  for (NSButton* button in buttons) {
    [GTMUILocalizerAndLayoutTweaker wrapButtonTitleForWidth:button];

    NSRect oldFrame = button.frame;
    [button sizeToFit];
    NSRect newFrame = button.frame;
    // -[NSControl sizeToFit], with no layout constraints, like here, will end
    // up horizontally resizing and keeping the upper left corner in place.
    // That wrecks RTL, and because all that's really needed is vertical
    // resizing, reset the horizontal size.
    newFrame.size.width = NSWidth(oldFrame);
    button.frame = newFrame;

    localVerticalShift += NSHeight(newFrame) - NSHeight(oldFrame);
    if (localVerticalShift) {
      NSPoint origin = button.frame.origin;
      origin.y -= localVerticalShift;
      [button setFrameOrigin:origin];
    }
  }
  return localVerticalShift;
}

void MoveViewsVertically(NSArray* views, CGFloat distance) {
  for (NSView* view : views) {
    NSRect frame = view.frame;
    frame.origin.y += distance;
    [view setFrame:frame];
  }
}

// Center |view| vertically within its own superview, so its horizontal
// centerline is the same as its superview's horizontal centerline.
void CenterVertically(NSView* view) {
  NSView* superview = view.superview;
  NSRect frame = view.frame;
  NSRect superframe = superview.frame;
  frame.origin.y = (NSHeight(superframe) - NSHeight(frame)) / 2.0;
  [view setFrame:frame];
}

}  // namespace

@implementation FirstRunDialogViewController {
  // These are owned by the NSView hierarchy:
  NSButton* _defaultBrowserCheckbox;
}

- (instancetype)initWithStatsCheckboxInitiallyChecked:(BOOL)checked
                        defaultBrowserCheckboxVisible:(BOOL)visible {
  // We always make default checkbox visible.
  DCHECK(visible);
  return [super init];
}

- (void)loadView {
  const int kDialogWidth = 480;

  BOOL isDarkMode = NO;
  if (@available(macOS 10.14, *)) {
    NSAppearanceName appearance =
        [[NSApp effectiveAppearance] bestMatchFromAppearancesWithNames:@[
          NSAppearanceNameAqua, NSAppearanceNameDarkAqua
        ]];
    isDarkMode = [appearance isEqual:NSAppearanceNameDarkAqua];
  }
  NSColor* topBoxColor = isDarkMode
                             ? [NSColor colorWithCalibratedRed:0x32 / 255.0
                                                         green:0x36 / 255.0
                                                          blue:0x39 / 255.0
                                                         alpha:1.0]
                             : [NSColor whiteColor];

  NSBox* topBox = [[[NSBox alloc]
      initWithFrame:NSMakeRect(0, 110, kDialogWidth, 90)] autorelease];
  [topBox setFillColor:topBoxColor];
  [topBox setBoxType:NSBoxCustom];
  [topBox setBorderType:NSNoBorder];
  [topBox setContentViewMargins:NSZeroSize];

  // This string starts with the app name, which is strongly LTR, so force the
  // correct layout.
  std::u16string completeInstallationString = l10n_util::GetStringUTF16(
      IDS_FIRSTRUN_DLG_COMPLETE_INSTALLATION_LABEL_BRAVE);
  base::i18n::AdjustStringForLocaleDirection(&completeInstallationString);
  NSTextField* completionLabel = [TextFieldUtils
      labelWithString:base::SysUTF16ToNSString(completeInstallationString)];
  [completionLabel setFrame:NSMakeRect(13, 25, kDialogWidth - 2 * 13, 60)];

  _defaultBrowserCheckbox = [ButtonUtils
      checkboxWithTitle:l10n_util::GetNSString(
                            IDS_FR_CUSTOMIZE_DEFAULT_BROWSER_BRAVE)];
  [_defaultBrowserCheckbox
      setFrame:NSMakeRect(45, 80, kDialogWidth - 2 * 45, 18)];
  [_defaultBrowserCheckbox setState:NSOnState];

  NSButton* startChromeButton =
      [ButtonUtils buttonWithTitle:NSStringWithProductName(
                                       IDS_FIRSTRUN_DLG_MAC_START_CHROME_BUTTON)
                            action:@selector(ok:)
                            target:self];
  [startChromeButton setFrame:NSMakeRect(161, 12, 306, 32)];
  [startChromeButton setKeyEquivalent:kKeyEquivalentReturn];

  NSBox* topSeparator = [[[NSBox alloc]
      initWithFrame:NSMakeRect(0, 109, kDialogWidth, 1)] autorelease];
  [topSeparator setBoxType:NSBoxSeparator];

  NSBox* bottomSeparator = [[[NSBox alloc]
      initWithFrame:NSMakeRect(0, 55, kDialogWidth, 5)] autorelease];
  [bottomSeparator setBoxType:NSBoxSeparator];

  [topBox addSubview:completionLabel];
  CenterVertically(completionLabel);

  base::scoped_nsobject<NSView> content_view(
      [[NSView alloc] initWithFrame:NSMakeRect(0, 0, kDialogWidth, 200)]);
  self.view = content_view.get();
  [self.view addSubview:topBox];
  [self.view addSubview:topSeparator];
  [self.view addSubview:_defaultBrowserCheckbox];
  [self.view addSubview:bottomSeparator];
  [self.view addSubview:startChromeButton];

  // Now that the content view is constructed, fix the layout. The first step is
  // to reflow the browser and stats checkbox texts, which can be quite lengthy
  // in some locales. They may wrap onto additional lines, and in doing so cause
  // the rest of the dialog to need to be rearranged.
  {
    CGFloat delta = VerticallyReflowButtons(@[ _defaultBrowserCheckbox ]);
    if (delta) {
      // If reflowing the checkboxes produced a height delta, move the
      // checkboxes and the items above them in the content view upward, then
      // grow the content view to match. This has the effect of moving
      // everything visually-below the checkboxes downwards and expanding the
      // window, leaving the vertical space the checkboxes need for their text.
      MoveViewsVertically(@[ _defaultBrowserCheckbox, topSeparator, topBox ],
                          delta);
      NSRect frame = [self.view frame];
      frame.size.height += delta;
      [self.view setAutoresizesSubviews:NO];
      [self.view setFrame:frame];
      [self.view setAutoresizesSubviews:YES];
    }
  }

  // The "Start Chrome" button needs to be sized to fit the localized string
  // inside it, but it should still be at the right-most edge of the dialog, so
  // any width added or subtracted by |sizeToFit| is added to its x coord, which
  // keeps its right edge where it was.
  CGFloat oldWidth = NSWidth([startChromeButton frame]);
  [startChromeButton sizeToFit];
  NSRect frame = [startChromeButton frame];
  frame.origin.x += oldWidth - NSWidth([startChromeButton frame]);
  if (base::i18n::IsRTL())
    frame.origin.x = kDialogWidth - NSMaxX(frame);
  [startChromeButton setFrame:frame];
}

- (NSString*)windowTitle {
  return l10n_util::GetNSString(IDS_FIRST_RUN_DIALOG_WINDOW_TITLE);
}

- (BOOL)isStatsReportingEnabled {
  // Give default value because we don't provide checkbox for this option.
  return GetDefaultPrefValueForMetricsReporting();
}

- (BOOL)isMakeDefaultBrowserEnabled {
  return [_defaultBrowserCheckbox state] == NSOnState;
}

- (void)ok:(id)sender {
  [[[self view] window] close];
  [NSApp stopModal];
}

@end
