/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_SELECTABLE_LIST_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_SELECTABLE_LIST_VIEW_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "brave/browser/ui/views/playlist/thumbnail_provider.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/box_layout_view.h"

namespace views {
class ImageView;
}  // namespace views

class ThumbnailView;

class SelectableView : public views::Button {
  METADATA_HEADER(SelectableView, views::Button)
 public:
  using OnPressedCallback = base::RepeatingCallback<void(SelectableView&)>;

  SelectableView(const std::string& id,
                 const std::string& name,
                 const gfx::Image& image,
                 OnPressedCallback on_pressed);
  ~SelectableView() override;

  bool selected() const { return selected_; }
  const std::string id() const { return id_; }

  void SetSelected(bool selected);

  base::OnceCallback<void(const gfx::Image&)> GetThumbnailSetter();

  // views::Button:
  void OnThemeChanged() override;

 private:
  SelectableView& OnPressed(const ui::Event& event);

  void UpdateBackground();

  std::string id_;
  std::string name_;
  gfx::Image image_;

  bool selected_ = false;

  raw_ptr<views::ImageView> selected_icon_ = nullptr;
  raw_ptr<ThumbnailView> thumbnail_view_ = nullptr;
};

template <typename T>
struct SelectableDataTraits {};

template <>
struct SelectableDataTraits<playlist::mojom::PlaylistItemPtr> {
  static const std::string& GetId(
      const playlist::mojom::PlaylistItemPtr& item) {
    return item->id;
  }

  static const std::string& GetName(
      const playlist::mojom::PlaylistItemPtr& item) {
    return item->name;
  }
};

template <>
struct SelectableDataTraits<playlist::mojom::PlaylistPtr> {
  static const std::string& GetId(
      const playlist::mojom::PlaylistPtr& playlist) {
    return *playlist->id;
  }

  static std::string GetName(const playlist::mojom::PlaylistPtr& playlist) {
    if (playlist->id == playlist::kDefaultPlaylistID) {
      return l10n_util::GetStringUTF8(IDS_PLAYLIST_DEFAULT_PLAYLIST_NAME);
    }

    return playlist->name;
  }
};

template <class DataType,
          bool multi_selectable = true,
          bool need_at_least_one_selected = false>
class SelectableListView : public views::BoxLayoutView {
 public:
  SelectableListView(ThumbnailProvider* thumbnail_provider,
                     const std::vector<DataType>& data,
                     base::RepeatingCallback<void()> on_selection_changed)
      : thumbnail_provider_(thumbnail_provider),
        on_selection_changed_(on_selection_changed) {
    CHECK(thumbnail_provider_);

    SetOrientation(views::BoxLayout::Orientation::kVertical);

    for (const auto& d : data) {
      auto id = SelectableDataTraits<DataType>::GetId(d);
      data_.insert({id, d.Clone()});

      auto* selectable_view = AddChildView(std::make_unique<SelectableView>(
          id, SelectableDataTraits<DataType>::GetName(d), gfx::Image(),
          base::BindRepeating(&SelectableListView::OnViewPressed,
                              base::Unretained(this))));
      thumbnail_provider->GetThumbnail(d,
                                       selectable_view->GetThumbnailSetter());
      child_views_.insert({id, selectable_view});
    }
  }
  ~SelectableListView() override = default;

  void SetSelected(const std::vector<DataType>& data) {
    std::vector<std::string> ids;
    base::ranges::transform(
        data, std::back_inserter(ids), [](const auto& data) {
          return SelectableDataTraits<DataType>::GetId(data);
        });
    SetSelected(ids);
  }

  void SetSelected(const std::vector<std::string>& ids) {
    if constexpr (!multi_selectable) {
      DCHECK_LE(ids.size(), 1u);
    } else if (need_at_least_one_selected) {
      DCHECK_EQ(ids.size(), 1u);
    }

    for (const auto& [id, view] : child_views_) {
      selected_views_.clear();
      view->SetSelected(false);
    }

    for (const auto& id : ids) {
      auto* view = child_views_.at(id);
      selected_views_.insert({id, view});
      view->SetSelected(true);
    }
  }

  std::vector<DataType> GetSelected() {
    std::vector<DataType> items;
    base::ranges::transform(
        selected_views_, std::back_inserter(items),
        [this](const auto& id_and_view) {
          return data_.at(id_and_view.second->id())->Clone();
        });
    return items;
  }

  bool HasSelected() const { return selected_views_.size(); }

 private:
  void OnViewPressed(SelectableView& view) {
    if constexpr (multi_selectable) {
      if (view.selected()) {
        selected_views_.insert({view.id(), &view});
      } else {
        selected_views_.erase(view.id());
      }
    } else {
      if constexpr (need_at_least_one_selected) {
        if (!view.selected()) {
          view.SetSelected(true);
          return;
        }
      }

      if (selected_views_.size()) {
        DCHECK_EQ(selected_views_.size(), 1u);
        (*selected_views_.begin()).second->SetSelected(false);
        selected_views_.clear();
      }

      selected_views_.insert({view.id(), &view});
    }

    on_selection_changed_.Run();
  }

  raw_ptr<ThumbnailProvider, DanglingUntriaged> thumbnail_provider_;

  base::RepeatingCallback<void()> on_selection_changed_;

  base::flat_map<std::string /*id*/, DataType> data_;
  base::flat_map<std::string /*id*/, SelectableView*> child_views_;
  base::flat_map<std::string /*id*/, SelectableView*> selected_views_;
};

using SelectableItemsView =
    SelectableListView<playlist::mojom::PlaylistItemPtr>;
using SelectablePlaylistsView =
    SelectableListView<playlist::mojom::PlaylistPtr,
                       /*multi_selectable=*/false,
                       /*need_at_least_one_selected=*/true>;

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_SELECTABLE_LIST_VIEW_H_
