/**
 * @file     ui_utils.cpp
 * @brief    UI 工具组件实现
 *
 * 修改：2026-11-21 重构为 UiUtils 静态类；
 *       canvas 缓冲按 I1 行距对齐分配（lv_draw_buf_width_to_stride），
 *       并经 LV_EVENT_DELETE 自动释放，消除原实现的内存泄漏
 */
#include "ui_utils.h"

#include <cstdlib>

/**
 * @brief WiFi 图标内部绘制（信号点 + 信号弧）
 */
static void drawWifiIconInternal(lv_obj_t *canvas, int x, int y, int size, WifiSignalLevel level)
{
    lv_color_t color = lv_color_white();

    int center_x = x + size / 2;
    int center_y = y + size / 2;

    const int arc_count = 3;      /* 信号弧数量 */
    const int start_angle = 180;
    const int end_angle = 0;
    const int arc_width = 2;

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    /* 黑色背景 */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_black();
    rect_dsc.bg_opa = LV_OPA_COVER;

    lv_area_t area;
    area.x1 = x;
    area.y1 = y;
    area.x2 = x + size - 1;
    area.y2 = y + size - 1;
    lv_draw_rect(&layer, &rect_dsc, &area);

    /* 信号源点（任何等级都画） */
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = color;
    arc_dsc.width = size / 4;
    arc_dsc.radius = size / 8;
    arc_dsc.start_angle = 0;
    arc_dsc.end_angle = 360;
    arc_dsc.center.x = center_x;
    arc_dsc.center.y = center_y;
    lv_draw_arc(&layer, &arc_dsc);

    /* 信号弧：等级越高弧越多 */
    for (int i = 0; i < arc_count; i++) {
        int radius = (size / 2) - (i * (size / (arc_count * 2)));

        if ((int)level > i) {
            lv_draw_arc_dsc_t arc_i_dsc;
            lv_draw_arc_dsc_init(&arc_i_dsc);
            arc_i_dsc.color = color;
            arc_i_dsc.width = arc_width;
            arc_i_dsc.radius = radius;
            arc_i_dsc.start_angle = start_angle;
            arc_i_dsc.end_angle = end_angle;
            arc_i_dsc.center.x = center_x;
            arc_i_dsc.center.y = center_y;
            lv_draw_arc(&layer, &arc_i_dsc);
        }
    }

    lv_canvas_finish_layer(canvas, &layer);
}

void UiUtils::drawWifiIcon(lv_obj_t *canvas, int x, int y, int size, WifiSignalLevel level)
{
    if (canvas == nullptr) {
        return;
    }
    drawWifiIconInternal(canvas, x, y, size, level);
}

lv_obj_t *UiUtils::createWifiIcon(lv_obj_t *parent, int x, int y, int size, WifiSignalLevel level)
{
    if (parent == nullptr || size <= 0) {
        return nullptr;
    }

    lv_obj_t *canvas = lv_canvas_create(parent);
    lv_obj_set_size(canvas, size, size);
    lv_obj_set_pos(canvas, x, y);

    /* I1 格式：每行 (size+7)/8 字节，再按 LVGL 要求对齐到 8 字节 */
    int32_t stride = lv_draw_buf_width_to_stride(size, 1);
    size_t buf_size = (size_t)stride * size;
    uint8_t *buf = (uint8_t *)lv_malloc(buf_size);
    if (buf == nullptr) {
        lv_obj_del(canvas);
        return nullptr;
    }

    lv_canvas_set_buffer(canvas, buf, size, size, LV_COLOR_FORMAT_I1);
    lv_obj_set_user_data(canvas, buf);
    /* 对象删除时自动释放缓冲（修复原实现泄漏） */
    lv_obj_add_event_cb(canvas, wifiIconDeleteCb, LV_EVENT_DELETE, buf);

    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    drawWifiIconInternal(canvas, 0, 0, size - 4, level);

    return canvas;
}

void UiUtils::updateWifiIcon(lv_obj_t *canvas, WifiSignalLevel level)
{
    if (canvas == nullptr) {
        return;
    }

    lv_coord_t size = lv_obj_get_width(canvas);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    drawWifiIconInternal(canvas, 0, 0, size - 4, level);
}

void UiUtils::wifiIconDeleteCb(lv_event_t *e)
{
    void *buf = lv_event_get_user_data(e);
    if (buf != nullptr) {
        lv_free(buf);
    }
}

lv_obj_t *UiUtils::createChineseLabel(lv_obj_t *parent, const char *text,
                                      const lv_font_t *font, lv_align_t align,
                                      int xOffset, int yOffset)
{
    if (parent == nullptr) {
        return nullptr;
    }

    lv_obj_t *label = lv_label_create(parent);

    if (text != nullptr) {
        lv_label_set_text(label, text);
    }

    if (font != nullptr) {
        lv_obj_set_style_text_font(label, font, 0);
    }

    lv_obj_align(label, align, xOffset, yOffset);

    return label;
}

void UiUtils::setLabelText(lv_obj_t *label, const char *text)
{
    if (label == nullptr || text == nullptr) {
        return;
    }
    lv_label_set_text(label, text);
}

void UiUtils::setLabelFont(lv_obj_t *label, const lv_font_t *font)
{
    if (label == nullptr || font == nullptr) {
        return;
    }
    lv_obj_set_style_text_font(label, font, 0);
}

void UiUtils::setLabelAlign(lv_obj_t *label, lv_align_t align, int xOffset, int yOffset)
{
    if (label == nullptr) {
        return;
    }
    lv_obj_align(label, align, xOffset, yOffset);
}

void UiUtils::setObjPos(lv_obj_t *obj, int x, int y)
{
    if (obj == nullptr) {
        return;
    }
    lv_obj_set_pos(obj, x, y);
}

void UiUtils::setObjSize(lv_obj_t *obj, int width, int height)
{
    if (obj == nullptr) {
        return;
    }
    lv_obj_set_size(obj, width, height);
}

#include "ui_utils_version.h"
#include "version_manager.h"
COMPONENT_VERSION_REGISTER(ui_utils, UI_UTILS_VERSION_STRING)
