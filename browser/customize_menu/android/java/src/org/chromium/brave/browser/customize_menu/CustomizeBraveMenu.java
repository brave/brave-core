/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import static org.chromium.base.BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.SparseIntArray;

import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.Nullable;
import androidx.annotation.Size;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;
import androidx.preference.Preference;

import org.chromium.base.Callback;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
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
 * Handles the customization logic for Brave's custom menu item preference screen, allowing users to
 * show or hide menu items based on their preferences.
 *
 * <p>This class provides functionality to:
 *
 * <ul>
 *   <li>Apply user-defined visibility settings to menu items
 *   <li>Manage the navigation to customization settings
 *   <li>Standardize menu item icons for consistent appearance
 *   <li>Sanitize menu separators to prevent visual issues
 * </ul>
 *
 * <p>Users can customize which menu items are visible through Settings > Appearance > Customize
 * menu. The customization settings are persisted in shared preferences and applied whenever the
 * menu is displayed.
 *
 * <p><strong>Note:</strong> Two menu items cannot be hidden and will always remain visible:
 *
 * <ul>
 *   <li>The "Customize menu" item itself ({@link #BRAVE_CUSTOMIZE_ITEM_ID})
 *   <li>The main "Settings" item ({@code R.id.preferences_id})
 * </ul>
 *
 * <p>These items are essential for menu navigation and configuration access.
 *
 * @see BraveCustomizeMenuPreferenceFragment for the settings screen implementation.
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

    /**
     * Static mapping of menu item IDs to their corresponding drawable resource IDs. Uses
     * SparseIntArray for optimal performance and memory efficiency on Android when mapping resource
     * IDs (sparse integers) to drawable resource IDs.
     *
     * <p>This provides significantly better performance and memory usage compared to {@code
     * HashMap<Integer, Integer>} by avoiding autoboxing overhead and utilizing Android's optimized
     * sparse array implementation designed for resource ID mappings.
     */
    private static final SparseIntArray MENU_ICON_MAP = new SparseIntArray();

    static {
        // Main menu items
        MENU_ICON_MAP.put(R.id.new_tab_menu_id, R.drawable.ic_new_tab_page);
        MENU_ICON_MAP.put(R.id.new_incognito_tab_menu_id, R.drawable.brave_menu_new_private_tab);
        MENU_ICON_MAP.put(R.id.add_to_group_menu_id, R.drawable.ic_widgets);
        MENU_ICON_MAP.put(R.id.pin_tab_menu_id, R.drawable.ic_keep_24dp);
        MENU_ICON_MAP.put(R.id.new_window_menu_id, R.drawable.ic_new_window);
        MENU_ICON_MAP.put(R.id.new_incognito_window_menu_id, R.drawable.ic_incognito);
        MENU_ICON_MAP.put(R.id.move_to_other_window_menu_id, R.drawable.ic_open_in_browser);
        MENU_ICON_MAP.put(R.id.manage_all_windows_menu_id, R.drawable.ic_select_window);
        MENU_ICON_MAP.put(R.id.open_history_menu_id, R.drawable.brave_menu_history);
        MENU_ICON_MAP.put(R.id.tinker_tank_menu_id, R.drawable.ic_add_box_rounded_corner);
        MENU_ICON_MAP.put(R.id.downloads_menu_id, R.drawable.brave_menu_downloads);
        MENU_ICON_MAP.put(R.id.all_bookmarks_menu_id, R.drawable.brave_menu_bookmarks);
        MENU_ICON_MAP.put(R.id.recent_tabs_menu_id, R.drawable.brave_menu_recent_tabs);
        MENU_ICON_MAP.put(R.id.brave_wallet_id, R.drawable.ic_crypto_wallets);
        MENU_ICON_MAP.put(R.id.brave_playlist_id, R.drawable.ic_open_playlist);
        MENU_ICON_MAP.put(R.id.add_to_playlist_id, R.drawable.ic_baseline_add_24);
        MENU_ICON_MAP.put(R.id.brave_news_id, R.drawable.ic_news);
        MENU_ICON_MAP.put(R.id.brave_speedreader_id, R.drawable.ic_readermode);
        MENU_ICON_MAP.put(R.id.brave_leo_id, R.drawable.ic_brave_ai);
        MENU_ICON_MAP.put(R.id.request_brave_vpn_id, R.drawable.ic_vpn);
        MENU_ICON_MAP.put(R.id.brave_rewards_id, R.drawable.brave_menu_rewards);
        MENU_ICON_MAP.put(R.id.set_default_browser, R.drawable.brave_menu_set_as_default);
        MENU_ICON_MAP.put(R.id.exit_id, R.drawable.brave_menu_exit);

        // Page action items
        MENU_ICON_MAP.put(R.id.page_zoom_id, R.drawable.ic_zoom);
        MENU_ICON_MAP.put(R.id.share_menu_id, R.drawable.ic_share_white_24dp);
        MENU_ICON_MAP.put(R.id.download_page_id, R.drawable.ic_download);
        MENU_ICON_MAP.put(R.id.print_id, R.drawable.sharing_print);
        MENU_ICON_MAP.put(R.id.enable_price_tracking_menu_id, R.drawable.price_tracking_disabled);
        MENU_ICON_MAP.put(
                R.id.disable_price_tracking_menu_id, R.drawable.price_tracking_enabled_filled);
        MENU_ICON_MAP.put(R.id.ai_web_menu_id, R.drawable.summarize_auto);
        MENU_ICON_MAP.put(R.id.find_in_page_id, R.drawable.ic_find_in_page);
        MENU_ICON_MAP.put(R.id.translate_id, R.drawable.ic_translate);
        MENU_ICON_MAP.put(R.id.readaloud_menu_id, R.drawable.ic_play_circle);
        MENU_ICON_MAP.put(R.id.reader_mode_menu_id, R.drawable.ic_mobile_friendly_24dp);
        MENU_ICON_MAP.put(R.id.open_with_id, R.drawable.ic_open_in_new);
        MENU_ICON_MAP.put(R.id.open_webapk_id, R.drawable.ic_open_webapk);
        MENU_ICON_MAP.put(R.id.universal_install, R.drawable.ic_add_to_home_screen);
        MENU_ICON_MAP.put(R.id.reader_mode_prefs_id, R.drawable.reader_mode_prefs_icon);
        MENU_ICON_MAP.put(R.id.auto_dark_web_contents_id, R.drawable.ic_brightness_medium_24dp);
        MENU_ICON_MAP.put(R.id.paint_preview_show_id, R.drawable.ic_photo_camera);
        MENU_ICON_MAP.put(R.id.get_image_descriptions_id, R.drawable.ic_image_descriptions);
        MENU_ICON_MAP.put(R.id.listen_to_feed_id, R.drawable.ic_play_circle);
    }

    /**
     * Applies user customization settings to hide or show menu items based on saved preferences.
     * Removes items from the menu that the user has chosen to hide in the customization settings.
     *
     * @param resources the resources used to access the entry name of a given menu item ID
     * @param modelList the mutable list of menu items to apply customization to; must not be {@code
     *     null}
     */
    public static void applyCustomization(
            final Resources resources, final MVCListAdapter.ModelList modelList) {
        for (Iterator<MVCListAdapter.ListItem> it = modelList.iterator(); it.hasNext(); ) {
            MVCListAdapter.ListItem item = it.next();
            final int menuItemId = item.model.get(AppMenuItemProperties.MENU_ITEM_ID);
            // Skip current item if it matches the customize menu id or settings menu id.
            if (BRAVE_CUSTOMIZE_ITEM_ID == menuItemId || R.id.preferences_id == menuItemId) {
                continue;
            }
            if (!isVisible(resources, menuItemId)) {
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
        MVCListAdapter.ListItem separator = null;

        for (Iterator<MVCListAdapter.ListItem> it = modelList.iterator(); it.hasNext(); ) {
            MVCListAdapter.ListItem curr = it.next();
            if (curr.type == AppMenuHandler.AppMenuItemType.DIVIDER) {
                // Just mark a separator was met, add it later when a standard item arrives.
                separator = curr;
                continue;
            }

            if (!out.isEmpty() && separator != null) {
                // Add a single separator between standard menu items.
                out.add(separator);
            }
            out.add(curr);
            // Reset after placing standard menu item.
            separator = null;
        }

        modelList.clear();
        if (!out.isEmpty()) {
            modelList.addAll(out);
        }
    }

    /**
     * Opens the Customize Menu settings screen where users can toggle visibility of menu items.
     * Passes the current menu items and page actions to the settings fragment for display.
     *
     * @param context the Android context used to launch the settings activity
     * @param menuItems the current list of main menu items to customize
     * @param pageActions the current list of page action menu items to customize
     */
    public static void openCustomizeMenuSettings(
            final Context context,
            final MVCListAdapter.ModelList menuItems,
            final MVCListAdapter.ModelList pageActions) {
        final Bundle bundle = new Bundle();
        populateBundle(context.getResources(), bundle, menuItems, pageActions);

        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(context, BraveCustomizeMenuPreferenceFragment.class, bundle);
    }

    /**
     * Populates a bundle with menu item data for passing between fragments. Converts menu items and
     * page actions into parcelable data structures.
     *
     * @param resources the resources used to access the entry name of a given menu item ID
     * @param bundle the bundle to populate with menu item data
     * @param menuItems the list of main menu items to convert to parcelable data
     * @param pageActions the list of page action items to convert to parcelable data
     * @return the same bundle instance passed in, now populated with menu data
     */
    public static Bundle populateBundle(
            final Resources resources,
            final Bundle bundle,
            final MVCListAdapter.ModelList menuItems,
            final MVCListAdapter.ModelList pageActions) {

        // Convert menu items to parcelable data.
        final ArrayList<MenuItemData> menuItemDataList = new ArrayList<>();
        populateMenuItemData(resources, menuItemDataList, menuItems);

        // Convert page action items to parcelable data.
        final ArrayList<MenuItemData> pageActionDataList = new ArrayList<>();
        populateMenuItemData(resources, pageActionDataList, pageActions);

        // Add data list to fragment bundle.
        bundle.putParcelableArrayList(KEY_MAIN_MENU_ITEM_LIST, menuItemDataList);
        bundle.putParcelableArrayList(KEY_PAGE_ACTION_ITEM_LIST, pageActionDataList);
        return bundle;
    }

    private static void populateMenuItemData(
            final Resources resources,
            final List<MenuItemData> menuItemDataList,
            final MVCListAdapter.ModelList menuItems) {
        for (int i = 0; i < menuItems.size(); i++) {
            MVCListAdapter.ListItem item = menuItems.get(i);

            if (item.type == AppMenuHandler.AppMenuItemType.DIVIDER) {
                continue;
            }
            int id = item.model.get(AppMenuItemProperties.MENU_ITEM_ID);
            CharSequence title = item.model.get(AppMenuItemProperties.TITLE);
            menuItemDataList.add(new MenuItemData(id, title.toString(), isVisible(resources, id)));
        }
    }

    /**
     * Checks whether a menu item should be visible based on user preferences. Returns true by
     * default if no preference has been set.
     *
     * @param resources the resources used to access the entry name of a given menu item ID
     * @param itemId the resource ID of the menu item to check
     * @return {@code true} if the item should be visible, {@code false} if it should be hidden
     */
    public static boolean isVisible(final Resources resource, final int itemId) {
        String resourceName;
        try {
            resourceName = resource.getResourceEntryName(itemId);
        } catch (Resources.NotFoundException notFoundException) {
            assert false : "Resource not found for item with ID " + itemId;
            // We are referencing a resource that does not
            // exist. This should never happen.
            // Leave visibility unchanged by returning true.
            return true;
        }
        return ChromeSharedPreferences.getInstance()
                .readBoolean(
                        String.format(
                                Locale.ENGLISH,
                                CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT,
                                resourceName),
                        true);
    }

    /**
     * Maps a menu item ID to its corresponding drawable resource ID. Used to display appropriate
     * icons for menu items in the settings screen.
     *
     * @param menuItemId the resource ID of the menu item
     * @return the drawable resource ID for the menu item's icon, or 0 if no icon is defined
     */
    @DrawableRes
    public static int getDrawableResFromMenuItemId(@IdRes final int menuItemId) {
        final int drawableRes = MENU_ICON_MAP.get(menuItemId, 0);

        assert drawableRes != 0 : "Unexpected value for menu item ID: " + menuItemId;
        return drawableRes;
    }

    /**
     * Creates a standardized menu icon drawable with consistent size and tinting asynchronously.
     * Ensures all menu icons have uniform appearance by applying standard sizing and color tinting.
     * Icons smaller than the target size are scaled up to match the standard menu icon dimensions.
     * Bitmap recreation is performed on a background thread to avoid blocking the UI thread.
     *
     * @param context the Android context for accessing resources
     * @param drawableRes the drawable resource ID to standardize
     * @param iconSizePx the target icon size in pixels (must be positive)
     * @param callback the callback to receive the standardized drawable, or {@code null} if the
     *     resource cannot be loaded
     */
    @SuppressWarnings("NullAway")
    public static void getStandardizedMenuIconAsync(
            final Context context,
            @DrawableRes final int drawableRes,
            @Size(min = 1) final int iconSizePx,
            final Callback<Drawable> callback) {
        if (drawableRes == 0) {
            callback.onResult(null);
            return;
        }

        Drawable src = ContextCompat.getDrawable(context, drawableRes);
        if (src == null) {
            callback.onResult(null);
            return;
        }

        // Work on a mutated instance to avoid shared state issues.
        final Drawable drawable = src.mutate();

        final int width = drawable.getIntrinsicWidth();
        final int height = drawable.getIntrinsicHeight();

        if (width < 1 || height < 1) {
            callback.onResult(null);
            return;
        }

        // If required, standardize icon size to match menu icons.
        if (width < iconSizePx || height < iconSizePx) {
            // Offload bitmap recreation to background thread.
            PostTask.postTask(
                    TaskTraits.BEST_EFFORT_MAY_BLOCK,
                    () -> {
                        // Create bitmap on background thread.
                        Bitmap bitmap =
                                Bitmap.createBitmap(
                                        iconSizePx, iconSizePx, Bitmap.Config.ARGB_8888);
                        bitmap.setDensity(context.getResources().getDisplayMetrics().densityDpi);
                        Canvas canvas = new Canvas(bitmap);
                        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
                        drawable.draw(canvas);
                        final Drawable resizedDrawable =
                                new BitmapDrawable(context.getResources(), bitmap);

                        // Apply tinting.
                        maybeApplyTinting(context, drawable);

                        // Return to UI thread with result.
                        PostTask.postTask(
                                TaskTraits.UI_DEFAULT, () -> callback.onResult(resizedDrawable));
                    });
        } else {
            // No resizing needed, apply tinting and return directly.
            maybeApplyTinting(context, drawable);
            callback.onResult(drawable);
        }
    }

    private static void maybeApplyTinting(final Context context, final Drawable drawable) {
        final ColorStateList tintList =
                AppCompatResources.getColorStateList(context, DEFAULT_COLOR_RES_MENU_ITEM_ICON);
        if (tintList != null) {
            drawable.setTintList(tintList);
        }
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
