#include "VEH.hpp"

// Static member initialization
VectoredExceptionHandler::ExceptionCallback VectoredExceptionHandler::exceptionCallback = nullptr;
PVOID VectoredExceptionHandler::handlerHandle = nullptr;

VectoredExceptionHandler::VectoredExceptionHandler() = default;

VectoredExceptionHandler::~VectoredExceptionHandler() {
    RemoveHandler();
}

bool VectoredExceptionHandler::AddHandler() {
    if (handlerHandle != nullptr) {
        std::cerr << "Handler already added!" << std::endl;
        return false;
    }

    handlerHandle = AddVectoredExceptionHandler(1, ExceptionHandler);
    if (!handlerHandle) {
        std::cerr << "Failed to add Vectored Exception Handler!" << std::endl;
        return false;
    }

    return true;
}

bool VectoredExceptionHandler::RemoveHandler() {
    if (!handlerHandle) {
        std::cerr << "Handler not installed!" << std::endl;
        return false;
    }

    if (RemoveVectoredExceptionHandler(handlerHandle)) {
        handlerHandle = nullptr;
        return true;
    }

    std::cerr << "Failed to remove Vectored Exception Handler!" << std::endl;
    return false;
}

void VectoredExceptionHandler::SetCallback(ExceptionCallback callback) {
    exceptionCallback = std::move(callback);
}

LONG CALLBACK VectoredExceptionHandler::ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        std::cerr << "Access Violation Caught!" << std::endl;

        if (exceptionCallback) {
            exceptionCallback(ExceptionInfo);
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
