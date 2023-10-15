#ifndef __RECTANGLE_H_
#define __RECTANGLE_H_

#include "UIBase.h"

namespace UI {
    class Rectangle : public Base
    {
    public:
        Rectangle();

    protected:
        void OnDraw() override;
    };
}

#endif