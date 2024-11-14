/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.materialswitch.MaterialSwitch;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.quick_search_engines.ItemTouchHelperCallback;
import org.chromium.chrome.browser.quick_search_engines.utils.QuickSearchEnginesUtil;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.util.ImageUtils;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class QuickSearchEnginesFragment extends BravePreferenceFragment
        implements QuickSearchEnginesCallback, ItemTouchHelperCallback.OnStartDragListener {
    private RecyclerView mRecyclerView;
    private QuickSearchEnginesAdapter mQuickSearchEnginesAdapter;
    private ItemTouchHelper mItemTouchHelper;

    private MenuItem mCloseItem;
    private MenuItem mSaveItem;

    private LinearLayout mDefaultSearchEngineLayout;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.quick_search_engines));
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_quick_search, container, false);

        LinearLayout quickSearchOptionsLayout = view.findViewById(R.id.quick_search_options_layout);

        // Quick search feature layout
        LinearLayout quickSearchFeatureLayout = view.findViewById(R.id.quick_search_feature_layout);
        TextView quickSearchFeatureText =
                quickSearchFeatureLayout.findViewById(R.id.search_engine_text);
        quickSearchFeatureText.setText(getString(R.string.show_quick_search_bar));
        MaterialSwitch quickSearchFeatureSwitch =
                quickSearchFeatureLayout.findViewById(R.id.search_engine_switch);
        quickSearchFeatureLayout.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        boolean isChecked = quickSearchFeatureSwitch.isChecked();
                        quickSearchFeatureSwitch.setChecked(!isChecked);
                    }
                });
        quickSearchFeatureLayout.findViewById(R.id.search_engine_logo).setVisibility(View.GONE);
        quickSearchFeatureSwitch.setChecked(QuickSearchEnginesUtil.getQuickSearchEnginesFeature());
        quickSearchFeatureSwitch.setOnCheckedChangeListener(
                new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        quickSearchOptionsLayout.setVisibility(
                                isChecked ? View.VISIBLE : View.GONE);
                        QuickSearchEnginesUtil.setQuickSearchEnginesFeature(isChecked);
                    }
                });

        quickSearchOptionsLayout.setVisibility(
                QuickSearchEnginesUtil.getQuickSearchEnginesFeature() ? View.VISIBLE : View.GONE);

        // Default search engine layout
        QuickSearchEnginesModel defaultSearchEnginesModel =
                QuickSearchEnginesUtil.getDefaultSearchEngine(getProfile());
        mDefaultSearchEngineLayout = view.findViewById(R.id.default_search_engine_layout);
        ImageView defaultSearchEngineLogo =
                mDefaultSearchEngineLayout.findViewById(R.id.search_engine_logo);
        ImageUtils.loadSearchEngineLogo(
                getProfile(), defaultSearchEngineLogo, defaultSearchEnginesModel.getKeyword());
        TextView defaultSearchEngineText =
                mDefaultSearchEngineLayout.findViewById(R.id.search_engine_text);
        defaultSearchEngineText.setText(defaultSearchEnginesModel.getShortName());
        MaterialSwitch defaultSearchEngineSwitch =
                mDefaultSearchEngineLayout.findViewById(R.id.search_engine_switch);
        mDefaultSearchEngineLayout.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        boolean isChecked = defaultSearchEngineSwitch.isChecked();
                        defaultSearchEngineSwitch.setChecked(!isChecked);
                    }
                });

        defaultSearchEngineSwitch.setChecked(defaultSearchEnginesModel.isEnabled());
        defaultSearchEngineSwitch.setOnCheckedChangeListener(
                new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        defaultSearchEnginesModel.setEnabled(isChecked);
                        updateQuickSearchEnginesInPref(defaultSearchEnginesModel);
                    }
                });

        mRecyclerView = (RecyclerView) view.findViewById(R.id.quick_search_settings_recyclerview);
        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        setHasOptionsMenu(true);
        return view;
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.quick_search_engines_menu, menu);
        mCloseItem = menu.findItem(R.id.close_menu_id);
        mSaveItem = menu.findItem(R.id.action_save);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.action_save) {
            if (mQuickSearchEnginesAdapter != null
                    && mQuickSearchEnginesAdapter.getQuickSearchEngines() != null
                    && mQuickSearchEnginesAdapter.getQuickSearchEngines().size() > 0) {
                Map<String, QuickSearchEnginesModel> searchEnginesMap =
                        new LinkedHashMap<String, QuickSearchEnginesModel>();
                QuickSearchEnginesModel defaultSearchEnginesModel =
                        QuickSearchEnginesUtil.getDefaultSearchEngine(getProfile());
                searchEnginesMap.put(
                        defaultSearchEnginesModel.getKeyword(), defaultSearchEnginesModel);
                for (QuickSearchEnginesModel quickSearchEnginesModel :
                        mQuickSearchEnginesAdapter.getQuickSearchEngines()) {
                    searchEnginesMap.put(
                            quickSearchEnginesModel.getKeyword(), quickSearchEnginesModel);
                }
                QuickSearchEnginesUtil.saveSearchEnginesIntoPref(searchEnginesMap);
                mQuickSearchEnginesAdapter.setEditMode(false);
                editModeUiVisibility();
            }
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        refreshData();
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void refreshData() {
        List<QuickSearchEnginesModel> quickSearchEngines =
                QuickSearchEnginesUtil.getQuickSearchEnginesForSettings(getProfile());
        setRecyclerViewData(quickSearchEngines);
    }

    private void setRecyclerViewData(List<QuickSearchEnginesModel> searchEngines) {
        mQuickSearchEnginesAdapter =
                new QuickSearchEnginesAdapter(getActivity(), searchEngines, this, this);
        mRecyclerView.setAdapter(mQuickSearchEnginesAdapter);
        ItemTouchHelper.Callback callback = new ItemTouchHelperCallback(mQuickSearchEnginesAdapter);
        mItemTouchHelper = new ItemTouchHelper(callback);
        mItemTouchHelper.attachToRecyclerView(mRecyclerView);
    }

    // QuickSearchCallback
    @Override
    public void onSearchEngineClick(int position, QuickSearchEnginesModel quickSearchEnginesModel) {
        updateQuickSearchEnginesInPref(quickSearchEnginesModel);
    }

    private void updateQuickSearchEnginesInPref(QuickSearchEnginesModel quickSearchEnginesModel) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap =
                QuickSearchEnginesUtil.getQuickSearchEnginesFromPref();
        searchEnginesMap.put(quickSearchEnginesModel.getKeyword(), quickSearchEnginesModel);
        QuickSearchEnginesUtil.saveSearchEnginesIntoPref(searchEnginesMap);
    }

    @Override
    public void onSearchEngineLongClick() {
        editModeUiVisibility();
    }

    @Override
    public void onStartDrag(RecyclerView.ViewHolder viewHolder) {
        mItemTouchHelper.startDrag(viewHolder);
    }

    private void editModeUiVisibility() {
        if (mQuickSearchEnginesAdapter != null) {
            boolean isEditMode = mQuickSearchEnginesAdapter.isEditMode();
            mDefaultSearchEngineLayout.setEnabled(!isEditMode);
            mDefaultSearchEngineLayout.setAlpha(!isEditMode ? 1.0f : 0.5f);
            if (mCloseItem != null) {
                mCloseItem.setVisible(!isEditMode);
            }
            if (mSaveItem != null) {
                mSaveItem.setVisible(isEditMode);
            }
        }
    }

    @Override
    public void loadSearchEngineLogo(
            ImageView logoView, QuickSearchEnginesModel quickSearchEnginesModel) {
        ImageUtils.loadSearchEngineLogo(
                getProfile(), logoView, quickSearchEnginesModel.getKeyword());
    }
}
