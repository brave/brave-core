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
- CRITICAL — transparent backgrounds only: The widget sits on a glass/frosted card on the New Tab Page. Every element MUST use a fully transparent background. Set \`html, body { background: transparent; }\` and never use background-color, background, or gradients on html, body, or any container. Do not use solid fills, cards, panels, or boxes with opaque/semi-opaque backgrounds. Text and icons may be visible; backgrounds must not be.
- It renders inside a small dark card about 300px wide and 128px tall. Use light-colored text, system-ui font, and compact sizing. Make it fit without scrollbars. You may show rich, multi-line layouts (scores, tickers, stats) as long as they fit.
- You may fetch live data ONLY from these exact hosts (public, no API key, CORS-enabled). Any other network request is blocked:
  - https://api.open-meteo.com and https://geocoding-api.open-meteo.com — weather and city geocoding
  - https://api.coingecko.com — crypto prices (e.g. /api/v3/simple/price)
  - https://api.frankfurter.app — currency exchange rates
  - https://query1.finance.yahoo.com and https://query2.finance.yahoo.com — stock quotes and charts (e.g. /v8/finance/chart/AAPL, /v7/finance/quote?symbols=AAPL,MSFT,GOOGL)
  - https://site.api.espn.com — live sports scores and schedules (e.g. /apis/site/v2/sports/soccer/fifa.world/scoreboard for World Cup, /apis/site/v2/sports/soccer/usa.1/scoreboard for MLS, /apis/site/v2/sports/football/nfl/scoreboard for NFL)
  - https://www.thesportsdb.com — sports teams, events, and scores (free test key in path: /api/v1/json/3/...)
  - https://hacker-news.firebaseio.com — Hacker News stories
  For live scores, stock tickers, weather, etc., use setInterval to refresh every 30–60 seconds where useful.
  If the request can't be satisfied with these hosts, build a self-contained widget with no network calls instead (for example a clock, countdown timer, or calculator).
- Show a brief loading state and a short error message if a fetch fails.
- Do not use cookies, localStorage, popups, alerts, or navigation. Keep it safe and self-contained.

The widget I want: `
