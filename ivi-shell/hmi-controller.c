/*
 * Copyright (C) 2013 DENSO CORPORATION
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <assert.h>
#include <time.h>

#include "compositor.h"
#include "weston-layout.h"
#include "hmi-controller.h"

#define PANEL_HEIGHT  70

/*****************************************************************************
 *  structure, globals
 ****************************************************************************/
struct hmi_controller_layer {
    struct weston_layout_layer  *ivilayer;
    uint32_t id_layer;

    uint32_t mode;

    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct link_layer {
    struct weston_layout_layer *layout_layer;
    struct wl_list link;
};

struct link_animation {
    struct hmi_controller_animation *animation;
    struct wl_list link;
};

struct hmi_controller_animation;
typedef void (*hmi_controller_animation_func)(struct hmi_controller_animation *animation);

struct hmi_controller_animation {
    void *data;
    float start;
    float end;
    struct weston_spring spring;
    uint32_t isFirstFrameDone;
    hmi_controller_animation_func frameFunc;
    hmi_controller_animation_func notifyFunc;
};

struct hmi_controller_fade {
    uint32_t isFadeIn;
    struct hmi_controller_animation *animation;
    struct wl_list layer_list;
};

struct hmi_controller_anima_event {
    struct wl_display       *display;
    struct wl_event_source  *event_source;
};

struct hmi_controller_layer g_DesktopLayer = {0};
struct hmi_controller_layer g_ApplicationLayer = {0};
struct hmi_controller_layer g_WorkSpaceBackGroundLayer = {0};
struct hmi_controller_layer g_WorkSpaceLayer = {0};
struct hmi_controller_layer g_CursorLayer = {0};
struct hmi_controller_setting g_HmiSetting = {0};

struct wl_list g_list_animation = {0};
struct hmi_controller_fade g_WorkSpaceFade = {0};
struct hmi_controller_anima_event  anima_event = {0};

/*****************************************************************************
 *  globals
 ****************************************************************************/

void
hmi_controller_set_background(uint32_t id_surface)
{
    struct weston_layout_surface *ivisurf = NULL;
    struct weston_layout_layer   *ivilayer = g_DesktopLayer.ivilayer;
    const uint32_t width  = g_DesktopLayer.width;
    const uint32_t height = g_DesktopLayer.height;
    uint32_t ret = 0;

    ivisurf = weston_layout_getSurfaceFromId(id_surface);
    assert(ivisurf != NULL);

    ret = weston_layout_layerAddSurface(ivilayer, ivisurf);
    assert(!ret);

    ret = weston_layout_surfaceSetDestinationRectangle(ivisurf,
                                    0, 0, width, height);
    assert(!ret);

    ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
    assert(!ret);

    weston_layout_commitChanges();
}

void
hmi_controller_set_panel(uint32_t id_surface)
{
    struct weston_layout_surface *ivisurf  = NULL;
    struct weston_layout_layer   *ivilayer = g_DesktopLayer.ivilayer;
    const uint32_t width  = g_DesktopLayer.width;
    uint32_t ret = 0;

    ivisurf = weston_layout_getSurfaceFromId(id_surface);
    assert(ivisurf != NULL);

    ret = weston_layout_layerAddSurface(ivilayer, ivisurf);
    assert(!ret);

    ret = weston_layout_surfaceSetDestinationRectangle(ivisurf,
                                    0, 0, width, PANEL_HEIGHT);
    assert(!ret);

    ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
    assert(!ret);

    weston_layout_commitChanges();
}

void
hmi_controller_set_button(uint32_t id_surface, uint32_t number)
{
    struct weston_layout_surface *ivisurf  = NULL;
    struct weston_layout_layer   *ivilayer = g_DesktopLayer.ivilayer;
    const uint32_t width  = 48;
    const uint32_t height = 48;
    uint32_t ret = 0;

    ivisurf = weston_layout_getSurfaceFromId(id_surface);
    assert(ivisurf != NULL);

    ret = weston_layout_layerAddSurface(ivilayer, ivisurf);
    assert(!ret);

    ret = weston_layout_surfaceSetDestinationRectangle(ivisurf,
                                    ((60 * number) + 15), 5, width, height);
    assert(!ret);

    ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
    assert(!ret);

    weston_layout_commitChanges();
}

void
hmi_controller_set_home_button(uint32_t id_surface)
{
    struct weston_layout_surface *ivisurf  = NULL;
    struct weston_layout_layer   *ivilayer = g_DesktopLayer.ivilayer;
    const uint32_t width  = g_DesktopLayer.width;
    uint32_t ret = 0;

    ivisurf = weston_layout_getSurfaceFromId(id_surface);
    assert(ivisurf != NULL);

    ret = weston_layout_layerAddSurface(ivilayer, ivisurf);
    assert(!ret);

    ret = weston_layout_surfaceSetDestinationRectangle(ivisurf,
                                   width / 2 - 24, 5, 48, 48);
    assert(!ret);

    ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
    assert(!ret);

    weston_layout_commitChanges();
}

void
hmi_controller_set_workspacebackground(uint32_t id_surface)
{
    struct weston_layout_surface *ivisurf = NULL;
    struct weston_layout_layer   *ivilayer = NULL;
    ivilayer = g_WorkSpaceBackGroundLayer.ivilayer;

    const uint32_t width  = g_WorkSpaceBackGroundLayer.width;
    const uint32_t height = g_WorkSpaceBackGroundLayer.height;
    uint32_t ret = 0;

    ivisurf = weston_layout_getSurfaceFromId(id_surface);
    assert(ivisurf != NULL);

    ret = weston_layout_layerAddSurface(ivilayer, ivisurf);
    assert(!ret);

    ret = weston_layout_surfaceSetDestinationRectangle(ivisurf,
                                    0, 0, width, height);
    assert(!ret);

    ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
    assert(!ret);

    weston_layout_commitChanges();
}

void
hmi_controller_set_launcher(struct wl_array *surface_ids)
{
    uint32_t size = 256;
    uint32_t ox = 0;
    uint32_t oy = 0;
    uint32_t space = 0;

    uint32_t vsize = size + space;
    uint32_t count_x = (uint32_t)((g_DesktopLayer.width - ox) / vsize);

    if (0 == count_x) {
        count_x = 1;
    }

    uint32_t* surface_id = NULL;
    uint32_t nx = 0;
    uint32_t ny = 0;

    wl_array_for_each(surface_id, surface_ids) {

        uint32_t x = nx * vsize + ox;
        uint32_t y = ny * vsize + oy;

        struct weston_layout_surface* layout_surface = NULL;

        layout_surface = weston_layout_getSurfaceFromId(*surface_id);
        assert(layout_surface);

        uint32_t ret = 0;
        ret = weston_layout_layerAddSurface(g_WorkSpaceLayer.ivilayer, layout_surface);
        assert(!ret);

        ret = weston_layout_surfaceSetDestinationRectangle(
                        layout_surface, x, y, size, size);
        assert(!ret);

        ret = weston_layout_surfaceSetVisibility(layout_surface, 1);
        assert(!ret);

        if (count_x - 1 == nx ) {
            ny++;
            nx = 0;
        } else {
            nx++;
        }
    }

    weston_layout_commitChanges();
}

/*****************************************************************************
 *  local functions
 ****************************************************************************/
static int32_t
is_surf_in_desktopWidget(struct weston_layout_surface *ivisurf)
{
    uint32_t id = weston_layout_getIdOfSurface(ivisurf);
    if (id == g_HmiSetting.background.id ||
        id == g_HmiSetting.panel.id ||
        id == g_HmiSetting.tiling.id ||
        id == g_HmiSetting.sidebyside.id ||
        id == g_HmiSetting.fullscreen.id ||
        id == g_HmiSetting.random.id ||
        id == g_HmiSetting.home.id ||
        id == g_HmiSetting.workspace_background.id) {
        return 1;
    }

    struct hmi_controller_launcher *launcher = NULL;
    wl_list_for_each(launcher, &g_HmiSetting.launcher_list, link)
    {
        if (id == launcher->id) {
            return 1;
        }
    }

    return 0;
}

static void
mode_divided_into_eight(weston_layout_surface_ptr *ppSurface, uint32_t surface_length,
                        struct hmi_controller_layer *layer)
{
    const float surface_width  = (float)layer->width * 0.25;
    const float surface_height = (float)layer->height * 0.5;
    float surface_x = 0.0f;
    float surface_y = 0.0f;
    struct weston_layout_surface *ivisurf  = NULL;
    int32_t ret = 0;

