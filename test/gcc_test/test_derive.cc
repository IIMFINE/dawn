#include <iostream>

struct a
{
  virtual void test1(char b) = 0;
  bool test1(int b)
  {
    std::cout << "a" << std::endl;
  }
};

struct b : public a
{
  void test1(char b) override
  {
    std::cout << "b" << std::endl;
  }
  using a::test1;
};

int main()
{
  b b1;
  b1.test1('a');
  b1.test1(123);
}
