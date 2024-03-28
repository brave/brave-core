/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/base/models/image_model.h"
#include "ui/base/models/menu_model.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_utils.h"

#define controllerWillAddItem controllerWillAddItem_Chromium

#include "src/ui/views/controls/menu/menu_controller_cocoa_delegate_impl.mm"

#undef controllerWillAddItem

@implementation MenuControllerCocoaDelegateImpl (Brave)

- (void)controllerWillAddItem:(NSMenuItem*)menuItem
                    fromModel:(ui::MenuModel*)model
                      atIndex:(size_t)index
            withColorProvider:(const ui::ColorProvider*)colorProvider {
  [self controllerWillAddItem_Chromium:menuItem
                             fromModel:model
                               atIndex:index
                     withColorProvider:colorProvider];

  if (!menuItem.image && model->GetIconAt(index).IsVectorIcon()) {
    auto icon = model->GetIconAt(index);
    auto vector_icon_model = icon.GetVectorIcon();
    auto icon_size =
        vector_icon_model.icon_size()
            ? vector_icon_model.icon_size()
            : gfx::GetDefaultSizeOfVectorIcon(*vector_icon_model.vector_icon());
    if (vector_icon_model.has_color()) {
      menuItem.image = gfx::Image(gfx::CreateVectorIcon(
                                      *vector_icon_model.vector_icon(),
                                      icon_size, vector_icon_model.color()))
                           .ToNSImage();
    } else {
      menuItem.image =
          gfx::Image(gfx::CreateVectorIcon(
                         *vector_icon_model.vector_icon(), icon_size,
                         colorProvider->GetColor(vector_icon_model.color_id())))
              .ToNSImage();
    }
  }
}

@end
