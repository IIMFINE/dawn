#ifndef _FUNC_WRAPPER_H_
#define _FUNC_WRAPPER_H_

namespace dawn
{
class funcWrapper
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
            wrapperFunc_m(std::forward<FUNC_T>(wrapperFunc))
        {
        }
        virtual void call()
        {
            wrapperFunc_m();
        }
        FUNC_T wrapperFunc_m;
    };

public:
    funcWrapper(const funcWrapper&) = delete;
    funcWrapper(funcWrapper&) = delete;
    funcWrapper& operator=(const funcWrapper&) = delete;

    funcWrapper() = default;
    ~funcWrapper() = default;

    template<typename FUNC_T>
    funcWrapper(FUNC_T givenFunc):
        p_wrappedFunc(std::move(std::make_unique<funcWrapperDriveImpl<FUNC_T>>(std::move(givenFunc))))
    {
    }

    //The move construct dont generate by implement.
    funcWrapper(funcWrapper&& givenFuncWrapper):
        p_wrappedFunc(std::move(givenFuncWrapper.p_wrappedFunc))
    {
    }

    funcWrapper& operator=(funcWrapper&& ins)
    {
      p_wrappedFunc = std::move(ins.p_wrappedFunc);
      return *this;
    }

    auto execFunc()
    {
        p_wrappedFunc->call();
    }
    auto operator()()
    {
        this->execFunc();
    }
    std::unique_ptr<funcWrapperBaseImpl>  p_wrappedFunc;
};

};


#endif