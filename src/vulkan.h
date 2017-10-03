#pragma once

#define VK_CHECK_RESULT(op_)              \
    do                                        \
    {                                         \
        VkResult res_ = (op_);                \
        if (res_ != VK_SUCCESS) return false; \
    } while (false)
