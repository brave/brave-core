/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Instruction template the user copies into Leo so its output conforms to the
// custom widget runtime. The allow-listed hosts MUST match the connect-src CSP
// in custom_widget_untrusted_ui.cc.
export const customWidgetPrompt = `You are creating a custom widget for the Brave browser New Tab Page.

Reply with a SINGLE self-contained HTML document inside one \`\`\`html code block, and nothing else (no explanation before or after).

Rules:
- One HTML document with inline <style> and inline <script>. Do NOT use external scripts, stylesheets, fonts, images-by-URL frameworks, or any <link>/CDN references. Everything must be inline.
- It renders inside a small dark card about 300px wide and 200px tall. Use a transparent background, light-colored text, system-ui font, and compact sizing. Make it fit without scrollbars.
- You may fetch live data ONLY from these exact hosts (all public, no API key, CORS-enabled). Any other network request is blocked:
  - https://api.open-meteo.com and https://geocoding-api.open-meteo.com (weather + city geocoding)
  - https://api.coingecko.com (crypto prices)
  - https://api.frankfurter.app (currency exchange rates)
  - https://hacker-news.firebaseio.com (Hacker News stories)
  If the request can't be satisfied with these, build a self-contained widget with no network calls instead (for example a clock, countdown timer, or calculator).
- Show a brief loading state and a short error message if a fetch fails.
- Do not use cookies, localStorage, popups, alerts, or navigation. Keep it safe and self-contained.

The widget I want: `
