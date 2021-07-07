/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PAGE_TRANSITION_TYPES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PAGE_TRANSITION_TYPES_H_

#include <cstdint>

namespace ads {

// Types of transitions between pages.
//
// WARNING: don't change these numbers. They should match
// |ui/base/page_transition_types.h| and are provided by the variations service,
// so will need the same values to match the enums.
//
// A type is made of a core value and a set of qualifiers. A type has one core
// value and 0 or or more qualifiers.

enum PageTransitionType : uint32_t {
  // User got to this page by clicking a link on another page.
  kPageTransitionLink = 0,

  // User got this page by typing the URL in the URL bar.  This should not be
  // used for cases where the user selected a choice that didn't look at all
  // like a URL; see GENERATED below.
  //
  // We also use this for other "explicit" navigation actions.
  kPageTransitionTyped = 1,

  // User got to this page through a suggestion in the UI, for example)
  // through the destinations page.
  kPageTransitionAutoBookmark = 2,

  // User got to this page by typing in the URL bar and selecting an entry
  // that did not look like a URL.  For example, a match might have the URL
  // of a Google search result page, but appear like "Search Google for ...".
  // These are not quite the same as TYPED navigations because the user
  // didn't type or see the destination URL.
  // See also KEYWORD.
  kPageTransitionGenerated = 5,

  // The user filled out values in a form and submitted it. NOTE that in
  // some situations submitting a form does not result in this transition
  // type. This can happen if the form uses script to submit the contents.
  kPageTransitionFormSubmit = 7,

  // The user "reloaded" the page, either by hitting the reload button or by
  // hitting enter in the address bar.  NOTE: This is distinct from the
  // concept of whether a particular load uses "reload semantics" (i.e.
  // bypasses cached data).  For this reason, lots of code needs to pass
  // around the concept of whether a load should be treated as a "reload"
  // separately from their tracking of this transition type, which is mainly
  // used for proper scoring for consumers who care about how frequently a
  // user typed/visited a particular URL.
  //
  // SessionRestore and undo tab close use this transition type too.
  kPageTransitionReload = 8,

  // The url was generated from a replaceable keyword other than the default
  // search provider. If the user types a keyword (which also applies to
  // tab-to-search) in the omnibox this qualifier is applied to the transition
  // type of the generated url. TemplateURLModel then may generate an
  // additional visit with a transition type of KEYWORD_GENERATED against the
  // url 'http://' + keyword. For example, if you do a tab-to-search against
  // wikipedia the generated url has a transition qualifer of KEYWORD, and
  // TemplateURLModel generates a visit for 'wikipedia.org' with a transition
  // type of KEYWORD_GENERATED.
  kPageTransitionKeyword = 9,

  // Corresponds to a visit generated for a keyword. See description of
  // KEYWORD for more details.
  kPageTransitionKeywordGenerated = 10,

  // ADDING NEW CORE VALUE? Be sure to update the LAST_CORE and CORE_MASK
  // values below.  Also update CoreTransitionString().
  kPageTransitionLastCore = kPageTransitionKeywordGenerated,
  kPageTransitionCoreMask = 0xFF,

  // Qualifiers
  // Any of the core values above can be augmented by one or more qualifiers.
  // These qualifiers further define the transition.

  // User used the Forward or Back button to navigate among browsing history.
  kPageTransitionForwardBack = 0x01000000,

  // User used the address bar to trigger this navigation.
  kPageTransitionFromAddressBar = 0x02000000,

  // User is navigating to the home page.
  kPageTransitionHomePage = 0x04000000,

  // The transition originated from an external application; the exact
  // definition of this is embedder dependent.
  kPageTransitionFromAPI = 0x08000000,

  // General mask defining the bits used for the qualifiers.
  kPageTransitionQualifierMask = 0xFFFFFF00,
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PAGE_TRANSITION_TYPES_H_
