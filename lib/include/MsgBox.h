#ifndef __MSGBOX_H_
#define __MSGBOX_H_

#include <string>

namespace MsgBox {
    enum class Type
    {
        None,
        Ok,
        OkCancel,
        YesNo,
        YesNoCancel
    };

    enum class Flags {
        None,
        Info,
        Warning,
        Error
    };

    enum class Result {
        None,
        Ok,
        Cancel,
        Yes,
        No
    };

    Result Show(const std::string title, const std::string message, Type type = Type::None, Flags flags = Flags::None);
}

#endif