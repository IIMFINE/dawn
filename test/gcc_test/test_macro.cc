#include <iostream>

char test_str[1024];

#define test(a, args...) printf(a, ##args)
#define test2(a, args...) snprintf(test_str, 1024, a,##args)


int main()
{
  test("hello world\n");
  test("hello world2\n");

  test2("hello world2\n");
  test("%s", test_str);
  test2("hello world %s\n", "pan");
  test("%s", test_str);

}


