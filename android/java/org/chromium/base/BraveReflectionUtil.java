/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import org.chromium.base.Log;

import java.lang.Class;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class BraveReflectionUtil {
    private static String TAG = "BraveReflectionUtil";

    // NOTE: For each method for invocation add appropriate test to `testMethodsForInvocationExist`
    // method in 'brave/android/javatests/org/chromium/chrome/browser/BytecodeTest.java' file with
    // checking parameter types.
    public static Object InvokeMethod(
            Class methodOwner, Object obj, String method, Object... typesAndArgs) {
        try {
            Class<?>[] parameterTypes = null;
            Object[] args = null;
            if (typesAndArgs != null) {
                // It must be even number as first goes type and then argument itself
                assert (typesAndArgs.length % 2 == 0);
                int size = typesAndArgs.length / 2;
                parameterTypes = new Class<?>[ size ];
                args = new Object[size];
                for (int i = 0; i < typesAndArgs.length; i++) {
                    if (i % 2 == 0) {
                        parameterTypes[i / 2] = (Class<?>) typesAndArgs[i];
                    } else {
                        args[i / 2] = typesAndArgs[i];
                    }
                }
            }
            Method toInvoke = methodOwner.getDeclaredMethod(method, parameterTypes);
            try {
                return toInvoke.invoke(obj, args);
            } catch (IllegalAccessException e) {
                Log.e(TAG, "Illegal access for method: " + e);
                assert (false);
            } catch (InvocationTargetException e) {
                Log.e(TAG, "Method invocation error e: " + e);
                assert (false);
            }
        } catch (NoSuchMethodException e) {
            Log.e(TAG, "Method not found: " + e);
            assert (false);
        }
        return null;
    }

    // Types should be compatible after bytecode patching
    @SuppressWarnings("EqualsIncompatibleType")
    public static Boolean EqualTypes(Class type1, Class type2) {
        return type1.equals(type2);
    }
}
