/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_TAB_BRAVE_PARENT_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_TAB_BRAVE_PARENT_H_


class BraveTabParent {
public:
  virtual ~BraveTabParent(){}

  // The combined width of the curves at the top and bottom of the endcap.
  static constexpr float kMinimumEndcapWidth = 2;


  // Returns the inverse of the slope of the diagonal portion of the tab outer
  // border.  (This is a positive value, so it's specifically for the slope of
  // the leading edge.)
  //
  // This returns the inverse (dx/dy instead of dy/dx) because we use exact
  // values for the vertical distances between points and then compute the
  // horizontal deltas from those.
  //
  // This is zero for rectangle shaped tab, because dx is 0
  static float GetInverseDiagonalSlope();

  static float GetTabEndcapWidth();
};


#endif //BRAVE_BROWSER_UI_VIEWS_TABS_TAB_BRAVE_PARENT_H_
