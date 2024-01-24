#ifndef __ONEBTNMENU__
#define __ONEBTNMENU__


//includes
//////////
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



typedef void(*tOnMenuItemSelected)();


typedef struct _tMenuItem
{
  String              strMenuText;
  tOnMenuItemSelected menuCallBack;
  _tMenuItem          *pSubMenu;
  _tMenuItem          *pNext;
} sMenuItem;



class CWSFOneButtonMenu
{
  public:
  
    CWSFOneButtonMenu(Adafruit_SSD1306 *pDisplay, sMenuItem *pMenu);
    ~CWSFOneButtonMenu();

    void openMenu();
    void closeMenu();
    bool isOpen();
    void shortPress();
    void longPress();

  private:
  
    //variables
    ///////////
    Adafruit_SSD1306  *m_pDisplay;
    sMenuItem         *m_pMenu;
    sMenuItem         *m_pCurrentLevel;
    sMenuItem         *m_pCurrentSelected;
    bool               m_bIsOpen;


    void drawMenu();
};





#endif
