#ifndef __SCREENBASE_H_
#define __SCREENBASE_H_

namespace Screens {
    class Base
    {
    public:
        Base() = default;
        virtual ~Base() = default;

        virtual void Update(double delta);
        virtual void Draw(double delta);
        
        virtual void OnKeyDown();
        virtual void OnKeyUp();

        virtual bool Attach();
        virtual bool Detach();
    };
}

#endif