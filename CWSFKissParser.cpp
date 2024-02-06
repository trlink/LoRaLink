//includes
//////////
#include "CWSFKissParser.h"



CWSFKissParser::CWSFKissParser(tOnKissPacketComplete OnPacketComplete)
{
  this->m_pszPacket               = new char[MAX_KISS_PACKET_LEN + 1];
  this->m_nPos                    = 0;
  this->m_bStart                  = false;
  this->m_bEscape                 = false;
  this->m_cbOnKissPacketComplete  = OnPacketComplete;
};

CWSFKissParser::~CWSFKissParser()
{
  delete this->m_pszPacket;
};


bool CWSFKissParser::addData(char cData)
{
  char szData[2];

  szData[0] = cData;

  return this->addData((char*)&szData, 1);
};

bool CWSFKissParser::addData(char *pszData, int nLength)
{
  for(int n = 0; n < nLength; ++n)
  {
    switch(pszData[n])
    {
      case 0xDB: //ESC
      {
        if(this->m_bEscape == false)
        {
          this->m_bEscape = true;
        }
        else
        {
          if(this->m_nPos < MAX_KISS_PACKET_LEN)
          {
            this->m_pszPacket[this->m_nPos] = pszData[n];
            this->m_nPos += 1;
          };

          this->m_bEscape = false;
        };
      };
      break;
      
      case 0xC0: //FRAME_END
      {
        if(this->m_bEscape == false)
        {
          if(this->m_nPos == 0)
          {
            this->m_bStart = true;
          }
          else
          {
            this->m_cbOnKissPacketComplete(this->m_pszPacket, this->m_nPos);

            this->m_nPos   = 0;
            this->m_bStart = false;
          };
        }
        else
        {
          if(this->m_nPos < MAX_KISS_PACKET_LEN)
          {
            this->m_pszPacket[this->m_nPos] = pszData[n];
            this->m_nPos += 1;
          };
        };
      };
      break;

      default:
      {
        if(this->m_nPos < MAX_KISS_PACKET_LEN)
        {
          this->m_pszPacket[this->m_nPos] = pszData[n];
          this->m_nPos += 1;
        };
      };
    };
  };

  return true;
};
    


int CWSFKissParser::getKissPacket(char *pszData, int nLength, char *pszPacket)
{
  int nPos = 0;

  pszPacket[nPos] = 0xC0;
  nPos += 1;
        
  for(int n = 0; n < nLength; ++n)
  {
    switch(pszData[n])
    {
      case 0xDB: //ESC
      {
        pszPacket[nPos] = 0xDB;
        nPos += 1;
        
        pszPacket[nPos] = pszData[n];
        nPos += 1;
      };
      break;
      
      case 0xC0: //FRAME_END
      {
        pszPacket[nPos] = 0xDB;
        nPos += 1;
        
        pszPacket[nPos] = pszData[n];
        nPos += 1;    
      };
      break;

      default:
      {
        pszPacket[nPos] = pszData[n];
        nPos += 1;
      };
    };
  };

  pszPacket[nPos] = 0xC0;
  nPos += 1;

  return nPos;
};
