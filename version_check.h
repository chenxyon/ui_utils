/**
 * @file     version_check.h
 * @brief    版本检查组件 — C++ class
 *
 * 功能：固件版本号比较、更新信息（最新版本/更新日志/包大小）管理。
 * 修改：2026-11-21 由 C API（version_check_* 函数）重构为 VersionCheck 静态类；
 *       compareVersions 改为 public 供外部直接比较版本号；
 *       新增 setCurrentVersion 设置当前固件版本；
 *       新增 setUpdateSource 注入真实版本源检查回调（默认保留原模拟行为）；
 *       字符串拷贝统一加终止符防护
 */
#ifndef VERSION_CHECK_H
#define VERSION_CHECK_H

#include <stddef.h>
#include <stdint.h>

/** 版本号最大长度 */
constexpr int MAX_VERSION_LEN = 32;
/** 更新日志最大长度 */
constexpr int MAX_UPDATE_LOG_LEN = 512;
/** 包大小字符串最大长度 */
constexpr int MAX_PACKAGE_SIZE_LEN = 32;

/**
 * 版本信息
 */
struct VersionInfo {
    char currentVersion[MAX_VERSION_LEN];      /*!< 当前固件版本 */
    char latestVersion[MAX_VERSION_LEN];       /*!< 最新版本 */
    char updateLog[MAX_UPDATE_LOG_LEN];        /*!< 更新日志 */
    char packageSize[MAX_PACKAGE_SIZE_LEN];    /*!< 升级包大小 */
    bool updateAvailable;                      /*!< 是否有可用更新 */
    bool checkInProgress;                      /*!< 是否正在检查 */
};

/**
 * 版本源检查回调（新功能：注入真实检查逻辑，如 HTTP 查询）
 *
 * 回调内填写 info.latestVersion / info.updateLog / info.packageSize，
 * updateAvailable 由 VersionCheck 按版本号比较结果统一计算。
 */
using VersionSourceCb = void (*)(VersionInfo &info);

/**
 * 版本检查静态类
 *
 * 用法：
 *   VersionCheck::setCurrentVersion("v1.0.0");
 *   VersionCheck::setUpdateSource(myHttpCheck);   // 可选，不注入则用内置模拟数据
 *   VersionCheck::start();
 *   if (VersionCheck::hasUpdate()) { ... }
 */
class VersionCheck {
public:
    /** 初始化（预留，当前无状态需建立） */
    static void init();

    /** 反初始化（预留） */
    static void deinit();

    /**
     * @brief 开始检查（正在检查时忽略）
     *
     * 检查完成后按 latestVersion 与 currentVersion 比较结果更新 updateAvailable。
     */
    static void start();

    /** 中止检查 */
    static void stop();

    /** 获取版本信息（只读） */
    static const VersionInfo &info();

    /** 是否有可用更新 */
    static bool hasUpdate();

    /**
     * @brief 比较两个语义化版本号（支持 v/V 前缀，逐段数字比较）
     *
     * @return 1：v1 > v2；-1：v1 < v2；0：相等或参数无效
     */
    static int compareVersions(const char *v1, const char *v2);

    /** 设置当前固件版本（空参数忽略） */
    static void setCurrentVersion(const char *version);

    /** 注入版本源检查回调（nullptr 恢复内置模拟行为） */
    static void setUpdateSource(VersionSourceCb cb);

private:
    /** 内置模拟检查（保留原实现行为：最新 v1.2.0） */
    static void simulateCheck(VersionInfo &info);

    /** 安全拷贝（保证终止符） */
    static void safeCopy(char *dst, size_t dstSize, const char *src);

    static VersionInfo s_info;
    static VersionSourceCb s_updateSource;
};

#endif /* VERSION_CHECK_H */
