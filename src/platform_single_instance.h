#ifndef PLATFORM_SINGLE_INSTANCE_H
#define PLATFORM_SINGLE_INSTANCE_H

/* Returns 0 if this is the first instance, -1 if another is running */
int acquire_single_instance(void);

/* Release the single instance lock */
void release_single_instance(void);

#endif