    uint32_t i = 0;
    uint32_t num = 1;
    for (i = 0; i < surface_length; i++) {
        ivisurf = ppSurface[i];

        /* skip desktop widgets */
        if (is_surf_in_desktopWidget(ivisurf)) {
            continue;
        }

        if (num <= 8) {
            if (num < 5) {
                surface_x = (num - 1) * (surface_width);
                surface_y = 0.0f;
            }
            else {
                surface_x = (num - 5) * (surface_width);
                surface_y = surface_height;
            }
            ret = weston_layout_surfaceSetDestinationRectangle(ivisurf, surface_x, surface_y,
                                                            surface_width, surface_height);
            assert(!ret);

            ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
            assert(!ret);

            num++;
            continue;
        }

        ret = weston_layout_surfaceSetVisibility(ivisurf, 0);
        assert(!ret);
    }
}

static void
mode_divided_into_two(weston_layout_surface_ptr *ppSurface, uint32_t surface_length,
                      struct hmi_controller_layer *layer)
{
    float surface_width  = (float)layer->width * 0.5f;
    float surface_height = (float)layer->height;
    struct weston_layout_surface *ivisurf  = NULL;
    int32_t ret = 0;

    uint32_t i = 0;
    uint32_t num = 1;
    for (i = 0; i < surface_length; i++) {
        ivisurf = ppSurface[i];

        /* skip desktop widgets */
        if (is_surf_in_desktopWidget(ivisurf)) {
            continue;
        }

        if (num == 1) {
            ret = weston_layout_surfaceSetDestinationRectangle(ivisurf, 0, 0,
                                                surface_width, surface_height);
            assert(!ret);

            ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
            assert(!ret);

            num++;
            continue;
        }
        else if (num == 2) {
            ret = weston_layout_surfaceSetDestinationRectangle(ivisurf, surface_width, 0,
                                                surface_width, surface_height);
            assert(!ret);

            ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
            assert(!ret);

            num++;
            continue;
        }

        weston_layout_surfaceSetVisibility(ivisurf, 0);
        assert(!ret);
    }
}

static void
mode_maximize_someone(weston_layout_surface_ptr *ppSurface, uint32_t surface_length,
                      struct hmi_controller_layer *layer)
{
    const float surface_width  = (float)layer->width;
    const float surface_height = (float)layer->height;
    struct weston_layout_surface *ivisurf  = NULL;
    int32_t ret = 0;

