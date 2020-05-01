/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

function braveFlagsUIOverrides() {
  $('channel-promo-beta').innerHTML = `
      Interested in cool new Brave features? Try our
      <a href="https://brave.com/download-beta/">beta channel</a>.
  `
  $('channel-promo-dev').innerHTML = `
      Interested in cool new Brave features? Try our
      <a href="https://brave.com/download-beta/">dev channel</a>.
  `
}

document.addEventListener('DOMContentLoaded', function() {
  braveFlagsUIOverrides();
});
