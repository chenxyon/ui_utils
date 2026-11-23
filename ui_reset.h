/**
 * @file     ui_reset.h
 * @brief    恢复出厂设置触发组件 — C++ class
 *
 * 功能：组合键长按检测，达到设定时长后恢复默认设置并广播复位事件。
 * 修改：2026-11-21 由 C API（ui_reset_* 函数）重构为 UiReset 静态类；
 *       修复 get_current_time_ms 恒返回 0 导致长按计时失效的问题
 *       （改用 esp_timer_get_time()/1000）；
 *       修复组合键检测恒返回 false 无法触发的问题（改为可注入回调）；
 *       新增触发锁存：触发后保持组合键不松开不会重复触发；
 *       新增 setHoldDuration 可配置长按时长
 */
#ifndef UI_RESET_H
#define UI_RESET_H

#include <stdint.h>

/**
 * 复位触发回调（长按达标并完成恢复默认后调用）
 */
using ResetTriggerCb = void (*)(void);

/**
 * 复位组合键检测回调；返回 true 表示组合键处于按下状态
 */
using ResetCombineCheckerCb = bool (*)(void);

/**
 * 恢复出厂设置触发静态类
 *
 * 用法：
 *   UiReset::setCombinationChecker([] { return 同时按下某几个键; });
 *   UiReset::init(onResetTriggered);
 *   // 在周期任务中调用：
 *   UiReset::check();
 */
class UiReset {
public:
    static constexpr uint32_t HOLD_DURATION_MS = 3000; /*!< 默认长按触发时长 */

    /**
     * @brief 初始化复位检测
     *
     * @param cb 触发回调（可为空）
     */
    static void init(ResetTriggerCb cb = nullptr);

    /** 反初始化（清除回调与状态） */
    static void deinit();

    /**
     * @brief 周期检查（应放在主循环或定时任务中周期调用）
     *
     * 组合键持续按下达到长按时长时：
     *   1. 调用 SettingsUi::instance().resetToDefaults() 恢复默认设置
     *   2. 调用触发回调
     *   3. 广播 UiEvent::RESET_TRIGGERED 事件
     * 触发后锁存，松开组合键前不会重复触发。
     */
    static void check();

    /** 是否已触发过（触发后保持，直到 clearTriggered） */
    static bool isTriggered();

    /** 清除触发标志 */
    static void clearTriggered();

    /** 注入组合键检测回调（不注入则恒为未按，与原行为一致） */
    static void setCombinationChecker(ResetCombineCheckerCb cb);

    /** 设置长按触发时长（0 视为无效，保持原值） */
    static void setHoldDuration(uint32_t ms);

    /** 获取当前长按触发时长 */
    static uint32_t holdDuration();

private:
    /** 当前时间（毫秒，esp_timer 提供） */
    static uint32_t nowMs();

    /** 设置持久化预留（当前为空实现，可接 NVS） */
    static void saveSettingsToFlash();

    static ResetTriggerCb s_triggerCb;
    static ResetCombineCheckerCb s_combinationChecker;
    static bool s_triggered;     /*!< 触发标志（锁存） */
    static bool s_holding;       /*!< 正在长按计时 */
    static bool s_latched;       /*!< 触发后锁存，等待松开组合键 */
    static uint32_t s_holdStart; /*!< 长按起始时间 */
    static uint32_t s_holdDurationMs;
};

#endif /* UI_RESET_H */
