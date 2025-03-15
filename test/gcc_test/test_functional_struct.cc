#include <functional>
#include <iostream>

struct test
{
  void a(std::function<bool(void)> condition = []() {
    static bool ret = true;
    std::cout << ret << std::endl;
    std::cout << "address " << &ret << std::endl;
    return (ret = (ret == false));
    })
  {
    condition();
    return;
  }

  void b()
  {
    std::function<bool(void)> condition = []() {
      static bool ret = true;
      std::cout << ret << std::endl;
      std::cout << "address " << &ret << std::endl;
      return (ret = (ret == false));
      };
    condition();
  }

};

int main()
{
  test t;
  t.a();
  t.a();
  t.a();
  test t1;
  t1.a();

  std::cout << "b" << std::endl;
  t.b();
  t.b();
  t.b();
  test t2;
  t2.b();

  return 0;
}
