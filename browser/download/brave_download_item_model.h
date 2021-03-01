/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_ITEM_MODEL_H_
#define BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_ITEM_MODEL_H_

#include "chrome/browser/download/download_item_model.h"

// The purpose of this class is to extend DownloadItemModel's class
// functionality by adding a method for the origin URL text and
// a method that returns tool tip text that includes origin URL.
// This class, however, doesn't inherit from DownloadItemModel because
// DownloadItemView has a member of DownloadItemModel type. To use this
// extended functionality, BraveDownloadItemView will have this model as
// a member.
class BraveDownloadItemModel {
 public:
  // Constructs a BraveDownloadItemModel that encapsulates DownloadItemModel.
  explicit BraveDownloadItemModel(DownloadUIModel* model);
  ~BraveDownloadItemModel();

  // Method that returns a string suitable for use as a tooltip. For
  // a regular download, the tooltip is the filename and the origin URL with an
  // indicator if the URL is secure. For an interrupted download, falls back on
  // the base class behavior.
  base::string16 GetTooltipText();

  // Returns a string suitable for use as the origin URL. |is_secure| is set to
  // true if the url is considered secure.
  base::string16 GetOriginURLText(bool* is_secure);

  // Encapsulated model.
  DownloadUIModel* model_;

  DISALLOW_COPY_AND_ASSIGN(BraveDownloadItemModel);
};

#endif  // BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_ITEM_MODEL_H_
