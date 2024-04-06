#ifndef _FUNC_WRAPPER_H_
#define _FUNC_WRAPPER_H_

namespace dawn
{
    class FuncWrapper
    {
        struct FuncWrapperBaseImpl
        {
            virtual void call() = 0;
            virtual ~FuncWrapperBaseImpl() = default;
        };

        template <typename FUNC_T>
        struct FuncWrapperDriveImpl : FuncWrapperBaseImpl
        {
            FuncWrapperDriveImpl(FUNC_T &&wrapper_func) : wrapper_func_(std::forward<FUNC_T>(wrapper_func))
            {
            }
            virtual void call()
            {
                wrapper_func_();
            }
            FUNC_T wrapper_func_;
        };

    public:
        FuncWrapper(const FuncWrapper &ins) : p_wrapped_func_(ins.p_wrapped_func_)
        {
        }

        FuncWrapper &operator=(const FuncWrapper &ins)
        {
            p_wrapped_func_ = ins.p_wrapped_func_;
            return *this;
        }

        FuncWrapper() = default;
        ~FuncWrapper() = default;

        template <typename FUNC_T>
        FuncWrapper(FUNC_T given_func) : p_wrapped_func_(std::make_shared<FuncWrapperDriveImpl<FUNC_T>>(std::move(given_func)))
        {
        }

        // The move construct dont generate by implement.
        FuncWrapper(FuncWrapper &&given_func_wrapper) : p_wrapped_func_(std::move(given_func_wrapper.p_wrapped_func_))
        {
        }

        FuncWrapper &operator=(FuncWrapper &&ins)
        {
            p_wrapped_func_ = std::move(ins.p_wrapped_func_);
            return *this;
        }

        auto execFunc()
        {
            p_wrapped_func_->call();
        }

        auto operator()()
        {
            this->execFunc();
        }
        std::shared_ptr<FuncWrapperBaseImpl> p_wrapped_func_;
    };

};

#endif
