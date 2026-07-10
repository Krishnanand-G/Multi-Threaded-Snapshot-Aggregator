#pragma once
#ifdef _WIN32
#include <windows.h>
#include <functional>
#include <chrono>

namespace compat {
    class thread {
    public:
        thread() : handle_(NULL) {}
        
        template<typename Function, typename... Args>
        explicit thread(Function&& f, Args&&... args) {
            auto lambda = [f = std::forward<Function>(f), args...]() mutable {
                f(args...);
            };
            using LambdaType = decltype(lambda);
            LambdaType* data = new LambdaType(std::move(lambda));
            
            handle_ = CreateThread(
                NULL,
                0,
                &thread::ThreadProxy<LambdaType>,
                data,
                0,
                NULL
            );
        }
        
        ~thread() {
            if (handle_) CloseHandle(handle_);
        }
        
        // Move-only support
        thread(const thread&) = delete;
        thread& operator=(const thread&) = delete;
        
        thread(thread&& other) noexcept : handle_(other.handle_) {
            other.handle_ = NULL;
        }
        
        thread& operator=(thread&& other) noexcept {
            if (this != &other) {
                if (handle_) CloseHandle(handle_);
                handle_ = other.handle_;
                other.handle_ = NULL;
            }
            return *this;
        }
        
        bool joinable() const {
            return handle_ != NULL;
        }
        
        void join() {
            if (handle_) {
                WaitForSingleObject(handle_, INFINITE);
                CloseHandle(handle_);
                handle_ = NULL;
            }
        }
        
    private:
        HANDLE handle_;

        template<typename LambdaType>
        static DWORD WINAPI ThreadProxy(LPVOID lpParam) {
            LambdaType* data_ptr = static_cast<LambdaType*>(lpParam);
            (*data_ptr)();
            delete data_ptr;
            return 0;
        }
    };
    
    namespace this_thread {
        inline void yield() {
            SwitchToThread();
        }
        
        template<typename Rep, typename Period>
        inline void sleep_for(const std::chrono::duration<Rep, Period>& duration) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            Sleep(static_cast<DWORD>(ms));
        }
    }
}
#endif
