#ifndef VECTORED_EXCEPTION_HANDLER_HPP
#define VECTORED_EXCEPTION_HANDLER_HPP

#include <Windows.h>
#include <functional>
#include <iostream>

class VectoredExceptionHandler {
public:
    using ExceptionCallback = std::function<void(EXCEPTION_POINTERS*)>;

    VectoredExceptionHandler();
    ~VectoredExceptionHandler();

    bool AddHandler();

    bool RemoveHandler();

    void SetCallback(ExceptionCallback callback);

private:
    static LONG CALLBACK ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);

    static ExceptionCallback exceptionCallback;
    static PVOID handlerHandle;
};

#endif // VECTORED_EXCEPTION_HANDLER_HPP
