package org.chromium.brave.browser.custom_app_icons;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.PackageManager;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.base.test.BaseRobolectricTestRunner;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CustomAppIconsManagerTest {

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private SharedPreferencesManager mPrefManager;

    @Mock private Context mContext;
    @Mock private PackageManager mPackageManager;

    private CustomAppIconsManager mCustomAppIconsManager;

    @Before
    public void setup() {
        mCustomAppIconsManager = CustomAppIconsManager.getInstance();
        mCustomAppIconsManager.setPrefManagerForTesting(mPrefManager);

        when(mContext.getPackageName()).thenReturn("com.example.app");
        when(mContext.getPackageManager()).thenReturn(mPackageManager);
    }

    @Test
    public void testSwitchIcon_DisablesAll_EnablesTarget_SavesPreference() {
        CustomAppIconsEnum selectedIcon = CustomAppIconsEnum.ICON_AQUA;
        mCustomAppIconsManager.switchIcon(selectedIcon);
        when(mPrefManager.readString("current_app_icon", CustomAppIconsEnum.ICON_DEFAULT.name()))
                .thenReturn(selectedIcon.name());
        assertEquals(selectedIcon, mCustomAppIconsManager.getCurrentIcon());
    }

    @Test
    public void testGetCurrentIcon_ReturnsValueFromPreferences() {
        when(mPrefManager.readString("current_app_icon", CustomAppIconsEnum.ICON_DEFAULT.name()))
                .thenReturn(CustomAppIconsEnum.ICON_AQUA.name());

        CustomAppIconsEnum currentIcon = mCustomAppIconsManager.getCurrentIcon();
        assertEquals(CustomAppIconsEnum.ICON_AQUA, currentIcon);
    }
}
