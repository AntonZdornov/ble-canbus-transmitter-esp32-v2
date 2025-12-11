#include "battery_widget.h"

#define BAT_W 240      // общая ширина батареи
#define BAT_H 24       // высота батареи

static lv_obj_t *battery_label  = NULL;
static lv_obj_t *battery_canvas = NULL;
static lv_obj_t *battery_cont   = NULL;

static lv_color_t bat_canvas_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(BAT_W, BAT_H)];
static uint8_t current_level = 100;

// ------------------ цвет по уровню ------------------

static lv_color_t color_for_level(uint8_t percent)
{
    if (percent < 30) {
        return lv_color_hex(0xFF4040);   // красный
    } else if (percent < 45) {
        return lv_color_hex(0xFFD050);   // жёлтый
      } else if (percent < 80) {
        return lv_color_hex(0x7CFF7C);   // зеленый
    } else {
        return lv_color_hex(0x1d34ff);   // синий
    }
}

// ------------------ отрисовка батареи ------------------

static void draw_battery_level(uint8_t percent)
{
    if (!battery_canvas || !battery_label) return;
    if (percent > 100) percent = 100;

    // --- обновить текст ---
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", percent);
    lv_label_set_text(battery_label, buf);

    // --- очистить canvas (прозрачный фон) ---
    lv_canvas_fill_bg(battery_canvas, lv_color_black(), LV_OPA_TRANSP);

    // --- стиль контура (капсула) ---
    static lv_draw_rect_dsc_t border_dsc;
    static bool border_inited = false;
    if (!border_inited) {
        lv_draw_rect_dsc_init(&border_dsc);
        border_dsc.bg_opa       = LV_OPA_TRANSP;
        border_dsc.border_opa   = LV_OPA_COVER;
        border_dsc.border_width = 2;
        border_dsc.border_color = lv_color_hex(0x9e9e9e); // голубой контур
        border_dsc.radius       = BAT_H / 4;              // закруглённые края
        border_inited = true;
    }

    // нарисовать контур (капсулу)
    lv_canvas_draw_rect(
        battery_canvas,
        0,          // x
        0,          // y
        BAT_W,      // w
        BAT_H,      // h
        &border_dsc
    );

    // --- внутренняя заливка ---

    // сколько пикселей по ширине занимает процент
    int inner_w = (BAT_W - 10) * percent / 100;
    if (inner_w <= 0) return;   // пусто -> только контур

    // координаты внутренней области
    lv_coord_t x0 = 5;
    lv_coord_t y0 = 3;
    lv_coord_t h  = BAT_H - 6;  // сверху/снизу по 3px отступ

    // базовый цвет по уровню
    lv_color_t base      = color_for_level(percent);
    lv_color_t left_col  = lv_color_mix(base, lv_color_black(), 160); // левый (темнее)
    lv_color_t right_col = base;                                      // правый (ярче)

    // сначала рисуем однотонную заливку с закруглёнными углами
    lv_draw_rect_dsc_t fill_dsc;
    lv_draw_rect_dsc_init(&fill_dsc);
    fill_dsc.bg_opa  = LV_OPA_COVER;
    fill_dsc.bg_color = base;
    fill_dsc.radius  = h / 4;        // округление внутренней капсулы

    lv_canvas_draw_rect(
        battery_canvas,
        x0,
        y0,
        inner_w,
        h,
        &fill_dsc
    );

    // теперь сверху накладываем градиент полосками, не заходя в самые края,
    // чтобы скруглённые углы остались красивыми от базовой заливки
    lv_draw_rect_dsc_t grad_dsc;
    lv_draw_rect_dsc_init(&grad_dsc);
    grad_dsc.bg_opa = LV_OPA_COVER;
    grad_dsc.radius = 4;   // полоски без радиуса, края уже закруглил fill_dsc

    int start_x = x0 + 1;
    int end_x   = x0 + inner_w - 2;
    if (end_x <= start_x) return;

    int grad_width = end_x - start_x + 1;

    for (int i = 0; i < grad_width; i++) {
        // t: 0 слева, 255 справа
        uint8_t t = (grad_width > 1) ? (uint8_t)((i * 255) / (grad_width - 1)) : 0;
        grad_dsc.bg_color = lv_color_mix(right_col, left_col, 255 - t);

        lv_canvas_draw_rect(
            battery_canvas,
            start_x + i,  // x
            y0,           // y
            1,            // w
            h,            // h
            &grad_dsc
        );
    }
}

// ------------------ анимация ------------------

static void anim_exec_cb(void *var, int32_t v)
{
    LV_UNUSED(var);
    current_level = (uint8_t)v;
    draw_battery_level(current_level);
}

void battery_animate_to(uint8_t target, uint32_t ms)
{
    if (target > 100) target = 100;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, NULL);
    lv_anim_set_values(&a, current_level, target);
    lv_anim_set_time(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, anim_exec_cb);
    lv_anim_start(&a);
}

// ------------------ публичные функции ------------------

void battery_set_level(uint8_t percent) {
    if (percent > 100) percent = 100;
    current_level = percent;
    draw_battery_level(percent);
}

void battery_widget_create(lv_obj_t *parent)
{
    // контейнер под текст + батарею
    battery_cont = lv_obj_create(parent);
    lv_obj_set_size(battery_cont, BAT_W, 50);
    lv_obj_set_style_bg_opa(battery_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(battery_cont, 0, 0);
    lv_obj_clear_flag(battery_cont, LV_OBJ_FLAG_SCROLLABLE);

    // можно позиционировать как хочешь, пример — по центру
    lv_obj_center(battery_cont);

    // текст "100%"
    battery_label = lv_label_create(battery_cont);
    lv_label_set_text(battery_label, "100%");
    lv_obj_align(battery_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);

    // canvas для рисования батарейки
    battery_canvas = lv_canvas_create(battery_cont);
    lv_canvas_set_buffer(battery_canvas, bat_canvas_buf,
                         BAT_W, BAT_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(battery_canvas, LV_ALIGN_BOTTOM_MID, 0, 0);

    // стартовое состояние
    battery_set_level(current_level);
}
