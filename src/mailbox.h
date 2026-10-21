#ifndef MAILBOX_H
#define MAILBOX_H

#include "actor.h"

/* Public for tests or external inspection of mailbox depth. */
size_t mailbox_depth(const Actor *a);

#endif
