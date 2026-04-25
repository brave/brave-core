/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.firstrun;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.text.SpannableString;
import android.view.ContextThemeWrapper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.test.core.app.ApplicationProvider;

import com.google.android.material.checkbox.MaterialCheckBox;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave.browser.firstrun.R;

/** Unit tests for {@link OnboardingStepAdapter}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class OnboardingStepAdapterTest {
    private static final int HELP_BRAVE_SEARCH = 0;
    private static final int BLOCK_INTERRUPTIONS = 1;
    private static final int MAKE_BRAVE_BETTER = 2;
    private static final int TOTAL_STEPS = 3;

    private ViewGroup mParent;
    private SpannableString mWdpLearnMore;
    private TestListener mListener;

    @Before
    public void setUp() {
        ContextThemeWrapper themedContext =
                new ContextThemeWrapper(
                        ApplicationProvider.getApplicationContext(),
                        R.style.Theme_Brave_WelcomeOnboarding);
        mParent = new android.widget.FrameLayout(themedContext);
        mWdpLearnMore = new SpannableString("Learn more about WDP");
        mListener = new TestListener();
    }

    @Test
    public void testItemCountAndViewTypes() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);

        assertEquals(TOTAL_STEPS, adapter.getItemCount());
        assertEquals(HELP_BRAVE_SEARCH, adapter.getItemViewType(0));
        assertEquals(BLOCK_INTERRUPTIONS, adapter.getItemViewType(1));
        assertEquals(MAKE_BRAVE_BETTER, adapter.getItemViewType(2));
    }

    @Test
    public void testHelpBraveSearchButtons() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 0);
        TextView subtitle = holder.itemView.findViewById(R.id.brave_onboarding_subtitle);
        Button later = holder.itemView.findViewById(R.id.onboarding_later);
        Button sure = holder.itemView.findViewById(R.id.onboarding_sure);

        assertNotNull(subtitle);
        assertNotNull(later);
        assertNotNull(sure);
        assertEquals(mWdpLearnMore.toString(), subtitle.getText().toString());

        later.performClick();
        assertEquals(1, mListener.mRequestedPage);
        assertEquals(0, mListener.mWebDiscoverEnabledCount);

        mListener.reset();
        sure.performClick();
        assertEquals(1, mListener.mRequestedPage);
        assertEquals(1, mListener.mWebDiscoverEnabledCount);
    }

    @Test
    public void testBlockInterruptionsNextPage() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 1);
        Button continueButton = holder.itemView.findViewById(R.id.onboarding_continue);

        assertNotNull(continueButton);
        continueButton.performClick();
        assertEquals(2, mListener.mRequestedPage);

        adapter.onViewRecycled(holder);
    }

    @Test
    public void testMakeBraveBetterManagedHidesCheckboxes() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);
        adapter.setCrashReportingManaged(true);
        adapter.setP3aManaged(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 2);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);
        Button startBrowsing = holder.itemView.findViewById(R.id.onboarding_start_browsing);

        assertNotNull(crashReports);
        assertNotNull(p3aReports);
        assertNotNull(startBrowsing);
        assertEquals(View.INVISIBLE, crashReports.getVisibility());
        assertEquals(View.INVISIBLE, p3aReports.getVisibility());

        startBrowsing.performClick();
        assertEquals(1, mListener.mDismissCount);
    }

    @Test
    public void testMakeBraveBetterCrashesManagedHidesCheckbox() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);
        adapter.setCrashReportingManaged(true);
        adapter.setP3aManaged(false);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 2);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);

        assertNotNull(crashReports);
        assertNotNull(p3aReports);
        assertEquals(View.INVISIBLE, crashReports.getVisibility());
        assertEquals(View.VISIBLE, p3aReports.getVisibility());
    }

    @Test
    public void testMakeBraveBetterP3aManagedHidesCheckbox() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);
        adapter.setCrashReportingManaged(false);
        adapter.setP3aManaged(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 2);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);

        assertNotNull(crashReports);
        assertNotNull(p3aReports);
        assertEquals(View.VISIBLE, crashReports.getVisibility());
        assertEquals(View.INVISIBLE, p3aReports.getVisibility());
    }

    @Test
    public void testMakeBraveBetterCrashesCheckedSetsCheckbox() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);
        adapter.setCrashReportingManaged(false);
        adapter.setCrashReportingChecked(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 2);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);

        assertNotNull(crashReports);
        assertEquals(View.VISIBLE, crashReports.getVisibility());
        assertTrue(crashReports.isChecked());
    }

    @Test
    public void testMakeBraveBetterP3aCheckedSetsCheckbox() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);
        adapter.setP3aManaged(false);
        adapter.setP3aChecked(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 2);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);

        assertNotNull(p3aReports);
        assertEquals(View.VISIBLE, p3aReports.getVisibility());
        assertTrue(p3aReports.isChecked());
    }

    @Test
    public void testMakeBraveBetterCrashReportingToggleNotifiesListener() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);
        adapter.setCrashReportingManaged(false);
        adapter.setCrashReportingChecked(false);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 2);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);

        assertNotNull(crashReports);
        assertEquals(View.VISIBLE, crashReports.getVisibility());
        assertEquals(0, mListener.mCrashReportingChangeCount);

        crashReports.setChecked(true);
        assertEquals(1, mListener.mCrashReportingChangeCount);
        assertTrue(mListener.mLastCrashReportingEnabled);

        crashReports.setChecked(false);
        assertEquals(2, mListener.mCrashReportingChangeCount);
        assertFalse(mListener.mLastCrashReportingEnabled);
    }

    @Test
    public void testMakeBraveBetterP3aToggleNotifiesListener() {
        OnboardingStepAdapter adapter = new OnboardingStepAdapter(mWdpLearnMore, mListener);
        adapter.setP3aManaged(false);
        adapter.setP3aChecked(false);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                adapter.onCreateViewHolder(mParent, 2);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);

        assertNotNull(p3aReports);
        assertEquals(View.VISIBLE, p3aReports.getVisibility());
        assertEquals(0, mListener.mP3aChangeCount);

        p3aReports.setChecked(true);
        assertEquals(1, mListener.mP3aChangeCount);
        assertTrue(mListener.mLastP3aEnabled);

        p3aReports.setChecked(false);
        assertEquals(2, mListener.mP3aChangeCount);
        assertFalse(mListener.mLastP3aEnabled);
    }

    private static final class TestListener
            implements OnboardingStepAdapter.OnboardingNavigationListener {
        int mRequestedPage = -1;
        int mDismissCount;
        int mWebDiscoverEnabledCount;
        int mCrashReportingChangeCount;
        int mP3aChangeCount;
        boolean mLastCrashReportingEnabled;
        boolean mLastP3aEnabled;

        @Override
        public void onRequestPageChange(int position) {
            mRequestedPage = position;
        }

        @Override
        public void onDismiss() {
            mDismissCount++;
        }

        @Override
        public void onWebDiscoverPreferenceEnabled() {
            mWebDiscoverEnabledCount++;
        }

        @Override
        public void onCrashReportingPreferenceChanged(final boolean enabled) {
            mCrashReportingChangeCount++;
            mLastCrashReportingEnabled = enabled;
        }

        @Override
        public void onP3aPreferenceChanged(final boolean enabled) {
            mP3aChangeCount++;
            mLastP3aEnabled = enabled;
        }

        void reset() {
            mRequestedPage = -1;
            mDismissCount = 0;
            mWebDiscoverEnabledCount = 0;
            mCrashReportingChangeCount = 0;
            mP3aChangeCount = 0;
            mLastCrashReportingEnabled = false;
            mLastP3aEnabled = false;
        }
    }
}
