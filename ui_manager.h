/**
 * @file     ui_manager.h
 * @brief    UI 管理器（桌面应用启动器）— C++ class
 *
 * 功能：应用注册/启动、桌面图标网格（分页）、桌面布局切换、页码指示、
 *       UI 事件通知（复位触发/翻页）。
 * 修改：2026-11-21 由 C API（ui_manager_* 函数）重构为 UiManager 单例类；
 *       修复 returnToDesktop 重复调用 init 导致 lv_group 与页码标签重复创建泄漏；
 *       修复图标按钮 user_data 用 malloc 存 app_id 的内存管理问题（改 intptr 直存）；
 *       修复 launchApp 清屏后桌面指针悬空（挂删除事件回调置空）；
 *       notifyEvent 由空实现改为可注册监听器分发；
 *       lv_img_* 弃用 API 换为 lv_image_*
 */
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h"
#include <stdint.h>

/** 应用 ID 类型 */
using AppId = uint32_t;

/**
 * 图标类型
 */
enum class IconType : uint8_t {
    NONE = 0,   /*!< 无图标（占位容器） */
    IMAGE,      /*!< 图片图标（icon 指向 LVGL 图片源） */
    SYMBOL,     /*!< 符号图标（icon 指向 LV_SYMBOL_* 字符串） */
};

/**
 * 桌面布局
 */
enum class DesktopLayout : uint8_t {
    LAYOUT_2x2 = 0,
    LAYOUT_2x3,
    LAYOUT_3x3,
    LAYOUT_4x2,
    LAYOUT_4x3,
    MAX,
};

/**
 * 桌面布局参数
 */
struct DesktopLayoutInfo {
    int cols;       /*!< 列数 */
    int rows;       /*!< 行数 */
    int btnWidth;   /*!< 图标按钮宽 */
    int btnHeight;  /*!< 图标按钮高 */
    int iconSize;   /*!< 图标尺寸 */
};

/**
 * 应用描述
 */
struct AppDesc {
    AppId id = 0;                       /*!< 应用 ID（唯一） */
    const char *name = nullptr;         /*!< 应用名（显示在图标下方） */
    IconType iconType = IconType::NONE; /*!< 图标类型 */
    const void *icon = nullptr;         /*!< 图标数据（图片源或符号字符串） */
    void (*launchCb)(void) = nullptr;   /*!< 启动回调（负责构建应用页面） */
    bool visible = true;                /*!< 是否显示在桌面 */
};

/**
 * UI 事件
 */
enum class UiEvent : uint32_t {
    RESET_TRIGGERED = 0x8000, /*!< 恢复出厂设置触发 */
    PAGE_CHANGED,             /*!< 桌面翻页 */
};

/** UI 事件监听回调 */
using UiEventCb = void (*)(UiEvent event, void *userData);

/**
 * UI 管理器单例
 *
 * 用法：
 *   UiManager::instance().init();
 *   UiManager::instance().registerApp({.id = 1, .name = "设置", ...});
 *   UiManager::instance().launchApp(1);
 *   UiManager::instance().returnToDesktop();
 */
class UiManager {
public:
    static constexpr int MAX_APP_COUNT = 30;       /*!< 最大注册应用数 */
    static constexpr int MAX_EVENT_LISTENERS = 8;  /*!< 最大事件监听器数 */

    /** 获取单例 */
    static UiManager &instance();

    /**
     * @brief 初始化桌面（创建桌面容器、输入组、页码指示）
     *
     * 重复调用安全：已初始化时仅刷新图标。
     */
    void init();

    /** 注册应用（ID 重复或已满时忽略），注册后自动刷新桌面 */
    void registerApp(const AppDesc &app);

    /** 注销应用并刷新桌面 */
    void unregisterApp(AppId id);

    /**
     * @brief 启动应用：清空当前屏幕，创建应用页面容器，调用应用启动回调
     *
     * 应用回调内应在 lv_scr_act() 上构建 UI（页面容器已创建好）。
     */
    void launchApp(AppId id);

    /** 返回桌面（清屏并重建桌面，输入组不重复创建） */
    void returnToDesktop();

    /** 广播 UI 事件给所有监听器 */
    void notifyEvent(UiEvent event);

    /** 注册事件监听器；返回是否成功（已满返回 false） */
    bool addEventListener(UiEventCb cb, void *userData = nullptr);

    /** 应用是否已注册 */
    bool isAppRegistered(AppId id) const;

    /** 应用是否可见 */
    bool isAppVisible(AppId id) const;

    /** 设置应用可见性并刷新桌面 */
    void setAppVisible(AppId id, bool visible);

    /** 当前正在启动/运行的应用 ID */
    AppId currentLaunchingAppId() const { return m_currentAppId; }

    /** 获取当前桌面布局 */
    DesktopLayout layout() const { return m_layout; }

    /** 设置桌面布局（回到第 0 页并刷新） */
    void setLayout(DesktopLayout layout);

    /** 当前页号（从 0 开始） */
    int currentPage() const { return m_currentPage; }

    /** 总页数（按可见应用数与布局容量计算） */
    int totalPages() const;

    /** 设置页号（自动钳位） */
    void setPage(int page);

    /** 下一页（有变化时广播 PAGE_CHANGED） */
    void nextPage();

    /** 上一页（有变化时广播 PAGE_CHANGED） */
    void prevPage();

    /** 获取布局参数表（只读） */
    static const DesktopLayoutInfo &layoutInfo(DesktopLayout layout);

private:
    UiManager() = default;

    void buildDesktopUi();
    void updateDesktopIcons();
    void updatePageIndicator();
    int visibleAppCount() const;
    int findApp(AppId id) const;

    static void appIconClickCb(lv_event_t *e);
    static void desktopDeleteCb(lv_event_t *e);
    static void indicatorDeleteCb(lv_event_t *e);

    AppDesc m_apps[MAX_APP_COUNT];
    int m_appCount = 0;
    lv_obj_t *m_desktop = nullptr;        /*!< 桌面容器（删除时自动置空） */
    lv_group_t *m_group = nullptr;        /*!< 输入组（仅创建一次） */
    lv_obj_t *m_currentPageObj = nullptr; /*!< 当前应用页面容器 */
    lv_obj_t *m_pageIndicator = nullptr;  /*!< 页码指示标签（删除时自动置空） */
    AppId m_currentAppId = 0;
    DesktopLayout m_layout = DesktopLayout::LAYOUT_4x3;
    int m_currentPage = 0;

    struct Listener {
        UiEventCb cb = nullptr;
        void *userData = nullptr;
    };
    Listener m_listeners[MAX_EVENT_LISTENERS];
    int m_listenerCount = 0;
};

#endif /* UI_MANAGER_H */
