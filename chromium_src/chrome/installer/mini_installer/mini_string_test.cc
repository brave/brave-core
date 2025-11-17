#include <chrome/installer/mini_installer/mini_string_test.cc>

// Tests the case insensitive find support of the StackString class.
TEST_F(MiniInstallerStringTest, StackStringFind) {
  static const wchar_t kTestStringSource[] = L"1234ABcD567890";
  static const wchar_t kTestStringFind[] = L"abcd";
  static const wchar_t kTestStringNotFound[] = L"80";

  StackString<MAX_PATH> str;
  EXPECT_TRUE(str.assign(kTestStringSource));
  EXPECT_EQ(str.get(), mini_installer::SearchStringI(str.get(), kTestStringSource));
  EXPECT_EQ(nullptr, mini_installer::SearchStringI(str.get(), kTestStringNotFound));
  const wchar_t* found = mini_installer::SearchStringI(str.get(), kTestStringFind);
  EXPECT_NE(nullptr, found);
  std::wstring check(found, _countof(kTestStringFind) - 1);
  EXPECT_EQ(0, lstrcmpi(check.c_str(), kTestStringFind));
}
