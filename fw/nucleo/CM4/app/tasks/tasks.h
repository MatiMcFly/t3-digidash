#ifndef TASKS_H
#define TASKS_H

void acquisition_task(void* params);
void conversion_task(void* params);
void filtering_task(void* params);
void publication_task(void* params);

#endif // TASKS_H
