#include <gtest/gtest.h>
#include "../../core/transfers/IFileTransfer.hpp"

using namespace peerchat::transfers;

TEST(SanitizationTest, FilenamePathTraversalPrevention) {
    // Standard relative traversal
    EXPECT_EQ(IFileTransfer::sanitizeFilename("../../etc/passwd"), "passwd");
    EXPECT_EQ(IFileTransfer::sanitizeFilename("../../../secret.key"), "secret.key");

    // Windows backslash traversal
    EXPECT_EQ(IFileTransfer::sanitizeFilename("..\\..\\windows\\system32\\cmd.exe"), "cmd.exe");

    // Slashes within basename
    EXPECT_EQ(IFileTransfer::sanitizeFilename("normal_file.txt"), "normal_file.txt");

    // Empty or pure dots
    QString safe1 = IFileTransfer::sanitizeFilename("");
    EXPECT_TRUE(safe1.startsWith("attachment_"));

    QString safe2 = IFileTransfer::sanitizeFilename(".");
    EXPECT_TRUE(safe2.startsWith("attachment_"));

    QString safe3 = IFileTransfer::sanitizeFilename("..");
    EXPECT_TRUE(safe3.startsWith("attachment_"));

    // Embedded null bytes and control chars
    QString tainted = QString::fromUtf8("bad\0file\r\n.png", 14);
    QString cleaned = IFileTransfer::sanitizeFilename(tainted);
    EXPECT_FALSE(cleaned.contains('\0'));
    EXPECT_FALSE(cleaned.contains('\r'));
    EXPECT_FALSE(cleaned.contains('\n'));
}
