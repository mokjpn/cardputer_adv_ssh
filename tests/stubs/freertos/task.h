#pragma once
inline unsigned test_ticks = 0;
inline void vTaskDelay(unsigned n) { test_ticks += n; }
inline unsigned xTaskGetTickCount() { return test_ticks; }
