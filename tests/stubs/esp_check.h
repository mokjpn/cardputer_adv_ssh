#pragma once
#define ESP_RETURN_ON_ERROR(expr,...) do { auto rc_=(expr); if(rc_!=0) return rc_; } while(0)
