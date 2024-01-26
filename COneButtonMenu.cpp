//includes
//////////
#include "COneButtonMenu.h"



CWSFOneButtonMenu::CWSFOneButtonMenu(Adafruit_SSD1306 *pDisplay, sMenuItem *pMenu)
{
  this->m_pDisplay          = pDisplay;
  this->m_pMenu             = pMenu;
  this->m_pCurrentLevel     = NULL;
  this->m_pCurrentSelected  = NULL;
  this->m_bIsOpen           = false;
};


CWSFOneButtonMenu::~CWSFOneButtonMenu()
{
};

void CWSFOneButtonMenu::openMenu()
{
  this->m_bIsOpen = true;

  this->m_pCurrentLevel     = this->m_pMenu;
  this->m_pCurrentSelected  = this->m_pMenu;

  this->drawMenu();
};


void CWSFOneButtonMenu::drawMenu()
{
  //variables
  ///////////
  sMenuItem *pMenu = this->m_pCurrentLevel;
  
  this->m_pDisplay->clearDisplay();
  this->m_pDisplay->setTextSize(1.2);
  this->m_pDisplay->setCursor(0, 0);
  
  while(pMenu != NULL)
  {
    if(this->m_pCurrentSelected == pMenu)
    {
      this->m_pDisplay->println(String(F("[ ")) + pMenu->strMenuText + String(F(" ]")));  
    }
    else
    {
      this->m_pDisplay->println(pMenu->strMenuText);  
    };
    
    pMenu = pMenu->pNext;
  };

  this->m_pDisplay->display();
};





void CWSFOneButtonMenu::closeMenu()
{
  this->m_bIsOpen = false;
};


bool CWSFOneButtonMenu::isOpen()
{
  return this->m_bIsOpen;
};


void CWSFOneButtonMenu::shortPress()
{
  if(this->m_pCurrentLevel != NULL)
  {
    this->m_pCurrentSelected = this->m_pCurrentSelected->pNext;

    if(this->m_pCurrentSelected == NULL)
    {
      this->m_pCurrentSelected = this->m_pCurrentLevel;
    };

    this->drawMenu();
  };
};

void CWSFOneButtonMenu::longPress()
{
  if(this->m_pCurrentSelected != NULL)
  {
    if(this->m_pCurrentSelected->pSubMenu != NULL)
    {
      this->m_pCurrentLevel     = this->m_pCurrentSelected->pSubMenu;
      this->m_pCurrentSelected  = this->m_pCurrentSelected->pSubMenu;
      this->drawMenu();

      return;
    };

    if(this->m_pCurrentSelected->menuCallBack != NULL)
    {
      this->m_pCurrentSelected->menuCallBack();

      return;
    };
  };
};