    uint32_t i = 0;
    for (i = 0; i < surface_length; i++) {
        ivisurf = ppSurface[i];

        /* skip desktop widgets */
        if (is_surf_in_desktopWidget(ivisurf)) {
            continue;
        }

        ret = weston_layout_surfaceSetDestinationRectangle(ivisurf, 0, 0,
                                            surface_width, surface_height);
        assert(!ret);

        ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
        assert(!ret);
    }
}

static void
mode_random_replace(weston_layout_surface_ptr *ppSurface, uint32_t surface_length,
                    struct hmi_controller_layer *layer)
{
    const float surface_width  = (float)layer->width * 0.25f;
    const float surface_height = (float)layer->height * 0.25;
    float surface_x = 0.0f;
    float surface_y = 0.0f;
    struct weston_layout_surface *ivisurf  = NULL;
    int32_t ret = 0;

    uint32_t i = 0;
    for (i = 0; i < surface_length; i++) {
        ivisurf = ppSurface[i];

        /* skip desktop widgets */
        if (is_surf_in_desktopWidget(ivisurf)) {
            continue;
        }

        surface_x = rand() % 800;
        surface_y = rand() % 500;

        ret = weston_layout_surfaceSetDestinationRectangle(ivisurf, surface_x, surface_y,
                                                  surface_width, surface_height);
        assert(!ret);

        ret = weston_layout_surfaceSetVisibility(ivisurf, 1);
        assert(!ret);
    }
}

static int32_t
has_applicatipn_surface(weston_layout_surface_ptr *ppSurface,
                        uint32_t surface_length)
{
    struct weston_layout_surface *ivisurf  = NULL;
    uint32_t i = 0;

    for (i = 0; i < surface_length; i++) {
        ivisurf = ppSurface[i];

        /* skip desktop widgets */
        if (is_surf_in_desktopWidget(ivisurf)) {
            continue;
        }

        return 1;
    }

    return 0;
}

static void
switch_mode(uint32_t id_surface)
{
    struct hmi_controller_layer *layer = &g_ApplicationLayer;
    weston_layout_surface_ptr  *ppSurface = NULL;
    uint32_t surface_length = 0;
    int32_t ret = 0;

    if (id_surface != g_HmiSetting.random.id &&
        id_surface != g_HmiSetting.tiling.id  &&
        id_surface != g_HmiSetting.fullscreen.id &&
        id_surface != g_HmiSetting.sidebyside.id) {
        return;
    }

    layer->mode = id_surface;

    ret = weston_layout_getSurfaces(&surface_length, &ppSurface);
    assert(!ret);

    if (!has_applicatipn_surface(ppSurface, surface_length)) {
        free(ppSurface);
        ppSurface = NULL;
        return;
    }

    do {
        if (id_surface == g_HmiSetting.tiling.id) {
            mode_divided_into_eight(ppSurface, surface_length, layer);
            break;
        }
        if (id_surface == g_HmiSetting.sidebyside.id) {
            mode_divided_into_two(ppSurface, surface_length, layer);
            break;
        }
        if (id_surface == g_HmiSetting.fullscreen.id) {
            mode_maximize_someone(ppSurface, surface_length, layer);
            break;
        }
        if (id_surface == g_HmiSetting.random.id) {
            mode_random_replace(ppSurface, surface_length, layer);
            break;
        }
    } while(0);

    weston_layout_commitChanges();

    free(ppSurface);
    ppSurface = NULL;

    return;
}

static void
hmi_controller_add_animation(struct hmi_controller_animation *animation)
{
    struct link_animation *link_anima = NULL;

    link_anima = calloc(1, sizeof(*link_anima));
    if (NULL == link_anima) {
        return;
    }

    link_anima->animation = animation;
    wl_list_insert(&g_list_animation, &link_anima->link);
    wl_event_source_timer_update(anima_event.event_source, 1);
}

static void
hmi_controller_remove_animation(struct hmi_controller_animation *animation)
{
    struct link_animation *link_animation = NULL;
    struct link_animation *next = NULL;

    wl_list_for_each_safe(link_animation, next, &g_list_animation, link) {
        if (link_animation->animation == animation) {
            wl_list_remove(&link_animation->link);
            free(link_animation);
            break;
        }
    }
}

static void
hmi_controller_animation_frame(struct hmi_controller_animation *animation, uint32_t timestamp)
{
    if (!animation->isFirstFrameDone) {
        animation->isFirstFrameDone = 1;
        animation->spring.timestamp = timestamp;
    }

    weston_spring_update(&animation->spring, timestamp);
    animation->notifyFunc(animation);
}

static void
hmi_controller_animation_destroy(struct hmi_controller_animation *animation)
{
    free(animation);
}

static struct hmi_controller_animation *
hmi_controller_animation_create(float start, float end,
                                hmi_controller_animation_func notifyFunc,
                                void *data)
{
    struct hmi_controller_animation *animation = NULL;

    animation = calloc(1, sizeof(*animation));
    if (!animation) {
        return NULL;
    }

    animation->start = start;
    animation->end = end;
    animation->notifyFunc = notifyFunc;
    animation->data = data;

    return animation;
}

static struct hmi_controller_animation *
hmi_controller_fade_animation_create(
    float start, float end, float k,
    hmi_controller_animation_func notifyFunc, void* data)
{
    struct hmi_controller_animation *animation = NULL;
    animation = hmi_controller_animation_create(start, end, notifyFunc, data);

    weston_spring_init(&animation->spring, k, start, end);
    animation->spring.friction = 1400;
    animation->spring.previous = -(end - start) * 0.03;

    return animation;
}

static float
hmi_controller_fade_animation_alpha_get(struct hmi_controller_animation *animation)
{
    if (animation->spring.current > 0.999) {
        return 1.0;
    } else if (animation->spring.current < 0.001 ) {
        return 0.0;
    } else {
        return animation->spring.current;
    }
}

static uint32_t
hmi_controller_is_animation_done(struct hmi_controller_animation *animation)
{
    return weston_spring_done(&animation->spring);
}

static void
hmi_controller_fade_update(struct hmi_controller_animation *animation, float end)
{
    animation->spring.target = end;
}

static void
hmi_controller_fade_anima_notify(struct hmi_controller_animation *animation)
{
    float alpha = hmi_controller_fade_animation_alpha_get(animation);
    alpha = wl_fixed_from_double(alpha);
    struct hmi_controller_fade *fade = animation->data;
    struct link_layer *linklayer = NULL;
    uint32_t isDone = hmi_controller_is_animation_done(animation);
    uint32_t isVisible = !isDone || fade->isFadeIn;

    wl_list_for_each(linklayer, &fade->layer_list, link) {
        weston_layout_layerSetOpacity(linklayer->layout_layer, alpha);
        weston_layout_layerSetVisibility(linklayer->layout_layer, isVisible);
    }

    if (isDone) {
        hmi_controller_remove_animation(animation);
        hmi_controller_animation_destroy(animation);
        fade->animation = NULL;
    }
}

static void
hmi_controller_fade_run(uint32_t isFadeIn, struct hmi_controller_fade *fade)
{
    float tint = isFadeIn ? 1.0 : 0.0;
    fade->isFadeIn = isFadeIn;

    if (fade->animation) {
        hmi_controller_fade_update(fade->animation, tint);
    } else {
        fade->animation = hmi_controller_fade_animation_create(
                                        1.0 - tint, tint, 300.0,
                                        hmi_controller_fade_anima_notify, fade);

        hmi_controller_add_animation(fade->animation);
    }
}

static void
home_button_event(uint32_t id_surface)
{
    if (id_surface != g_HmiSetting.home.id) {
        return;
    }

    uint32_t isFadeIn = !g_WorkSpaceFade.isFadeIn;
    hmi_controller_fade_run(isFadeIn, &g_WorkSpaceFade);
}

static void
create_layer(struct weston_layout_screen  *iviscrn,
             struct hmi_controller_layer *layer)
{
    int32_t ret = 0;

