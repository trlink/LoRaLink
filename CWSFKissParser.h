#ifndef __CWSFKISSPARSER__
#define __CWSFKISSPARSER__

#define MAX_KISS_PACKET_LEN 2048
#define KISS_DEBUG          1


typedef void(*tOnKissPacketComplete)(char *pszData, int nPacketLength); 



class CWSFKissParser
{
  public:
    CWSFKissParser(tOnKissPacketComplete OnPacketComplete);
    ~CWSFKissParser();

    bool addData(char cData);
    bool addData(char *pszData, int nLength);

    bool packetIncomplete();
    void invalidatePacket();

  private:
    //variables
    ///////////
    tOnKissPacketComplete m_cbOnKissPacketComplete;
    int                   m_nPos;
    char                  *m_pszPacket;
    bool                  m_bEscape;
    bool                  m_bHaveStart;
};






#endif
