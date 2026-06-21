#ifndef _LV_PORT_H_
#define _LV_PORT_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool flush_in_progress;
    bool draw_buf_internal;
    bool bounce_available;
    uint16_t bounce_lines;
    uint32_t flush_count;
    uint32_t flush_wait_timeouts;
    uint32_t last_flush_start_ms;
    uint32_t last_flush_end_ms;
} display_flush_state_t;

void lv_port_init(void);
void lv_port_get_flush_state(display_flush_state_t *out_state);


#endif
