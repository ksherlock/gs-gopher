#ifndef Q_H
#define Q_H

void StartupQueue(void);
void ShutDownQueue(void);
void CancelQueue(void);
void ProcessQueue(void);
unsigned QueueURL(const char *cp, unsigned length);
unsigned QueueEntry(struct ListEntry *e, const char *query);

#endif
