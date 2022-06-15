#ifndef _FUNC_WARPPER_H_
#define _FUNC_WARPPER_H_

namespace net
{
class funcWarpper
{
    struct funcWrapperBaseImpl
    {
        virtual void call() = 0;
        virtual ~funcWrapperBaseImpl() = default;
    };

    template<typename FUNC_T>
    struct funcWrapperDriveImpl: funcWrapperBaseImpl
    {
        funcWrapperDriveImpl(FUNC_T&& wrapperFunc):
            warpperFunc_m(std::forward<FUNC_T>(wrapperFunc))
        {
        }
        virtual void call()
        {
            warpperFunc_m();
        }
        FUNC_T warpperFunc_m;
    };

public:
    funcWarpper(const funcWarpper&) = delete;
    funcWarpper(funcWarpper&) = delete;
    funcWarpper& operator=(const funcWarpper&) = delete;

    template<typename FUNC_T>
    funcWarpper(FUNC_T givenFunc):
        p_wrapedFunc(std::move(std::make_unique<funcWrapperDriveImpl<FUNC_T>>(std::move(givenFunc))))
    {
    }

    //The move construct dont generate by implement.
    funcWarpper(funcWarpper&& givenFuncWarpper):
        p_wrapedFunc(std::move(givenFuncWarpper.p_wrapedFunc))
    {
    }
    auto execFunc()
    {
        p_wrapedFunc->call();
    }
    auto operator()()
    {
        this->execFunc();
    }
    std::unique_ptr<funcWrapperBaseImpl>  p_wrapedFunc;
};

};


#endif