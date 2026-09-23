/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.util;

import org.jni_zero.JNINamespace;
import org.jni_zero.JniType;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;

/**
 * Utility class for accessing Brave domains services from Java. Provides access to C++
 * brave_domains::GetServicesDomain function
 */
@NullMarked
@JNINamespace("brave_domains::android")
public class BraveDomainsUtils {
    /**
     * Gets the services domain for a given prefix and environment. This is a wrapper around the C++
     * brave_domains::GetServicesDomain function.
     *
     * @param prefix The service prefix (e.g., "leo", "origin", "vpn")
     * @param environment The environment (ServicesEnvironment.DEV, ServicesEnvironment.STAGING, or
     *     ServicesEnvironment.PROD)
     * @return The constructed domain string
     */
    public static String getServicesDomain(String prefix, @ServicesEnvironment int environment) {
        return BraveDomainsUtilsJni.get().getServicesDomain(prefix, environment);
    }

    @NativeMethods
    interface Natives {
        @JniType("std::string")
        String getServicesDomain(@JniType("std::string") String prefix, int environment);
    }
}
