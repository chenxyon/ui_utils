/**
 * @file     version_check.cpp
 * @brief    版本检查组件实现
 *
 * 修改：2026-11-21 重构为 VersionCheck 静态类；新增版本源注入与当前版本设置；
 *       字符串拷贝加终止符防护
 */
#include "version_check.h"

#include <string.h>

VersionInfo VersionCheck::s_info = {
    .currentVersion = "v1.0.0",
    .latestVersion = "v1.0.0",
    .updateLog = "",
    .packageSize = "",
    .updateAvailable = false,
    .checkInProgress = false,
};

VersionSourceCb VersionCheck::s_updateSource = nullptr;

void VersionCheck::safeCopy(char *dst, size_t dstSize, const char *src) {
    if (dst == nullptr || dstSize == 0) {
        return;
    }
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

int VersionCheck::compareVersions(const char *v1, const char *v2) {
    if (v1 == nullptr || v2 == nullptr) {
        return 0;
    }

    const char *p1 = v1;
    const char *p2 = v2;

    /* 跳过 v/V 前缀 */
    if (*p1 == 'v' || *p1 == 'V') p1++;
    if (*p2 == 'v' || *p2 == 'V') p2++;

    while (*p1 != '\0' || *p2 != '\0') {
        int num1 = 0;
        int num2 = 0;

        while (*p1 >= '0' && *p1 <= '9') {
            num1 = num1 * 10 + (*p1 - '0');
            p1++;
        }
        while (*p2 >= '0' && *p2 <= '9') {
            num2 = num2 * 10 + (*p2 - '0');
            p2++;
        }

        if (num1 > num2) return 1;
        if (num1 < num2) return -1;

        if (*p1 == '.') p1++;
        if (*p2 == '.') p2++;
    }

    return 0;
}

void VersionCheck::simulateCheck(VersionInfo &info) {
    /* 保留原实现的模拟行为：最新版本 v1.2.0 */
    safeCopy(info.latestVersion, sizeof(info.latestVersion), "v1.2.0");
    safeCopy(info.updateLog, sizeof(info.updateLog),
             "- Added new features\n- Bug fixes\n- Performance improvements");
    safeCopy(info.packageSize, sizeof(info.packageSize), "1.5 MB");
}

void VersionCheck::init() {
}

void VersionCheck::deinit() {
}

void VersionCheck::start() {
    if (s_info.checkInProgress) {
        return;
    }

    s_info.checkInProgress = true;

    if (s_updateSource != nullptr) {
        s_updateSource(s_info);
    } else {
        simulateCheck(s_info);
    }

    /* 统一按版本号比较结果计算是否有更新 */
    int cmp = compareVersions(s_info.latestVersion, s_info.currentVersion);
    s_info.updateAvailable = (cmp > 0);

    s_info.checkInProgress = false;
}

void VersionCheck::stop() {
    s_info.checkInProgress = false;
}

const VersionInfo &VersionCheck::info() {
    return s_info;
}

bool VersionCheck::hasUpdate() {
    return s_info.updateAvailable;
}

void VersionCheck::setCurrentVersion(const char *version) {
    if (version == nullptr) {
        return;
    }
    safeCopy(s_info.currentVersion, sizeof(s_info.currentVersion), version);
    /* 当前版本变化后重新计算更新状态 */
    int cmp = compareVersions(s_info.latestVersion, s_info.currentVersion);
    s_info.updateAvailable = (cmp > 0);
}

void VersionCheck::setUpdateSource(VersionSourceCb cb) {
    s_updateSource = cb;
}
