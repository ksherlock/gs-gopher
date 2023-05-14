#ifndef HIERARCHIC_H
#define HIERARCHIC_H

#include <menu.h>

int HierarchicStartUp(unsigned memID);
void HierarchicShutDown(void);
MenuRecHndl HierarchicNew(unsigned refDesc, Ref ref);
MenuRecHndl HierarchicGetMenuHandle(MenuRecHndl parent, unsigned menuID);
void HierarchicDispose(MenuRecHndl menu);


#endif
