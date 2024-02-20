//includes
//////////
#include "CWSFKissParser.h"



CWSFKissParser::CWSFKissParser(tOnKissPacketComplete OnPacketComplete)
{
  this->m_pszPacket               = new char[MAX_KISS_PACKET_LEN + 1];
  this->m_nPos                    = 0;
  this->m_bEscape                 = false;
  this->m_cbOnKissPacketComplete  = OnPacketComplete;
  this->m_bHaveStart              = false;
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


bool CWSFKissParser::packetIncomplete()
{
  return this->m_bHaveStart;
};


void CWSFKissParser::invalidatePacket()
{
  this->m_bEscape     = false;
  this->m_nPos        = 0;
  this->m_bHaveStart  = false;
};


bool CWSFKissParser::addData(char *pszData, int nLength)
{
  for(int n = 0; n < nLength; ++n)
  {
    switch(pszData[n])
    {
      case 0xDB: //ESC
      {
        if(this->m_bHaveStart == true)
        {
          if(this->m_bEscape == false)
          {
            this->m_bEscape = true;
          };
          
          if(this->m_nPos < MAX_KISS_PACKET_LEN)
          {
            this->m_pszPacket[this->m_nPos] = pszData[n];
            this->m_nPos += 1;
          };
        };
      };
      break;

      #if KISS_DEBUG == 1
        case 'z': //FRAME_END
      #endif
      
      case 0xC0: //FRAME_END
      {
        if(this->m_bEscape == false)
        {
          if(this->m_nPos == 0)
          {
            this->m_pszPacket[this->m_nPos] = pszData[n];
            this->m_nPos                    += 1;
            this->m_bHaveStart              = true;
          }
          else
          {
            if(this->m_nPos < MAX_KISS_PACKET_LEN)
            {
              this->m_pszPacket[this->m_nPos] = pszData[n];
              this->m_nPos += 1;
            };

            this->m_cbOnKissPacketComplete(this->m_pszPacket, this->m_nPos);

            this->m_nPos        = 0;
            this->m_bHaveStart  = false;
          };
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

      default:
      {
        if(this->m_bHaveStart == true)
        {
          if(this->m_nPos < MAX_KISS_PACKET_LEN)
          {
            this->m_pszPacket[this->m_nPos] = pszData[n];
            this->m_nPos += 1;
          };
        };
        
        this->m_bEscape = false;
      };
    };
  };

  return true;
};
    
