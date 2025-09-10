/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.Nullable;
import androidx.annotation.Size;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;
import androidx.preference.Preference;

import org.chromium.brave.browser.customize_menu.settings.BraveCustomizeMenuPreferenceFragment;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.ui.modelutil.MVCListAdapter;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;

/**
 * A Brave menu to handle customization logic for all those items that can be hidden from Settings >
 * Appearance > Customize menu. See {@link BraveCustomizeMenuPreferenceFragment} to inspect the
 * settings screen where a user can toggle the visibility of supported items.
 */
@NullMarked
public class CustomizeBraveMenu {

    public static final String KEY_MAIN_MENU_ITEM_LIST =
            "org.chromium.brave.browser.customize_menu.KEY_MAIN_MENU_ITEM_LIST";
    public static final String KEY_PAGE_ACTION_ITEM_LIST =
            "org.chromium.brave.browser.customize_menu.KEY_PAGE_ACTION_ITEM_LIST";
    @IdRes public static final int BRAVE_CUSTOMIZE_ITEM_ID = R.id.brave_customize_menu_id;

    @ColorRes
    public static final int DEFAULT_COLOR_RES_MENU_ITEM_ICON =
            R.color.default_icon_color_secondary_tint_list;

    public static final int PREFERENCE_MENU_ICON_SIZE_DP = 24;

    public static void applyCustomization(final MVCListAdapter.ModelList modelList) {
        for (Iterator<MVCListAdapter.ListItem> it = modelList.iterator(); it.hasNext(); ) {
            MVCListAdapter.ListItem item = it.next();
            final int menuItemId = item.model.get(AppMenuItemProperties.MENU_ITEM_ID);
            // Skip current item if it matches the customize menu id or settings menu id.
            if (BRAVE_CUSTOMIZE_ITEM_ID == menuItemId || R.id.preferences_id == menuItemId) {
                continue;
            }
            if (!isVisible(menuItemId)) {
                it.remove();
            }
        }

        // Removes adjacent separators, if any.
        sanitizeSeparators(modelList);
    }

    /**
     * Sanitizes a menu model by normalizing separator items (type {@code DIVIDER}).
     *
     * <p>Specifically, this method:
     *
     * <ul>
     *   <li>Removes any leading separators.
     *   <li>Removes any trailing separators.
     *   <li>Collapses runs of adjacent separators to a single separator.
     * </ul>
     *
     * <p>The operation modifies {@code modelList} in place and preserves the relative order of
     * non-separator items.
     *
     * @param modelList the mutable list of menu items to sanitize; must not be {@code null}
     * @see AppMenuHandler.AppMenuItemType#DIVIDER
     */
    public static void sanitizeSeparators(final MVCListAdapter.ModelList modelList) {
        if (modelList.isEmpty()) {
            return;
        }
        final MVCListAdapter.ModelList out = new MVCListAdapter.ModelList();

        // Mark when one or more separators are met.
        boolean pendingSeparators = false;
        MVCListAdapter.ListItem separator = null;

        for (Iterator<MVCListAdapter.ListItem> it = modelList.iterator(); it.hasNext(); ) {
            MVCListAdapter.ListItem curr = it.next();
            if (curr.type == AppMenuHandler.AppMenuItemType.DIVIDER) {
                // Just mark a separator was met, act when a standard item arrives.
                pendingSeparators = true;
                separator = curr;
                continue;
            }

            if (!out.isEmpty() && pendingSeparators) {
                // Add a single separator between standard menu items.
                out.add(separator);
            }
            out.add(curr);
            // Reset after placing standard menu item.
            pendingSeparators = false;
        }

        modelList.clear();
        modelList.addAll(out);
    }

    public static void openCustomizeMenuSettings(
            final Context context,
            final MVCListAdapter.ModelList menuItems,
            final MVCListAdapter.ModelList pageActions) {
        final Bundle bundle = new Bundle();
        populateBundle(bundle, menuItems, pageActions);

        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(context, BraveCustomizeMenuPreferenceFragment.class, bundle);
    }

    public static Bundle populateBundle(
            final Bundle bundle,
            final MVCListAdapter.ModelList menuItems,
            final MVCListAdapter.ModelList pageActions) {

        // Convert menu items to parcelable data.
        final ArrayList<MenuItemData> menuItemDataList = new ArrayList<>();
        populateMenuItemData(menuItemDataList, menuItems);

        // Convert page action items to parcelable data.
        final ArrayList<MenuItemData> pageActionDataList = new ArrayList<>();
        populateMenuItemData(pageActionDataList, pageActions);

        // Add data list to fragment bundle.
        bundle.putParcelableArrayList(KEY_MAIN_MENU_ITEM_LIST, menuItemDataList);
        bundle.putParcelableArrayList(KEY_PAGE_ACTION_ITEM_LIST, pageActionDataList);
        return bundle;
    }

