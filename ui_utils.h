/**
 * @file     ui_utils.h
 * @brief    UI 工具组件 — C++ class
 *
 * 功能：WiFi 信号图标绘制（canvas）、中文标签创建、对象位置/尺寸设置等通用 UI 工具。
 * 修改：2026-11-21 由 C API（ui_* 函数）重构为 UiUtils 静态类；
 *       修复 canvas 缓冲尺寸未按 I1 行距对齐计算的问题（改用 lv_draw_buf_width_to_stride）；
 *       修复 canvas 缓冲存 user_data 后对象删除时永不释放的内存泄漏（挂删除事件）；
 *       缓冲改用 lv_malloc 分配，与 LVGL 内存管理一致
 */
#ifndef UI_UTILS_H
#define UI_UTILS_H

#include "lvgl.h"

/**
 * WiFi 信号强度等级
 */
enum class WifiSignalLevel : uint8_t {
    NONE = 0,   /*!< 无信号 */
    WEAK,       /*!< 弱（1 格） */
    MEDIUM,     /*!< 中（2 格） */
    STRONG,     /*!< 强（3 格） */
};

/**
 * UI 工具静态类
 *
 * 用法：
 *   lv_obj_t *icon = UiUtils::createWifiIcon(parent, 280, 5, 24, WifiSignalLevel::STRONG);
 *   UiUtils::updateWifiIcon(icon, WifiSignalLevel::WEAK);
 */
class UiUtils {
public:
    /**
     * @brief 在已有 canvas 上绘制 WiFi 信号图标
     *
     * @param canvas 已设置好缓冲的 canvas 对象
     * @param x      左上角 X（canvas 缓冲坐标系）
     * @param y      左上角 Y
     * @param size   图标边长
     * @param level  信号等级
     */
    static void drawWifiIcon(lv_obj_t *canvas, int x, int y, int size, WifiSignalLevel level);

    /**
     * @brief 创建 WiFi 信号图标（canvas 实现）
     *
     * 内部自动分配 I1 格式缓冲（行距对齐），并挂删除事件自动释放缓冲。
     *
     * @param parent 父对象
     * @param x      位置 X
     * @param y      位置 Y
     * @param size   图标边长
     * @param level  初始信号等级
     * @return canvas 对象；失败返回 nullptr
     */
    static lv_obj_t *createWifiIcon(lv_obj_t *parent, int x, int y, int size, WifiSignalLevel level);

    /**
     * @brief 更新 WiFi 信号图标（重绘）
     *
     * @param canvas createWifiIcon 创建的 canvas
     * @param level  新信号等级
     */
    static void updateWifiIcon(lv_obj_t *canvas, WifiSignalLevel level);

    /**
     * @brief 创建带字体与对齐的标签（常用于中文文本）
     *
     * @param parent   父对象
     * @param text     文本（可为空）
     * @param font     字体（可为空，用默认字体）
     * @param align    对齐方式
     * @param xOffset  X 偏移
     * @param yOffset  Y 偏移
     * @return 标签对象
     */
    static lv_obj_t *createChineseLabel(lv_obj_t *parent, const char *text,
                                        const lv_font_t *font, lv_align_t align,
                                        int xOffset, int yOffset);

    /** 设置标签文本（空参数防护） */
    static void setLabelText(lv_obj_t *label, const char *text);

    /** 设置标签字体（空参数防护） */
    static void setLabelFont(lv_obj_t *label, const lv_font_t *font);

    /** 设置标签对齐（空参数防护） */
    static void setLabelAlign(lv_obj_t *label, lv_align_t align, int xOffset, int yOffset);

    /** 设置对象位置（空参数防护） */
    static void setObjPos(lv_obj_t *obj, int x, int y);

    /** 设置对象尺寸（空参数防护） */
    static void setObjSize(lv_obj_t *obj, int width, int height);

private:
    /** canvas 删除事件回调：释放位图缓冲 */
    static void wifiIconDeleteCb(lv_event_t *e);
};

#endif /* UI_UTILS_H */
