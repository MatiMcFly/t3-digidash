#ifndef SCREENVIEW_HPP
#define SCREENVIEW_HPP

#include <gui_generated/screen_screen/screenViewBase.hpp>
#include <gui/screen_screen/screenPresenter.hpp>

class screenView : public screenViewBase
{
public:
    screenView();
    virtual ~screenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
protected:
    /* Test pattern: animate box2 size between 110x110 and 720x720 to
     * help debug LTDC / SDRAM bandwidth issues during dirty-rect
     * propagation. Step in pixels per tick (TouchGFX tick = ~16.6 ms
     * at 60 fps), direction 1 = grow, -1 = shrink. */
    int16_t box2Size;
    int8_t  box2Dir;
};

#endif // SCREENVIEW_HPP
