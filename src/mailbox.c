#include "mailbox.h"

size_t mailbox_depth(const Actor *a) {
    return a ? a->mailbox_len : 0;
}
