/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.firstrun;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import static org.chromium.chrome.browser.day_zero.DayZeroHelper.DAY_ZERO_DEFAULT_VARIANT;
import static org.chromium.chrome.browser.day_zero.DayZeroHelper.DAY_ZERO_VARIANT_Y;

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
    private static final int CUSTOMIZE_BRAVE = 2;
    private static final int TOTAL_STEPS = 3;

    private ViewGroup mParent;
    private SpannableString mWdpLearnMore;
    private PageBounceAnimator mPageBounceAnimator;
    private TestListener mListener;

    @Before
    public void setUp() {
        ContextThemeWrapper themedContext =
                new ContextThemeWrapper(
                        ApplicationProvider.getApplicationContext(),
                        R.style.Theme_Brave_WelcomeOnboarding);
        mParent = new android.widget.FrameLayout(themedContext);
        mWdpLearnMore = new SpannableString("Learn more about WDP");
        mPageBounceAnimator = mock(PageBounceAnimator.class);
        mListener = new TestListener();
    }

    private OnboardingStepAdapter createAdapter() {
        return createAdapter(DAY_ZERO_DEFAULT_VARIANT, false);
    }

    private OnboardingStepAdapter createAdapter(
            final String dayZeroVariant, final boolean skipMakeBraveBetter) {
        return new OnboardingStepAdapter(
                mWdpLearnMore, dayZeroVariant, skipMakeBraveBetter, mPageBounceAnimator, mListener);
    }

    private OnboardingStepAdapter.OnboardingBaseViewHolder createViewHolder(
            final OnboardingStepAdapter adapter, final int position) {
        return adapter.onCreateViewHolder(mParent, adapter.getItemViewType(position));
    }

    @Test
    public void testItemCountAndViewTypes() {
        OnboardingStepAdapter adapter = createAdapter();

        assertEquals(TOTAL_STEPS, adapter.getItemCount());
        assertEquals(R.layout.help_brave_search, adapter.getItemViewType(HELP_BRAVE_SEARCH));
        assertEquals(R.layout.block_interruptions, adapter.getItemViewType(BLOCK_INTERRUPTIONS));
        assertEquals(R.layout.make_brave_better, adapter.getItemViewType(MAKE_BRAVE_BETTER));
    }

    @Test
    public void testHelpBraveSearchButtons() {
        OnboardingStepAdapter adapter = createAdapter();

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, HELP_BRAVE_SEARCH);
        TextView subtitle = holder.itemView.findViewById(R.id.brave_onboarding_subtitle);
        Button later = holder.itemView.findViewById(R.id.onboarding_later);
        Button sure = holder.itemView.findViewById(R.id.onboarding_sure);

        assertNotNull(subtitle);
        assertNotNull(later);
        assertNotNull(sure);
        assertEquals(mWdpLearnMore.toString(), subtitle.getText().toString());

        later.performClick();
        verify(mPageBounceAnimator).animateToPosition(BLOCK_INTERRUPTIONS);
        assertEquals(0, mListener.mWebDiscoverEnabledCount);

        sure.performClick();
        verify(mPageBounceAnimator, times(2)).animateToPosition(BLOCK_INTERRUPTIONS);
        assertEquals(1, mListener.mWebDiscoverEnabledCount);
    }

    @Test
    public void testBlockInterruptionsNextPage() {
        OnboardingStepAdapter adapter = createAdapter();

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, BLOCK_INTERRUPTIONS);
        Button continueButton = holder.itemView.findViewById(R.id.onboarding_continue);

        assertNotNull(continueButton);
        continueButton.performClick();
        verify(mPageBounceAnimator).animateToPosition(MAKE_BRAVE_BETTER);
        assertEquals(0, mListener.mDismissCount);

        adapter.onViewRecycled(holder);
    }

    @Test
    public void testBlockInterruptionsDismissesWhenMakeBraveBetterSkipped() {
        OnboardingStepAdapter adapter = createAdapter(DAY_ZERO_DEFAULT_VARIANT, true);

        assertEquals(TOTAL_STEPS - 1, adapter.getItemCount());

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, BLOCK_INTERRUPTIONS);
        Button continueButton = holder.itemView.findViewById(R.id.onboarding_continue);

        continueButton.performClick();
        verify(mPageBounceAnimator, never()).animateToPosition(anyInt());
        assertEquals(1, mListener.mDismissCount);
    }

    @Test
    public void testCustomizeStepVariant() {
        OnboardingStepAdapter adapter = createAdapter(DAY_ZERO_VARIANT_Y, false);

        assertEquals(TOTAL_STEPS + 1, adapter.getItemCount());
        assertEquals(R.layout.customize_brave, adapter.getItemViewType(CUSTOMIZE_BRAVE));

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, BLOCK_INTERRUPTIONS);
        Button continueButton = holder.itemView.findViewById(R.id.onboarding_continue);

        continueButton.performClick();
        verify(mPageBounceAnimator).animateToPosition(CUSTOMIZE_BRAVE);
    }

    @Test
    public void testMakeBraveBetterManagedHidesCheckboxes() {
        OnboardingStepAdapter adapter = createAdapter();
        adapter.setCrashReportingManaged(true);
        adapter.setP3aManaged(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, MAKE_BRAVE_BETTER);
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
        OnboardingStepAdapter adapter = createAdapter();
        adapter.setCrashReportingManaged(true);
        adapter.setP3aManaged(false);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, MAKE_BRAVE_BETTER);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);

        assertNotNull(crashReports);
        assertNotNull(p3aReports);
        assertEquals(View.INVISIBLE, crashReports.getVisibility());
        assertEquals(View.VISIBLE, p3aReports.getVisibility());
    }

    @Test
    public void testMakeBraveBetterP3aManagedHidesCheckbox() {
        OnboardingStepAdapter adapter = createAdapter();
        adapter.setCrashReportingManaged(false);
        adapter.setP3aManaged(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, MAKE_BRAVE_BETTER);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);

        assertNotNull(crashReports);
        assertNotNull(p3aReports);
        assertEquals(View.VISIBLE, crashReports.getVisibility());
        assertEquals(View.INVISIBLE, p3aReports.getVisibility());
    }

    @Test
    public void testMakeBraveBetterCrashesCheckedSetsCheckbox() {
        OnboardingStepAdapter adapter = createAdapter();
        adapter.setCrashReportingManaged(false);
        adapter.setCrashReportingChecked(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, MAKE_BRAVE_BETTER);
        MaterialCheckBox crashReports = holder.itemView.findViewById(R.id.send_crash_reports);

        assertNotNull(crashReports);
        assertEquals(View.VISIBLE, crashReports.getVisibility());
        assertTrue(crashReports.isChecked());
    }

    @Test
    public void testMakeBraveBetterP3aCheckedSetsCheckbox() {
        OnboardingStepAdapter adapter = createAdapter();
        adapter.setP3aManaged(false);
        adapter.setP3aChecked(true);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, MAKE_BRAVE_BETTER);
        MaterialCheckBox p3aReports = holder.itemView.findViewById(R.id.send_p3a_reports);

        assertNotNull(p3aReports);
        assertEquals(View.VISIBLE, p3aReports.getVisibility());
        assertTrue(p3aReports.isChecked());
    }

    @Test
    public void testMakeBraveBetterCrashReportingToggleNotifiesListener() {
        OnboardingStepAdapter adapter = createAdapter();
        adapter.setCrashReportingManaged(false);
        adapter.setCrashReportingChecked(false);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, MAKE_BRAVE_BETTER);
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
        OnboardingStepAdapter adapter = createAdapter();
        adapter.setP3aManaged(false);
        adapter.setP3aChecked(false);

        OnboardingStepAdapter.OnboardingBaseViewHolder holder =
                createViewHolder(adapter, MAKE_BRAVE_BETTER);
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
        int mDismissCount;
        int mWebDiscoverEnabledCount;
        int mCrashReportingChangeCount;
        int mP3aChangeCount;
        boolean mLastCrashReportingEnabled;
        boolean mLastP3aEnabled;

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
    }
}
