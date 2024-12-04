/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_DARK;
import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_LIGHT;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.browser.customtabs.CustomTabsIntent;

import org.chromium.base.IntentUtils;
import org.chromium.ui.LayoutInflaterUtils;
import org.chromium.ui.UiUtils;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.text.ChromeClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.util.ColorUtils;

/* Adds additional items to Permission Dialog. */
class BravePermissionDialogModel {
    // Link for Widevine
    private static final String URL_WIDEVINE_LEARN_MORE =
            "https://support.brave.com/hc/en-us/articles/17428756610061";

    public static PropertyModel getModel(
            ModalDialogProperties.Controller controller,
            PermissionDialogDelegate delegate,
            View customView,
            Runnable touchFilteredCallback) {
        BravePermissionDialogDelegate braveDelegate =
                (BravePermissionDialogDelegate) (Object) delegate;
        PropertyModel model = null;
        if (braveDelegate.getIsWidevinePermissionRequest()) {
            model = createModelForWidevineRequest(controller, delegate, touchFilteredCallback);
        } else {
            model =
                    PermissionDialogModelFactory.getModel(
                            controller, delegate, customView, touchFilteredCallback);
            addLifetimeOptions(customView, delegate);
        }
        assert model != null;

        return model;
    }

    private static PropertyModel createModelForWidevineRequest(
            ModalDialogProperties.Controller controller, PermissionDialogDelegate delegate,
            Runnable touchFilteredCallback) {
        BravePermissionDialogDelegate braveDelegate =
                (BravePermissionDialogDelegate) (Object) delegate;
        Context context = delegate.getWindow().getContext().get();
        assert context != null;
        View customView = LayoutInflaterUtils.inflate(
                context, R.layout.widevine_permission_request_custom_view, null);

        String messageText = delegate.getMessageText();
        // Override Allow button text
        String primaryButtonText =
                context.getResources()
                        .getString(R.string.widevine_permission_request_primary_button_text);
        assert !TextUtils.isEmpty(messageText) && !TextUtils.isEmpty(primaryButtonText);
        SpannableString learnMoreLink =
                SpanApplier.applySpans(
                        context.getResources().getString(R.string.widevine_permission_request_link),
                        new SpanApplier.SpanInfo(
                                "<LINK>",
                                "</LINK>",
                                new ChromeClickableSpan(
                                        context,
                                        R.color.brave_link,
                                        result -> {
                                            openUrlInCustomTab(context, URL_WIDEVINE_LEARN_MORE);
                                        })));

        TextView messageTextView = customView.findViewById(R.id.message);
        messageTextView.setText(messageText);

        TextView linkTextView = customView.findViewById(R.id.link);
        linkTextView.setText(learnMoreLink);
        linkTextView.setClickable(true);
        linkTextView.setMovementMethod(LinkMovementMethod.getInstance());

        CheckBox checkbox = customView.findViewById(R.id.checkbox);
        checkbox.setOnCheckedChangeListener(
                (buttonView, isChecked) -> { braveDelegate.setDontAskAgain(isChecked); });

        PropertyModel model =
                new PropertyModel.Builder(ModalDialogProperties.ALL_KEYS)
                        .with(ModalDialogProperties.CONTROLLER, controller)
                        .with(
                                ModalDialogProperties.TITLE,
                                context.getResources()
                                        .getString(R.string.widevine_permission_request_title))
                        .with(ModalDialogProperties.CUSTOM_VIEW, customView)
                        .with(ModalDialogProperties.POSITIVE_BUTTON_TEXT, primaryButtonText)
                        .with(
                                ModalDialogProperties.NEGATIVE_BUTTON_TEXT,
                                delegate.getNegativeButtonText())
                        .with(ModalDialogProperties.CONTENT_DESCRIPTION, messageText)
                        .with(ModalDialogProperties.FILTER_TOUCH_FOR_SECURITY, true)
                        .with(ModalDialogProperties.TOUCH_FILTERED_CALLBACK, touchFilteredCallback)
                        .with(
                                ModalDialogProperties.BUTTON_TAP_PROTECTION_PERIOD_MS,
                                UiUtils.PROMPT_INPUT_PROTECTION_SHORT_DELAY_MS)
                        .build();

        return model;
    }

    /* Adds a permission lifetime options to a dialog view if lifetime options are available. */
    private static void addLifetimeOptions(View customView, PermissionDialogDelegate delegate) {
        Context context = delegate.getWindow().getContext().get();
        BravePermissionDialogDelegate braveDelegate =
                (BravePermissionDialogDelegate) (Object) delegate;

        String[] lifetimeOptions = braveDelegate.getLifetimeOptions();
        if (lifetimeOptions == null) {
            return;
        }

        LinearLayout layout = (LinearLayout) customView;

        // Create a text label before the lifetime selector.
        TextView lifetimeOptionsText = new TextView(context);
        lifetimeOptionsText.setText(braveDelegate.getLifetimeOptionsText());
        lifetimeOptionsText.setTextAppearance(R.style.TextAppearance_TextMedium_Primary);

        LinearLayout.LayoutParams lifetimeOptionsTextLayoutParams =
                new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lifetimeOptionsTextLayoutParams.setMargins(0, 0, 0, ViewUtils.dpToPx(context, 8));
        lifetimeOptionsText.setLayoutParams(lifetimeOptionsTextLayoutParams);
        layout.addView(lifetimeOptionsText);

        // Create radio buttons with lifetime options.
        RadioGroup radioGroup = new RadioGroup(context);
        int radioButtonIndex = 0;
        for (String lifetimeOption : lifetimeOptions) {
            RadioButton radioButon = new RadioButton(context);
            radioButon.setText(lifetimeOption);
            radioButon.setId(radioButtonIndex);
            radioButtonIndex += 1;
            radioGroup.addView(radioButon);
        }
        radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                braveDelegate.setSelectedLifetimeOption(checkedId);
            }
        });
        radioGroup.check(0);
        layout.addView(radioGroup);
    }

    /* Cannot use TabUtils because no chrome packages access from here */
    private static void openUrlInCustomTab(Context context, String url) {
        CustomTabsIntent customTabIntent =
                new CustomTabsIntent.Builder()
                        .setShowTitle(true)
                        .setColorScheme(ColorUtils.inNightMode(context) ? COLOR_SCHEME_DARK
                                                                        : COLOR_SCHEME_LIGHT)
                        .build();
        Uri uri = Uri.parse(url);
        // customTabIntent.launchUrl(context, uri);
        Intent newIntent = new Intent(customTabIntent.intent);
        newIntent.setAction(Intent.ACTION_VIEW);
        newIntent.setData(uri);

        newIntent.setPackage(context.getPackageName());
        newIntent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
        if (!(context instanceof Activity)) newIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        IntentUtils.addTrustedIntentExtras(newIntent);

        context.startActivity(newIntent);
    }
}
