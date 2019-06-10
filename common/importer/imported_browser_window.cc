/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/imported_browser_window.h"

ImportedBrowserTab::ImportedBrowserTab() {}
ImportedBrowserTab::ImportedBrowserTab(const ImportedBrowserTab& other) =
    default;
ImportedBrowserTab::~ImportedBrowserTab() {}

ImportedBrowserWindow::ImportedBrowserWindow() {}
ImportedBrowserWindow::ImportedBrowserWindow(
    const ImportedBrowserWindow& other) = default;
ImportedBrowserWindow::~ImportedBrowserWindow() {}

ImportedWindowState::ImportedWindowState() {}
ImportedWindowState::ImportedWindowState(const ImportedWindowState& other) =
    default;
ImportedWindowState::~ImportedWindowState() {}
