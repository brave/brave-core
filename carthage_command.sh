xcodesdk=`xcodebuild -scheme Debug  -showBuildSettings  | grep -i 'SDK_VERSION =' | sed 's/[ ]*SDK_VERSION = //' | colrm 3`
echo Xcode SDK: "$xcodesdk"
if [[ "$xcodesdk" != "13" ]]; then
  echo XCode 12 version detected! ••• Please ensure this is correct. •••
echo 'EXCLUDED_ARCHS__EFFECTIVE_PLATFORM_SUFFIX_simulator__NATIVE_ARCH_64_BIT_x86_64=arm64 arm64e armv7 armv7s armv6 armv8' > /tmp/tmp.xcconfig
echo 'EXCLUDED_ARCHS=$(inherited) $(EXCLUDED_ARCHS__EFFECTIVE_PLATFORM_SUFFIX_$(EFFECTIVE_PLATFORM_SUFFIX)__NATIVE_ARCH_64_BIT_$(NATIVE_ARCH_64_BIT))' >> /tmp/tmp.xcconfig
export XCODE_XCCONFIG_FILE=/tmp/tmp.xcconfig
fi

carthage bootstrap $CARTHAGE_VERBOSE --platform ios --color auto --cache-builds --no-use-binaries

