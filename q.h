#ifndef Q_H
#define Q_H

void StartupQueue(void);
void ShutDownQueue(void);
void CancelQueue(void);
void ProcessQueue(void);
unsigned QueueURL(const char *cp, const char *query);
unsigned QueueEntry(struct ListEntry *e);

int AnalyzeURL(const char *cp);

#endif
