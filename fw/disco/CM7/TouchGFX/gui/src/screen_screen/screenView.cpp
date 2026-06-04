#include <gui/screen_screen/screenView.hpp>

screenView::screenView()
    : box2Size(110), box2Dir(1)
{

}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();
}

void screenView::tearDownScreen()
{
    screenViewBase::tearDownScreen();
}

void screenView::handleTickEvent()
{
    /* Grow/shrink box2 by ~10 px per tick. At 60 fps that's a full
     * 110 -> 720 sweep in ~1.0 s, then back. The rectangle is anchored
     * at (0,0) so width and height grow toward the bottom-right. */
    const int16_t kStep = 10;
    const int16_t kMin  = 110;
    const int16_t kMax  = 720;

    box2Size += (int16_t)(box2Dir * kStep);
    if (box2Size >= kMax)
    {
        box2Size = kMax;
        box2Dir  = -1;
    }
    else if (box2Size <= kMin)
    {
        box2Size = kMin;
        box2Dir  = 1;
    }

    /* invalidate() the OLD area before resize so the framework knows
     * which region to repaint, then set the new size and invalidate
     * the NEW area. */
    box2.invalidate();
    box2.setWidthHeight(box2Size, box2Size);
    box2.invalidate();
}
