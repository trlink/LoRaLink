//includes
//////////
#include <Arduino.h>
#include "CDBTaskQuery.h"



bool compareTaskQueryNode(byte *pTaskData, int nType, void *pStruct)
{
  //variables
  ///////////
  sDBTaskQueryNode *sTQN;
  sDBTaskQueryNode sCmp;

  if(nType == DBTASK_QUERYNODE)
  {
    sTQN = (sDBTaskQueryNode*)pStruct;
    memcpy(&sCmp, pTaskData, sizeof(sDBTaskQueryNode));

    if(sCmp.dwNodeToQuery == sTQN->dwNodeToQuery)
    {
      return true;
    };
  };

  return false;
};
