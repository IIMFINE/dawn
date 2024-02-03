#include <iostream>
#include <memory>

struct a
{

};

struct b : public a
{

};

int main()
{
  std::shared_ptr<a> a_ptr = std::make_shared<b>();
  std::shared_ptr<b> b_ptr = std::dynamic_pointer_cast<b>(a_ptr);
  std::cout << "end" << std::endl;
}