    private static void populateMenuItemData(
            final List<MenuItemData> menuItemDataList, final MVCListAdapter.ModelList menuItems) {
        for (int i = 0; i < menuItems.size(); i++) {
            MVCListAdapter.ListItem item = menuItems.get(i);

            if (item.type == AppMenuHandler.AppMenuItemType.DIVIDER) {
                continue;
            }
            int id = item.model.get(AppMenuItemProperties.MENU_ITEM_ID);
            CharSequence title = item.model.get(AppMenuItemProperties.TITLE);
            menuItemDataList.add(new MenuItemData(id, title.toString(), isVisible(id)));
        }
    }

    public static boolean isVisible(final int itemId) {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        String.format(
                                Locale.ENGLISH, CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT, itemId),
                        true);
    }

    @DrawableRes
    public static int getDrawableResFromMenuItemId(@IdRes final int menuItemId) {
        // Resource IDs will be non-final by default in next
        // Gradle Plugin version, so we'll avoid using switch/case statements.
        if (menuItemId == R.id.new_tab_menu_id) {
            return R.drawable.ic_new_tab_page;
        } else if (menuItemId == R.id.new_incognito_tab_menu_id) {
            return R.drawable.brave_menu_new_private_tab;
        } else if (menuItemId == R.id.add_to_group_menu_id) {
            return R.drawable.ic_widgets;
        } else if (menuItemId == R.id.pin_tab_menu_id) {
            return R.drawable.ic_keep_24dp;
        } else if (menuItemId == R.id.new_window_menu_id) {
            return R.drawable.ic_new_window;
        } else if (menuItemId == R.id.new_incognito_window_menu_id) {
            return R.drawable.ic_incognito;
        } else if (menuItemId == R.id.move_to_other_window_menu_id) {
            return R.drawable.ic_open_in_browser;
        } else if (menuItemId == R.id.manage_all_windows_menu_id) {
            return R.drawable.ic_select_window;
        } else if (menuItemId == R.id.open_history_menu_id) {
            return R.drawable.brave_menu_history;
        } else if (menuItemId == R.id.tinker_tank_menu_id) {
            return R.drawable.ic_add_box_rounded_corner;
        } else if (menuItemId == R.id.downloads_menu_id) {
            return R.drawable.brave_menu_downloads;
        } else if (menuItemId == R.id.all_bookmarks_menu_id) {
            return R.drawable.brave_menu_bookmarks;
        } else if (menuItemId == R.id.recent_tabs_menu_id) {
            return R.drawable.brave_menu_recent_tabs;
        } else if (menuItemId == R.id.brave_wallet_id) {
            return R.drawable.ic_crypto_wallets;
        } else if (menuItemId == R.id.brave_playlist_id) {
            return R.drawable.ic_open_playlist;
        } else if (menuItemId == R.id.add_to_playlist_id) {
            return R.drawable.ic_baseline_add_24;
        } else if (menuItemId == R.id.brave_news_id) {
            return R.drawable.ic_news;
        } else if (menuItemId == R.id.brave_speedreader_id) {
            return R.drawable.ic_readermode;
        } else if (menuItemId == R.id.brave_leo_id) {
            return R.drawable.ic_brave_ai;
        } else if (menuItemId == R.id.request_brave_vpn_id) {
            return R.drawable.ic_vpn;
        } else if (menuItemId == R.id.brave_rewards_id) {
            return R.drawable.brave_menu_rewards;
        } else if (menuItemId == R.id.set_default_browser) {
            return R.drawable.brave_menu_set_as_default;
        } else if (menuItemId == R.id.exit_id) {
            return R.drawable.brave_menu_exit;
        } else if (menuItemId == R.id.page_zoom_id) {
            return R.drawable.ic_zoom;
        } else if (menuItemId == R.id.share_menu_id) {
            return R.drawable.ic_share_white_24dp;
        } else if (menuItemId == R.id.download_page_id) {
            return R.drawable.ic_download;
        } else if (menuItemId == R.id.print_id) {
            return R.drawable.sharing_print;
        } else if (menuItemId == R.id.enable_price_tracking_menu_id) {
            return R.drawable.price_tracking_disabled;
        } else if (menuItemId == R.id.disable_price_tracking_menu_id) {
            return R.drawable.price_tracking_enabled_filled;
        } else if (menuItemId == R.id.ai_web_menu_id) {
            return R.drawable.summarize_auto;
        } else if (menuItemId == R.id.find_in_page_id) {
            return R.drawable.ic_find_in_page;
        } else if (menuItemId == R.id.translate_id) {
            return R.drawable.ic_translate;
        } else if (menuItemId == R.id.readaloud_menu_id) {
            return R.drawable.ic_play_circle;
        } else if (menuItemId == R.id.reader_mode_menu_id) {
            return R.drawable.ic_reader_mode_24dp;
        } else if (menuItemId == R.id.open_with_id) {
            return R.drawable.ic_open_in_new;
        } else if (menuItemId == R.id.open_webapk_id) {
            return R.drawable.ic_open_webapk;
        } else if (menuItemId == R.id.universal_install) {
            return R.drawable.ic_add_to_home_screen;
        } else if (menuItemId == R.id.reader_mode_prefs_id) {
            return R.drawable.reader_mode_prefs_icon;
        } else if (menuItemId == R.id.auto_dark_web_contents_id) {
            return R.drawable.ic_brightness_medium_24dp;
        } else if (menuItemId == R.id.paint_preview_show_id) {
            return R.drawable.ic_photo_camera;
        } else if (menuItemId == R.id.get_image_descriptions_id) {
            return R.drawable.ic_image_descriptions;
        } else if (menuItemId == R.id.listen_to_feed_id) {
            return R.drawable.ic_play_circle;
        }

        assert true : "Unexpected value for menu item ID: " + menuItemId;
        return 0;
    }

    @Nullable
    public static Drawable getStandardizedMenuIcon(
            final Context context,
            @DrawableRes final int drawableRes,
            @Size(min = 1) final int iconSizePx) {
        if (drawableRes == 0) {
            return null;
        }
        Drawable src = ContextCompat.getDrawable(context, drawableRes);
        if (src == null) {
            return null;
        }

        // Work on a mutated instance to avoid shared state issues.
        Drawable drawable = src.mutate();

        final int width = drawable.getIntrinsicWidth();
        final int height = drawable.getIntrinsicHeight();

        if (width < 1 || height < 1) {
            return null;
        }

        // If required, standardize icon size to match menu icons.
        if (width < iconSizePx || height < iconSizePx) {
            Bitmap bitmap = Bitmap.createBitmap(iconSizePx, iconSizePx, Bitmap.Config.ARGB_8888);
            bitmap.setDensity(context.getResources().getDisplayMetrics().densityDpi);
            Canvas canvas = new Canvas(bitmap);
            drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
            drawable.draw(canvas);
            drawable = new BitmapDrawable(context.getResources(), bitmap);
        }

        final ColorStateList tintList =
                AppCompatResources.getColorStateList(context, DEFAULT_COLOR_RES_MENU_ITEM_ICON);
        if (tintList != null) {
            drawable.setTintList(tintList);
        }
        return drawable;
    }

    /**
     * Propagates custom menu related extras from a parent settings screen into the given {@link
     * ChromeBasePreference} so that the payload is carried to the next fragment when that
     * preference is clicked.
     *
     * @param preference The child {@link ChromeBasePreference} that will navigate to the next
     *     screen (e.g., “Appearance”). Its {@link Preference#getExtras()} will be augmented if
     *     non-null.
     * @param bundle Source of extras to forward (typically the current fragment’s {@link
     *     androidx.fragment.app.Fragment#getArguments()}); may be null.
     */
    public static void propagateMenuItemExtras(
            @Nullable final ChromeBasePreference preference, @Nullable final Bundle bundle) {
        if (preference == null || bundle == null) {
            return;
        }
        // Copy over the custom menu item list (main section and page actions)
        // from the main settings screen to the appearance screen.
        if (bundle.containsKey(KEY_MAIN_MENU_ITEM_LIST)) {
            preference
                    .getExtras()
                    .putParcelableArrayList(
                            KEY_MAIN_MENU_ITEM_LIST,
                            bundle.getParcelableArrayList(KEY_MAIN_MENU_ITEM_LIST));
        }

        if (bundle.containsKey(KEY_PAGE_ACTION_ITEM_LIST)) {
            preference
                    .getExtras()
                    .putParcelableArrayList(
                            KEY_PAGE_ACTION_ITEM_LIST,
                            bundle.getParcelableArrayList(KEY_PAGE_ACTION_ITEM_LIST));
        }
    }
}
