/**
 * @file     ui_manager.cpp
 * @brief    UI 管理器实现
 *
 * 修改：2026-11-21 重构为 UiManager 单例；修复组/页码标签重复创建泄漏、
 *       user_data malloc 泄漏、清屏后指针悬空；notifyEvent 支持监听器分发
 */
#include "ui_manager.h"

#include <cstdio>

/** 布局参数表（与 DesktopLayout 枚举一一对应） */
static const DesktopLayoutInfo s_layoutInfo[(int)DesktopLayout::MAX] = {
    {2, 2, 180, 200, 120}, /* 2x2 */
    {2, 3, 180, 140, 100}, /* 2x3 */
    {3, 3, 120, 140, 80},  /* 3x3 */
    {4, 2, 100, 200, 70},  /* 4x2 */
    {4, 3, 100, 140, 65},  /* 4x3 */
};

UiManager &UiManager::instance()
{
    static UiManager inst;
    return inst;
}

const DesktopLayoutInfo &UiManager::layoutInfo(DesktopLayout layout)
{
    if (layout >= DesktopLayout::MAX) {
        layout = DesktopLayout::LAYOUT_4x3;
    }
    return s_layoutInfo[(int)layout];
}

void UiManager::init()
{
    /* 输入组只创建一次（修复原实现 returnToDesktop 重复 init 导致的泄漏） */
    if (m_group == nullptr) {
        m_group = lv_group_create();
    }
    buildDesktopUi();
}

