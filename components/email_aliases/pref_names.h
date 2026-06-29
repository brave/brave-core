/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_PREF_NAMES_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_PREF_NAMES_H_

namespace email_aliases::prefs {

inline constexpr char kEmailAliasesEnabled[] = "brave.email_aliases.enabled";

// Preference key for storing the aliases notes.
inline constexpr char kEmailAliasesNotes[] = "brave.email_aliases.notes";

// Set to true once the user has at least one alias.
inline constexpr char kAliasesPresent[] = "brave.email_aliases.aliases_present";

// Weekly storage list for clipboard copy count metric.
inline constexpr char kClipboardCopyCountStorage[] =
    "brave.email_aliases.clipboard_copy_count_storage";

// Set to true once the settings page access method metric has been recorded.
inline constexpr char kSettingsPageMethodReported[] =
    "brave.email_aliases.settings_page_method_reported";

inline constexpr char kPromoShown[] = "brave.email_aliases.promo_shown";

}  // namespace email_aliases::prefs

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_PREF_NAMES_H_