    layer->ivilayer = weston_layout_layerCreateWithDimension(layer->id_layer,
                                                layer->width, layer->height);
    assert(layer->ivilayer != NULL);

    ret = weston_layout_screenAddLayer(iviscrn, layer->ivilayer);
    assert(!ret);

    ret = weston_layout_layerSetDestinationRectangle(layer->ivilayer, layer->x, layer->y,
                                                  layer->width, layer->height);
    assert(!ret);

    ret = weston_layout_layerSetVisibility(layer->ivilayer, 1);
    assert(!ret);
}

static void
set_notification_create_surface(struct weston_layout_surface *ivisurf,
                                void *userdata)
{
    struct hmi_controller_layer *layer = &g_ApplicationLayer;
    int32_t ret = 0;
    (void)userdata;

    /* skip desktop widgets */
    if (is_surf_in_desktopWidget(ivisurf)) {
        return;
    }

    ret = weston_layout_layerAddSurface(layer->ivilayer, ivisurf);
    assert(!ret);
}

static void
set_notification_remove_surface(struct weston_layout_surface *ivisurf,
                                void *userdata)
{
    struct hmi_controller_layer *layer = &g_ApplicationLayer;
    (void)userdata;

    switch_mode(layer->mode);
}

static void
set_notification_configure_surface(struct weston_layout_surface *ivisurf,
                                void *userdata)
{
    struct hmi_controller_layer *layer = &g_ApplicationLayer;
    (void)ivisurf;
    (void)userdata;

    switch_mode(layer->mode);
}

static void
init_hmi_controllerSetting(struct hmi_controller_setting *setting)
{
    memset(setting, 0, sizeof(*setting));
    wl_list_init(&setting->workspace_list);
    wl_list_init(&setting->launcher_list);

    struct weston_config *config = NULL;
    config = weston_config_parse("weston.ini");

    struct weston_config_section *shellSection = NULL;
    shellSection = weston_config_get_section(config, "ivi-shell", NULL, NULL);

    weston_config_section_get_uint(
        shellSection, "desktop-layer-id", &setting->desktop_layer_id, 1000);

    weston_config_section_get_uint(
        shellSection, "workspace-background-layer-id", &setting->workspace_backgound_layer_id, 2000);

    weston_config_section_get_uint(
        shellSection, "workspace-layer-id", &setting->workspace_layer_id, 3000);

    weston_config_section_get_uint(
        shellSection, "application-layer-id", &setting->application_layer_id, 4000);

    weston_config_section_get_uint(
        shellSection, "cursor-layer-id", &setting->cursor_layer_id, 5000);

    weston_config_section_get_string(
        shellSection, "background-image", &setting->background.filePath,
        DATADIR "/weston/background.png");

    weston_config_section_get_uint(
        shellSection, "background-id", &setting->background.id, 1001);

    weston_config_section_get_string(
        shellSection, "panel-image", &setting->panel.filePath,
        DATADIR "/weston/panel.png");

    weston_config_section_get_uint(
        shellSection, "panel-id", &setting->panel.id, 1002);

    weston_config_section_get_string(
        shellSection, "tiling-image", &setting->tiling.filePath,
        DATADIR "/weston/tiling.png");

    weston_config_section_get_uint(
        shellSection, "tiling-id", &setting->tiling.id, 1003);

    weston_config_section_get_string(
        shellSection, "sidebyside-image", &setting->sidebyside.filePath,
        DATADIR "/weston/sidebyside.png");

    weston_config_section_get_uint(
        shellSection, "sidebyside-id", &setting->sidebyside.id, 1004);

    weston_config_section_get_string(
        shellSection, "fullscreen-image", &setting->fullscreen.filePath,
        DATADIR "/weston/fullscreen.png");

    weston_config_section_get_uint(
        shellSection, "fullscreen-id", &setting->fullscreen.id, 1005);

    weston_config_section_get_string(
        shellSection, "random-image", &setting->random.filePath,
        DATADIR "/weston/random.png");

    weston_config_section_get_uint(
        shellSection, "random-id", &setting->random.id, 1006);

    weston_config_section_get_string(
        shellSection, "home-image", &setting->home.filePath,
        DATADIR "/weston/home.png");

    weston_config_section_get_uint(
        shellSection, "home-id", &setting->home.id, 1007);

    weston_config_section_get_string(
        shellSection, "cursor-image", &setting->cursor.filePath,
        DATADIR "/weston/cursor.png");

    weston_config_section_get_uint(
        shellSection, "cursor-id", &setting->cursor.id, 5001);

    weston_config_section_get_uint(
        shellSection, "workspace-background-color",
        &setting->workspace_background.color, 0x99000000);

    weston_config_section_get_uint(
        shellSection, "workspace-background-id",
        &setting->workspace_background.id, 2001);

    struct weston_config_section *section = NULL;
    const char *name = NULL;

    while (weston_config_next_section(config, &section, &name)) {

        if (0 == strcmp(name, "ivi-workspace")) {

        } else if (0 == strcmp(name, "ivi-launcher")) {

            struct hmi_controller_launcher *launcher = NULL;
            launcher = calloc(1, sizeof(*launcher));
            assert(launcher);
            wl_list_init(&launcher->link);

            weston_config_section_get_uint(section, "id", &launcher->id, 0);
            weston_config_section_get_string(section, "icon", &launcher->icon, NULL);
            weston_config_section_get_string(section, "path", &launcher->path, NULL);

            wl_list_insert(&setting->launcher_list, &launcher->link);
        }
    }

    weston_config_destroy(config);
}

static void
init_hmi_controller(void)
{
    weston_layout_screen_ptr    *ppScreen = NULL;
    struct weston_layout_screen *iviscrn  = NULL;
    uint32_t screen_length  = 0;
    uint32_t screen_width   = 0;
    uint32_t screen_height  = 0;
    int32_t ret = 0;
    struct link_layer *tmp_link_layer = NULL;

    init_hmi_controllerSetting(&g_HmiSetting);

    weston_layout_getScreens(&screen_length, &ppScreen);

    iviscrn = ppScreen[0];

    weston_layout_getScreenResolution(iviscrn, &screen_width, &screen_height);
    assert(!ret);

    /* init desktop layer*/
    g_DesktopLayer.x = 0;
    g_DesktopLayer.y = 0;
    g_DesktopLayer.width  = screen_width;
    g_DesktopLayer.height = screen_height;
    g_DesktopLayer.id_layer = g_HmiSetting.desktop_layer_id;

    create_layer(iviscrn, &g_DesktopLayer);

    /* init application layer */
    g_ApplicationLayer.x = 0;
    g_ApplicationLayer.y = PANEL_HEIGHT;
    g_ApplicationLayer.width  = screen_width;
    g_ApplicationLayer.height = screen_height - PANEL_HEIGHT;
    g_ApplicationLayer.id_layer = g_HmiSetting.application_layer_id;
    g_ApplicationLayer.mode = g_HmiSetting.tiling.id;

    create_layer(iviscrn, &g_ApplicationLayer);

    /* init workspace backgound layer */
    g_WorkSpaceBackGroundLayer.x = 0;
    g_WorkSpaceBackGroundLayer.y = PANEL_HEIGHT;
    g_WorkSpaceBackGroundLayer.width = screen_width;
    g_WorkSpaceBackGroundLayer.height = screen_height - PANEL_HEIGHT;

    g_WorkSpaceBackGroundLayer.id_layer =
            g_HmiSetting.workspace_backgound_layer_id;

    create_layer(iviscrn, &g_WorkSpaceBackGroundLayer);
    weston_layout_layerSetOpacity(g_WorkSpaceBackGroundLayer.ivilayer, 0);
    weston_layout_layerSetVisibility(g_WorkSpaceBackGroundLayer.ivilayer, 0);

    /* init workspace layer */
    g_WorkSpaceLayer.x = 0;
    g_WorkSpaceLayer.y = PANEL_HEIGHT;
    g_WorkSpaceLayer.width = screen_width;
    g_WorkSpaceLayer.height = screen_height - PANEL_HEIGHT;
    g_WorkSpaceLayer.id_layer = g_HmiSetting.workspace_layer_id;
    create_layer(iviscrn, &g_WorkSpaceLayer);
    weston_layout_layerSetOpacity(g_WorkSpaceLayer.ivilayer, 0);
    weston_layout_layerSetVisibility(g_WorkSpaceLayer.ivilayer, 0);

    /* init work space fade */
    wl_list_init(&g_WorkSpaceFade.layer_list);
    tmp_link_layer = calloc(1, sizeof(*tmp_link_layer));
    tmp_link_layer->layout_layer = g_WorkSpaceLayer.ivilayer;
    wl_list_insert(&g_WorkSpaceFade.layer_list, &tmp_link_layer->link);
    tmp_link_layer = calloc(1, sizeof(*tmp_link_layer));
    tmp_link_layer->layout_layer = g_WorkSpaceBackGroundLayer.ivilayer;
    wl_list_insert(&g_WorkSpaceFade.layer_list, &tmp_link_layer->link);

    wl_list_init(&g_list_animation);

    weston_layout_setNotificationCreateSurface(set_notification_create_surface, NULL);
    weston_layout_setNotificationRemoveSurface(set_notification_remove_surface, NULL);
    weston_layout_setNotificationConfigureSurface(set_notification_configure_surface, NULL);

    free(ppScreen);
    ppScreen = NULL;
}

static int
do_anima(void* data)
{
    struct hmi_controller_anima_event *event = data;
    uint32_t fps = 20;

    if (wl_list_empty(&g_list_animation)) {
        wl_event_source_timer_update(event->event_source, 0);
        return 1;
    }

    wl_event_source_timer_update(event->event_source, 1000 / fps);

    struct timespec timestamp = {0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    uint32_t msec = (1e+3 * timestamp.tv_sec + 1e-6 * timestamp.tv_nsec);

    struct link_animation *link_animation = NULL;
    struct link_animation *next = NULL;

    wl_list_for_each_safe(link_animation, next, &g_list_animation, link) {
        hmi_controller_animation_frame(link_animation->animation, msec);
    }

    weston_layout_commitChanges();
    return 1;
}

/*****************************************************************************
 *  exported functions
 ****************************************************************************/
WL_EXPORT void
pointer_button_event(uint32_t id_surface)
{
    home_button_event(id_surface);
    switch_mode(id_surface);
}

WL_EXPORT void
pointer_move_event(uint32_t surface_id, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    (void)surface_id;
    (void)time;
    (void)sx;
    (void)sy;
}

WL_EXPORT int
module_init(struct weston_compositor *ec,
            int *argc, char *argv[])
{
    printf("DEBUG >>>> module_init in hmi-controller\n");
    init_hmi_controller();
    hmi_client_start();

    anima_event.display = ec->wl_display;
    struct wl_event_loop *loop = wl_display_get_event_loop(ec->wl_display);
    anima_event.event_source = wl_event_loop_add_timer(loop, do_anima, &anima_event);
    wl_event_source_timer_update(anima_event.event_source, 0);

    return 0;
}
