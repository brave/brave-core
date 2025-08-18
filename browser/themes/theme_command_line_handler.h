// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THEMES_THEME_COMMAND_LINE_HANDLER_H_
#define CHROME_BROWSER_THEMES_THEME_COMMAND_LINE_HANDLER_H_

namespace base {
class CommandLine;
}

class Profile;
class ThemeService;

class ThemeCommandLineHandler {
 public:
  static void ProcessThemeCommandLineSwitches(
      const base::CommandLine* command_line,
      ThemeService* theme_service);

  static void ProcessForProfile(const base::CommandLine* command_line,
                                Profile* profile);

  ThemeCommandLineHandler() = delete;
  ~ThemeCommandLineHandler() = delete;
};

#endif  // CHROME_BROWSER_THEMES_THEME_COMMAND_LINE_HANDLER_H_
