/**
 * @file     settings_ui.h
 * @brief    设置界面组件 — C++ class
 *
 * 功能：设置主菜单（系统信息/网络/蓝牙/声音/显示/按键/桌面布局/恢复默认）
 *       与设置数据（音量/亮度/按键灵敏度/网络与蓝牙状态等）管理。
 * 修改：2026-11-21 由 C API（settings_ui_* 函数）重构为 SettingsUi 单例类；
 *       修复菜单/布局按钮 user_data 用 malloc 存整数的内存管理问题（改 intptr 直存）；
 *       修复菜单点击回调先删子页再读被删按钮 user_data 的 use-after-free；
 *       修复重复调用 show 重复创建页面的问题（已显示时忽略）；
 *       修复返回键在菜单页会删掉菜单留下空白页的问题（菜单/子页分开管理）；
 *       字符串拷贝统一加终止符防护
 */
#ifndef SETTINGS_UI_H
#define SETTINGS_UI_H

#include "lvgl.h"
#include <stdint.h>

/** 设置组件版本 */
#define SETTINGS_VERSION "1.0.0"

/**
 * 设置数据
 */
struct SettingsData {
    char projectVersion[32];    /*!< 项目版本 */
    char componentVersion[32];  /*!< 组件版本 */
    char ipAddress[16];         /*!< IP 地址 */
    char ssid[32];              /*!< WiFi 名称 */
    int wifiSignalLevel;        /*!< WiFi 信号等级 */
    bool wifiConnected;         /*!< WiFi 是否已连接 */
    bool bluetoothEnabled;      /*!< 蓝牙是否开启 */
    char bluetoothDeviceName[32]; /*!< 蓝牙设备名 */
    int volume;                 /*!< 音量 0-100 */
    int brightness;             /*!< 亮度 0-100 */
    int keySensitivity;         /*!< 按键灵敏度 0-100 */
};

/**
 * 设置界面单例
 *
 * 用法：
 *   SettingsUi::instance().show();               // 显示设置页
 *   SettingsUi::instance().setVolume(80);        // 读写设置数据
 *   SettingsUi::instance().updateWifiStatus("MyWiFi", "192.168.1.10", 3, true);
 */
class SettingsUi {
public:
    /** 获取单例 */
    static SettingsUi &instance();

    /** 初始化（预留，当前无状态需建立） */
    void init();

    /** 销毁设置页面（若已显示） */
    void deinit();

    /** 显示设置页（已显示时忽略，修复原实现重复创建） */
    void show();

    /** 隐藏设置页（同 deinit） */
    void hide();

    /** 设置页是否正在显示 */
    bool isShown() const { return m_page != nullptr; }

    /** 设置版本信息 */
    void setVersion(const char *projectVersion, const char *componentVersion);

    /** 更新 WiFi 状态 */
    void updateWifiStatus(const char *ssid, const char *ipAddress, int signalLevel, bool connected);

    /** 更新蓝牙状态 */
    void updateBluetoothStatus(bool enabled, const char *deviceName);

    int volume() const { return m_settings.volume; }
    int brightness() const { return m_settings.brightness; }
    int keySensitivity() const { return m_settings.keySensitivity; }

    /** 设置音量（自动钳位 0-100） */
    void setVolume(int volume);

    /** 设置亮度（自动钳位 0-100） */
    void setBrightness(int brightness);

    /** 设置按键灵敏度（自动钳位 0-100） */
    void setKeySensitivity(int sensitivity);

    /** 恢复默认设置（页面显示中会同步刷新） */
    void resetToDefaults();

    /** 只读访问完整设置数据 */
    const SettingsData &data() const { return m_settings; }

private:
    SettingsUi();

    static void safeCopy(char *dst, size_t dstSize, const char *src);
    static int clampPercent(int value);

    void buildPage();
    lv_obj_t *createSubpageContainer(lv_obj_t *parent, int padRow);
    void showMenu();

    void createBackBtn(lv_obj_t *parent);
    void createSystemInfoPage(lv_obj_t *parent);
    void createNetworkPage(lv_obj_t *parent);
    void createBluetoothPage(lv_obj_t *parent);
    void createSoundPage(lv_obj_t *parent);
    void createDisplayPage(lv_obj_t *parent);
    void createKeyboardPage(lv_obj_t *parent);
    void createDesktopPage(lv_obj_t *parent);

    static void backBtnCb(lv_event_t *e);
    static void menuItemClickCb(lv_event_t *e);
    static void layoutBtnClickCb(lv_event_t *e);
    static void volumeSliderCb(lv_event_t *e);
    static void brightnessSliderCb(lv_event_t *e);
    static void sensitivitySliderCb(lv_event_t *e);
    static void pageDeleteCb(lv_event_t *e);

    SettingsData m_settings;
    lv_obj_t *m_page = nullptr;    /*!< 设置页根对象（删除时自动置空） */
    lv_obj_t *m_menu = nullptr;    /*!< 菜单容器 */
    lv_obj_t *m_subpage = nullptr; /*!< 当前子页容器 */
};

#endif /* SETTINGS_UI_H */
