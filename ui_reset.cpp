/**
 * @file     ui_reset.cpp
 * @brief    恢复出厂设置触发组件实现
 *
 * 修改：2026-11-21 重构为 UiReset 静态类；修复计时恒 0 与组合键检测恒 false 的问题；
 *       新增触发锁存与可配置长按时长
 */
#include "ui_reset.h"
#include "settings_ui.h"
#include "ui_manager.h"

#include "esp_timer.h"

ResetTriggerCb UiReset::s_triggerCb = nullptr;
ResetCombineCheckerCb UiReset::s_combinationChecker = nullptr;
bool UiReset::s_triggered = false;
bool UiReset::s_holding = false;
bool UiReset::s_latched = false;
uint32_t UiReset::s_holdStart = 0;
uint32_t UiReset::s_holdDurationMs = UiReset::HOLD_DURATION_MS;

uint32_t UiReset::nowMs() {
    /* 修复：原实现恒返回 0，长按计时永远达不到阈值 */
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void UiReset::saveSettingsToFlash() {
    /* 预留：设置持久化（可接 NVS），当前为空实现 */
}

void UiReset::init(ResetTriggerCb cb) {
    s_triggerCb = cb;
    s_triggered = false;
    s_holding = false;
    s_latched = false;
}

void UiReset::deinit() {
    s_triggerCb = nullptr;
    s_triggered = false;
    s_holding = false;
    s_latched = false;
}

void UiReset::check() {
    /* 修复：原组合键检测恒返回 false；现由外部注入真实检测 */
    bool pressed = (s_combinationChecker != nullptr) ? s_combinationChecker() : false;

    if (pressed) {
        /* 触发后锁存：保持按住不松开也不会重复触发 */
        if (s_latched) {
            return;
        }
        if (!s_holding) {
            s_holding = true;
            s_holdStart = nowMs();
        } else {
            uint32_t elapsed = nowMs() - s_holdStart;
            if (elapsed >= s_holdDurationMs) {
                s_triggered = true;
                s_holding = false;
                s_latched = true;

                SettingsUi::instance().resetToDefaults();
                saveSettingsToFlash();

                if (s_triggerCb != nullptr) {
                    s_triggerCb();
                }

                UiManager::instance().notifyEvent(UiEvent::RESET_TRIGGERED);
            }
        }
    } else {
        s_holding = false;
        s_latched = false;
    }
}

bool UiReset::isTriggered() {
    return s_triggered;
}

void UiReset::clearTriggered() {
    s_triggered = false;
}

void UiReset::setCombinationChecker(ResetCombineCheckerCb cb) {
    s_combinationChecker = cb;
}

void UiReset::setHoldDuration(uint32_t ms) {
    if (ms > 0) {
        s_holdDurationMs = ms;
    }
}

uint32_t UiReset::holdDuration() {
    return s_holdDurationMs;
}