void UiManager::buildDesktopUi()
{
    if (m_desktop != nullptr) {
        /* 已初始化：仅刷新图标 */
        updateDesktopIcons();
        return;
    }

    m_desktop = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_desktop, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(m_desktop, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(m_desktop, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(m_desktop, 20, 0);
    lv_obj_set_style_pad_hor(m_desktop, 30, 0);
    lv_obj_set_style_pad_row(m_desktop, 20, 0);
    /* 桌面被清屏删除时置空指针，避免悬空 */
    lv_obj_add_event_cb(m_desktop, desktopDeleteCb, LV_EVENT_DELETE, this);

    m_pageIndicator = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(m_pageIndicator, &lv_font_montserrat_14, 0);
    lv_obj_align(m_pageIndicator, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(m_pageIndicator, indicatorDeleteCb, LV_EVENT_DELETE, this);

    updateDesktopIcons();
}

void UiManager::registerApp(const AppDesc &app)
{
    if (m_appCount >= MAX_APP_COUNT) {
        return;
    }
    if (findApp(app.id) >= 0) {
        return; /* ID 重复 */
    }

    m_apps[m_appCount] = app;
    m_appCount++;

    updateDesktopIcons();
}

void UiManager::unregisterApp(AppId id)
{
    int idx = findApp(id);
    if (idx < 0) {
        return;
    }

    for (int j = idx; j < m_appCount - 1; j++) {
        m_apps[j] = m_apps[j + 1];
    }
    m_appCount--;

    if (m_currentPage >= totalPages()) {
        m_currentPage = LV_MAX(0, totalPages() - 1);
    }

    updateDesktopIcons();
}

int UiManager::findApp(AppId id) const
{
    for (int i = 0; i < m_appCount; i++) {
        if (m_apps[i].id == id) {
            return i;
        }
    }
    return -1;
}

int UiManager::visibleAppCount() const
{
    int count = 0;
    for (int i = 0; i < m_appCount; i++) {
        if (m_apps[i].visible) {
            count++;
        }
    }
    return count;
}

int UiManager::totalPages() const
{
    const DesktopLayoutInfo *info = &layoutInfo(m_layout);
    int apps_per_page = info->cols * info->rows;
    int visible_count = visibleAppCount();

    if (visible_count <= 0) {
        return 0;
    }

    return (visible_count + apps_per_page - 1) / apps_per_page;
}

void UiManager::setPage(int page)
{
    int total = totalPages();
    if (total <= 0) {
        m_currentPage = 0;
        return;
    }

    if (page < 0) {
        m_currentPage = 0;
    } else if (page >= total) {
        m_currentPage = total - 1;
    } else {
        m_currentPage = page;
    }

    updateDesktopIcons();
}

void UiManager::nextPage()
{
    int total = totalPages();
    if (total > 1 && m_currentPage < total - 1) {
        m_currentPage++;
        updateDesktopIcons();
        notifyEvent(UiEvent::PAGE_CHANGED);
    }
}

void UiManager::prevPage()
{
    if (m_currentPage > 0) {
        m_currentPage--;
        updateDesktopIcons();
        notifyEvent(UiEvent::PAGE_CHANGED);
    }
}

void UiManager::setLayout(DesktopLayout layout)
{
    if (layout >= DesktopLayout::MAX) {
        return;
    }

    m_layout = layout;
    m_currentPage = 0;
    updateDesktopIcons();
}

void UiManager::updatePageIndicator()
{
    if (m_pageIndicator == nullptr) {
        return;
    }

    int total = totalPages();
    if (total <= 1) {
        lv_label_set_text(m_pageIndicator, "");
        return;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%d/%d", m_currentPage + 1, total);
    lv_label_set_text(m_pageIndicator, buf);
}

void UiManager::updateDesktopIcons()
{
    if (m_desktop == nullptr) {
        return;
    }

    lv_obj_clean(m_desktop);

    const DesktopLayoutInfo *info = &layoutInfo(m_layout);
    int apps_per_page = info->cols * info->rows;
    int start_idx = m_currentPage * apps_per_page;

    int visible_idx = 0;
    for (int i = 0; i < m_appCount; i++) {
        if (!m_apps[i].visible) {
            continue;
        }

        if (visible_idx >= start_idx + apps_per_page) {
            break;
        }

        if (visible_idx >= start_idx) {
            lv_obj_t *btn = lv_btn_create(m_desktop);
            lv_obj_set_size(btn, info->btnWidth, info->btnHeight);
            lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            if (m_apps[i].iconType == IconType::IMAGE && m_apps[i].icon != nullptr) {
                lv_obj_t *img = lv_image_create(btn);
                lv_image_set_src(img, m_apps[i].icon);
                lv_obj_set_size(img, info->iconSize, info->iconSize);
                lv_obj_center(img);
            } else if (m_apps[i].iconType == IconType::SYMBOL && m_apps[i].icon != nullptr) {
                lv_obj_t *symbol_label = lv_label_create(btn);
                lv_label_set_text(symbol_label, (const char *)m_apps[i].icon);
                lv_obj_set_style_text_font(symbol_label, &lv_font_montserrat_14, 0);
                lv_obj_set_size(symbol_label, info->iconSize, info->iconSize);
                lv_obj_center(symbol_label);
            } else {
                lv_obj_t *icon_container = lv_obj_create(btn);
                lv_obj_set_size(icon_container, info->iconSize, info->iconSize);
            }

            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, m_apps[i].name != nullptr ? m_apps[i].name : "");
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

            /* app id 直接存指针值，无需 malloc（修复原实现内存管理问题） */
            lv_obj_set_user_data(btn, (void *)(intptr_t)m_apps[i].id);
            lv_obj_add_event_cb(btn, appIconClickCb, LV_EVENT_CLICKED, nullptr);

            if (m_group != nullptr) {
                lv_group_add_obj(m_group, btn);
            }
        }

        visible_idx++;
    }

    updatePageIndicator();
}

void UiManager::appIconClickCb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    AppId id = (AppId)(intptr_t)lv_obj_get_user_data(btn);
    UiManager::instance().launchApp(id);
}

void UiManager::desktopDeleteCb(lv_event_t *e)
{
    UiManager *self = (UiManager *)lv_event_get_user_data(e);
    self->m_desktop = nullptr;
}

void UiManager::indicatorDeleteCb(lv_event_t *e)
{
    UiManager *self = (UiManager *)lv_event_get_user_data(e);
    self->m_pageIndicator = nullptr;
}

void UiManager::launchApp(AppId id)
{
    int idx = findApp(id);
    if (idx < 0 || m_apps[idx].launchCb == nullptr) {
        return;
    }

    m_currentAppId = id;
    lv_obj_clean(lv_scr_act()); /* 删除桌面与页码标签，删除回调会置空指针 */
    m_currentPageObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_currentPageObj, LV_HOR_RES, LV_VER_RES);
    m_apps[idx].launchCb();
}

void UiManager::returnToDesktop()
{
    lv_obj_clean(lv_scr_act());
    m_currentPageObj = nullptr;
    m_currentAppId = 0;
    buildDesktopUi(); /* 组不重复创建，仅重建桌面（修复原实现泄漏） */
}

void UiManager::notifyEvent(UiEvent event)
{
    for (int i = 0; i < m_listenerCount; i++) {
        if (m_listeners[i].cb != nullptr) {
            m_listeners[i].cb(event, m_listeners[i].userData);
        }
    }
}

bool UiManager::addEventListener(UiEventCb cb, void *userData)
{
    if (cb == nullptr || m_listenerCount >= MAX_EVENT_LISTENERS) {
        return false;
    }
    m_listeners[m_listenerCount].cb = cb;
    m_listeners[m_listenerCount].userData = userData;
    m_listenerCount++;
    return true;
}

bool UiManager::isAppRegistered(AppId id) const
{
    return findApp(id) >= 0;
}

bool UiManager::isAppVisible(AppId id) const
{
    int idx = findApp(id);
    return idx >= 0 && m_apps[idx].visible;
}

void UiManager::setAppVisible(AppId id, bool visible)
{
    int idx = findApp(id);
    if (idx < 0) {
        return;
    }

    m_apps[idx].visible = visible;

    if (!visible && m_currentPage >= totalPages()) {
        m_currentPage = LV_MAX(0, totalPages() - 1);
    }

    updateDesktopIcons();
}
