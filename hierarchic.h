#ifndef HIERARCHIC_H
#define HIERARCHIC_H

#include <menu.h>

int HierarchicStartUp(unsigned memID);
void HierarchicShutDown(void);
MenuRecHndl HierarchicNew(unsigned refDesc, Ref ref);
void HierarchicDispose(MenuRecHndl menu);


#endif
